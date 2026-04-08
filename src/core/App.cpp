#include "App.h"
#include "SceneLayout.h"
#include "../game/HunterSpriteAtlas.h"
#include "../game/HunterSpriteData.h"

#include <GLFW/glfw3.h>

#include <array>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif

namespace core
{
    namespace
    {
        constexpr float FOOTPRINT_DECAY_PER_SECOND = 1.85f;
        constexpr float FOOTPRINT_FOLLOW_PER_SECOND = 18.0f;
        constexpr float FOOT_CONTACT_THRESHOLD = 0.08f;

        struct CursorScreenPosition
        {
            float x = 0.0f;
            float y = 0.0f;
            bool valid = false;
        };

        /// يجمع نية الحركة الأفقية في قيمة واحدة:
        /// -1 = يسار، 0 = بدون حركة، +1 = يمين.
        [[nodiscard]] int resolveMoveIntent(const Input& input)
        {
            int moveIntent = 0;
            if (input.isKeyPressed(GLFW_KEY_A) || input.isKeyPressed(GLFW_KEY_LEFT))
            {
                --moveIntent;
            }
            if (input.isKeyPressed(GLFW_KEY_D) || input.isKeyPressed(GLFW_KEY_RIGHT))
            {
                ++moveIntent;
            }
            return std::clamp(moveIntent, -1, 1);
        }

        /// اتجاه الوقوف والتصويب يتبع موقع المؤشر حول مركز الصياد.
        [[nodiscard]] bool resolveFacingLeftFromCursor(const CursorScreenPosition& cursor,
                                                       float hunterX,
                                                       bool fallbackFacingLeft)
        {
            if (!cursor.valid)
            {
                return fallbackFacingLeft;
            }

            if (cursor.x < hunterX)
            {
                return true;
            }
            if (cursor.x > hunterX)
            {
                return false;
            }
            return fallbackFacingLeft;
        }

        /// اتجاه المشي يتبع الإدخال المباشر بدل المؤشر.
        [[nodiscard]] bool resolveFacingLeftFromMoveIntent(int moveIntent,
                                                           bool fallbackFacingLeft)
        {
            if (moveIntent < 0)
            {
                return true;
            }
            if (moveIntent > 0)
            {
                return false;
            }
            return fallbackFacingLeft;
        }

        /// عند الوقوف يتبع الاتجاه المؤشر، وعند المشي يتبع المفاتيح.
        [[nodiscard]] bool resolveIdleFacingLeft(int moveIntent,
                                                 const CursorScreenPosition& cursor,
                                                 float hunterX,
                                                 bool fallbackFacingLeft)
        {
            if (moveIntent != 0)
            {
                return resolveFacingLeftFromMoveIntent(moveIntent, fallbackFacingLeft);
            }

            return resolveFacingLeftFromCursor(cursor, hunterX, fallbackFacingLeft);
        }

        void updateReloadState(bool& isReloading,
                               float& reloadTimer,
                               float deltaTime)
        {
            if (!isReloading)
            {
                return;
            }

            reloadTimer = std::max(0.0f, reloadTimer - deltaTime);
            if (reloadTimer <= 0.0f)
            {
                isReloading = false;
            }
        }

        [[nodiscard]] bool isWalkingClip(const game::AnimationState& state)
        {
            return state.currentClip == "walk_left" ||
                   state.currentClip == "walk_right";
        }

        [[nodiscard]] CursorScreenPosition resolveCursorScreenPosition(const Input& input,
                                                                      GLFWwindow* window)
        {
            CursorScreenPosition cursor;
            if (window == nullptr)
            {
                return cursor;
            }

            int windowWidth = 0;
            int windowHeight = 0;
            glfwGetWindowSize(window, &windowWidth, &windowHeight);
            if (windowWidth <= 0 || windowHeight <= 0)
            {
                return cursor;
            }

            const float clampedMouseX = std::clamp(
                input.getMouseX(),
                0.0f,
                static_cast<float>(windowWidth));
            const float clampedMouseY = std::clamp(
                input.getMouseY(),
                0.0f,
                static_cast<float>(windowHeight));

            cursor.x = (clampedMouseX / static_cast<float>(windowWidth)) * 2.0f - 1.0f;
            cursor.y = (clampedMouseY / static_cast<float>(windowHeight)) * 2.0f - 1.0f;
            cursor.valid = true;
            return cursor;
        }

        [[nodiscard]] bool shouldUseHighShootPose(const Input& input,
                                                  GLFWwindow* window,
                                                  float hunterX,
                                                  const scene::WindowMetrics& metrics,
                                                  const game::AtlasFrame* frame)
        {
            if (!metrics.valid() || frame == nullptr || frame->sourceW <= 0 || frame->sourceH <= 0)
            {
                return false;
            }

            const CursorScreenPosition cursor = resolveCursorScreenPosition(input, window);
            if (!cursor.valid)
            {
                return false;
            }

            const gfx::TexturedQuad hunterQuad = scene::buildHunterQuad(*frame, hunterX, metrics);
            const float aimOriginX = hunterX;
            const float aimOriginY = hunterQuad.screen.y0 +
                (hunterQuad.screen.y1 - hunterQuad.screen.y0) * 0.34f;
            const float dx = cursor.x - aimOriginX;
            const float dy = cursor.y - aimOriginY;

            return dy < -0.08f && (-dy) > std::abs(dx) * 1.35f;
        }
    } // namespace

    App::App(const Config &config)
        : mWindow(config.window), mAssetsPath(config.assetsPath)
    {
        init();
    }

    App::~App()
    {
        cleanup();
    }

    void App::init()
    {
        if (mInitialized)
            return;

        mInput.init(mWindow.getHandle());

        ui::Localization::Language sysLang = detectSystemLanguage();
        if (!mLocalization.load(mAssetsPath, sysLang))
        {
            mLocalization.load(mAssetsPath, ui::Localization::Language::English);
        }

        mVulkan.init(mWindow.getHandle(), mWindow.getWidth(), mWindow.getHeight());

        constexpr std::array<unsigned char, 4> WHITE_PIXEL = {255, 255, 255, 255};
        mSoilLayerId = mVulkan.createTexturedLayerFromPixels(
            WHITE_PIXEL.data(),
            1,
            1,
            1);

        // أطلس الصياد الموحد: حركة + إطلاق عادي + إطلاق عالي في صورة واحدة.
        const std::string atlasPath = mAssetsPath + "/sprite/hunter_atlas.png";
        mHunterLayerId = mVulkan.createTexturedLayerWithCallback(atlasPath, 1,
            [this](int w, int h, const unsigned char* /*pixels*/) {
                mSpriteAnim.setAtlasData(game::createHunterAtlasData(w, h));
            });

        mHunterShotSound.load(mAssetsPath + "/audio/hunter_shot.mp3");

        mGrassLayerId = mVulkan.createTexturedLayerFromPixels(
            WHITE_PIXEL.data(),
            1,
            1,
            scene::kMaxGrassQuads);
        std::cout << "[Grass] تم تفعيل العشب الإجرائي عبر الشيدر" << std::endl;

        mSpriteAnim.play(mHunterState, "idle_right");
        mNatureSystem.initialize(scene::makeWindowMetrics(mWindow.getWidth(), mWindow.getHeight()));

        mInitialized = true;
        mRunning = true;
    }

    ui::Localization::Language App::detectSystemLanguage()
    {
#ifdef __APPLE__
        CFArrayRef languages = CFLocaleCopyPreferredLanguages();
        if (languages)
        {
            const CFStringRef primaryLang = static_cast<CFStringRef>(CFArrayGetValueAtIndex(languages, 0));
            if (primaryLang)
            {
                char buffer[16] = {};
                CFStringGetCString(primaryLang, buffer, sizeof(buffer), kCFStringEncodingUTF8);
                CFRelease(languages);
                if (buffer[0] == 'a' && buffer[1] == 'r')
                {
                    return ui::Localization::Language::Arabic;
                }
            }
            else
            {
                CFRelease(languages);
            }
        }
#else
        const char *lang = std::getenv("LANG");
        if (lang && std::strncmp(lang, "ar", 2) == 0)
        {
            return ui::Localization::Language::Arabic;
        }
#endif
        return ui::Localization::Language::English;
    }

    void App::run()
    {
        if (!mInitialized)
        {
            throw std::runtime_error("لم تتم تهيئة التطبيق قبل استدعاء run()");
        }

        while (mRunning && mWindow.isOpen())
        {
            mTimer.update();
            mInput.update();
            mWindow.pollEvents();

            if (mWindow.wasResized())
            {
                mVulkan.onResize(mWindow.getWidth(), mWindow.getHeight());
            }

            update(mTimer.getDeltaTime());
            render();
        }

        mRunning = false;
    }

    void App::requestShutdown()
    {
        mRunning = false;
    }

    bool App::isRunning() const
    {
        return mRunning;
    }

    // ─── الوصول للمكونات ───────────────────────────────────────────

    Window &App::getWindow() { return mWindow; }
    const Window &App::getWindow() const { return mWindow; }
    Input &App::getInput() { return mInput; }
    const Input &App::getInput() const { return mInput; }
    Timer &App::getTimer() { return mTimer; }
    const Timer &App::getTimer() const { return mTimer; }
    ui::Localization &App::getLocalization() { return mLocalization; }
    const ui::Localization &App::getLocalization() const { return mLocalization; }

    // ─── الطرق الخاصة ──────────────────────────────────────────────

    void App::update(float deltaTime)
    {
        updateNatureSystem(deltaTime);
        updateHunterMotion(deltaTime);
        updateSoilRenderData();
        updateNatureRenderData();
        updateGrassRenderData();
        updateHunterRenderData();
        updateGroundInteraction(deltaTime);
    }

    void App::updateNatureSystem(float deltaTime)
    {
        mNatureSystem.update(
            deltaTime,
            scene::makeWindowMetrics(mWindow.getWidth(), mWindow.getHeight()));
    }

    void App::updateSoilRenderData()
    {
        if (mSoilLayerId == gfx::VulkanContext::INVALID_LAYER_ID)
        {
            return;
        }

        const scene::WindowMetrics metrics = scene::makeWindowMetrics(mWindow.getWidth(), mWindow.getHeight());
        if (!metrics.valid())
        {
            mSoilLayoutWidth = 0;
            mSoilLayoutHeight = 0;
            mVulkan.clearTexturedLayer(mSoilLayerId);
            return;
        }

        if (scene::sameWindowLayout(metrics, mSoilLayoutWidth, mSoilLayoutHeight))
        {
            return;
        }

        mSoilLayoutWidth = metrics.width;
        mSoilLayoutHeight = metrics.height;
        mVulkan.updateTexturedLayer(mSoilLayerId, scene::buildSoilQuad());
    }

    void App::updateHunterMotion(float deltaTime)
    {
        const game::HunterActionTiming& actionTiming = game::hunterActionTiming();
        updateReloadState(mIsReloading, mReloadTimer, deltaTime);

        const int moveIntent = resolveMoveIntent(mInput);
        const bool reloadRequested = mInput.isKeyJustPressed(GLFW_KEY_R);
        const bool shotRequested = mInput.isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT) && !mIsReloading;

        if (moveIntent != 0)
        {
            mHunterX += static_cast<float>(moveIntent) * mHunterSpeed * deltaTime;
        }

        const CursorScreenPosition cursor = resolveCursorScreenPosition(mInput, mWindow.getHandle());
        const auto playDirectionalClip = [this](const char* leftClipKey,
                                                const char* rightClipKey,
                                                bool facingLeft)
        {
            mSpriteAnim.play(mHunterState, facingLeft ? leftClipKey : rightClipKey);
        };

        const scene::WindowMetrics metrics = scene::makeWindowMetrics(mWindow.getWidth(), mWindow.getHeight());
        const game::AtlasFrame* baseFrame = mSpriteAnim.getFrame(mHunterState.currentFrameIndex);
        const bool highShotRequested = shotRequested &&
            shouldUseHighShootPose(
                mInput, mWindow.getHandle(), mHunterX, metrics, baseFrame);

        if (reloadRequested)
        {
            mIsReloading = true;
            mReloadTimer = actionTiming.reloadDurationSeconds;
        }

        const auto startShot = [&](bool highShot)
        {
            mFacingLeft = resolveFacingLeftFromCursor(cursor, mHunterX, mFacingLeft);
            mHunterPhase = highShot ? HunterPhase::ShootHigh : HunterPhase::Shoot;

            if (highShot)
            {
                playDirectionalClip("shoot_up_left", "shoot_up_right", mFacingLeft);
            }
            else
            {
                playDirectionalClip("shoot_left", "shoot_right", mFacingLeft);
            }

            mHunterShotSound.play();
        };

        const auto applyLocomotionClip = [&]()
        {
            mFacingLeft = resolveIdleFacingLeft(moveIntent, cursor, mHunterX, mFacingLeft);

            if (moveIntent != 0)
            {
                playDirectionalClip("walk_left", "walk_right", mFacingLeft);
            }
            else
            {
                playDirectionalClip("idle_left", "idle_right", mFacingLeft);
            }
        };

        bool applyLocomotionState = false;
        bool preserveShotFacingOnLocomotionEntry = false;
        switch (mHunterPhase)
        {
        case HunterPhase::Locomotion:
            applyLocomotionState = !shotRequested;
            if (shotRequested)
            {
                startShot(highShotRequested);
            }
            break;

        case HunterPhase::Shoot:
        case HunterPhase::ShootHigh:
            mSpriteAnim.update(mHunterState, deltaTime);

            if (shotRequested)
            {
                startShot(highShotRequested);
            }
            else if (mHunterState.finished)
            {
                mHunterPhase = HunterPhase::Locomotion;
                applyLocomotionState = true;
                preserveShotFacingOnLocomotionEntry = true;
            }
            break;
        } // switch

        if (applyLocomotionState && mHunterPhase == HunterPhase::Locomotion)
        {
            if (preserveShotFacingOnLocomotionEntry && moveIntent == 0)
            {
                playDirectionalClip("idle_left", "idle_right", mFacingLeft);
            }
            else
            {
                applyLocomotionClip();
            }
            mSpriteAnim.update(mHunterState, deltaTime);
        }
    }

    void App::updateGrassRenderData()
    {
        if (mGrassLayerId == gfx::VulkanContext::INVALID_LAYER_ID)
        {
            return;
        }

        const scene::WindowMetrics metrics = scene::makeWindowMetrics(mWindow.getWidth(), mWindow.getHeight());
        if (!metrics.valid())
        {
            mGrassLayoutWidth = 0;
            mGrassLayoutHeight = 0;
            mVulkan.clearTexturedLayer(mGrassLayerId);
            return;
        }
        if (scene::sameWindowLayout(metrics, mGrassLayoutWidth, mGrassLayoutHeight))
        {
            return;
        }

        mGrassLayoutWidth = metrics.width;
        mGrassLayoutHeight = metrics.height;
        const scene::GrassLayout layout = scene::buildGrassLayout(metrics);

        std::vector<gfx::TexturedQuad> quads;
        quads.reserve(scene::kMaxGrassQuads);

        constexpr std::array<scene::GrassDepthLayer, 3> GRASS_LAYERS = {
            scene::GrassDepthLayer::Background,
            scene::GrassDepthLayer::Midground,
            scene::GrassDepthLayer::Foreground,
        };

        for (scene::GrassDepthLayer layer : GRASS_LAYERS)
        {
            for (int i = 0; i < layout.tileCount && quads.size() < scene::kMaxGrassQuads; ++i)
            {
                scene::appendGrassQuads(quads, layout, i, layer);
            }
        }

        mVulkan.updateTexturedLayer(mGrassLayerId, quads);
    }

    void App::updateNatureRenderData()
    {
        const scene::WindowMetrics metrics = scene::makeWindowMetrics(mWindow.getWidth(), mWindow.getHeight());
        if (!metrics.valid())
        {
            mEnvironmentRenderData.cloudInstances.clear();
            mEnvironmentRenderData.backgroundTreeInstances.clear();
            mEnvironmentRenderData.foregroundTreeInstances.clear();
            mVulkan.updateEnvironmentBatches(
                mEnvironmentRenderData.cloudInstances,
                mEnvironmentRenderData.backgroundTreeInstances,
                mEnvironmentRenderData.foregroundTreeInstances);
            return;
        }

        mNatureSystem.buildRenderData(metrics, mEnvironmentRenderData);
        mVulkan.updateEnvironmentBatches(
            mEnvironmentRenderData.cloudInstances,
            mEnvironmentRenderData.backgroundTreeInstances,
            mEnvironmentRenderData.foregroundTreeInstances);
    }

    void App::updateHunterRenderData()
    {
        if (mHunterLayerId == gfx::VulkanContext::INVALID_LAYER_ID)
        {
            return;
        }

        if (mHunterState.currentClip.empty())
        {
            return;
        }

        const scene::WindowMetrics metrics = scene::makeWindowMetrics(mWindow.getWidth(), mWindow.getHeight());
        if (!metrics.valid())
        {
            return;
        }

        const game::AtlasFrame* frame = mSpriteAnim.getFrame(mHunterState.currentFrameIndex);
        if (frame == nullptr || frame->sourceW <= 0 || frame->sourceH <= 0)
        {
            mVulkan.clearTexturedLayer(mHunterLayerId);
            return;
        }

        gfx::UvRect uv;
        mSpriteAnim.getFrameUV(mHunterState.currentFrameIndex, uv.u0, uv.u1, uv.v0, uv.v1);
        if (mHunterState.flipX)
        {
            std::swap(uv.u0, uv.u1);
        }

        const float logicalHalfWidth = scene::hunterLogicalHalfWidth(*frame, metrics);
        const float clampMin = -1.0f + logicalHalfWidth;
        const float clampMax = 1.0f - logicalHalfWidth;
        if (clampMin < clampMax)
        {
            mHunterX = std::clamp(mHunterX, clampMin, clampMax);
        }

        gfx::TexturedQuad quad = scene::buildHunterQuad(*frame, mHunterX, metrics);
        quad.uv = uv;
        mVulkan.updateTexturedLayer(mHunterLayerId, quad);
    }

    void App::updateGroundInteraction(float deltaTime)
    {
        mLeftGroundPressure = std::max(0.0f, mLeftGroundPressure - deltaTime * FOOTPRINT_DECAY_PER_SECOND);
        mRightGroundPressure = std::max(0.0f, mRightGroundPressure - deltaTime * FOOTPRINT_DECAY_PER_SECOND);

        const bool walking = isWalkingClip(mHunterState);

        const scene::WindowMetrics metrics = scene::makeWindowMetrics(mWindow.getWidth(), mWindow.getHeight());
        const game::AtlasFrame* frame = mSpriteAnim.getFrame(mHunterState.currentFrameIndex);
        if (walking && metrics.valid() &&
            frame != nullptr && frame->sourceW > 0 && frame->sourceH > 0)
        {
            const float logicalHalfWidth = scene::hunterLogicalHalfWidth(*frame, metrics);
            const float hunterWidth = logicalHalfWidth * 2.0f;
            const float gait = scene::resolveGait(mHunterState.elapsed);
            const float leftTargetPressure = scene::resolvePressure(gait, true);
            const float rightTargetPressure = scene::resolvePressure(gait, false);
            const float leftFootTargetX = scene::resolveFootTargetX(mHunterX, hunterWidth, gait, true);
            const float rightFootTargetX = scene::resolveFootTargetX(mHunterX, hunterWidth, gait, false);
            const float followFactor = std::clamp(deltaTime * FOOTPRINT_FOLLOW_PER_SECOND, 0.0f, 1.0f);

            mGroundFootRadius = std::max(hunterWidth * 0.16f, 0.05f);

            if (leftTargetPressure > FOOT_CONTACT_THRESHOLD)
            {
                if (mLeftGroundPressure <= FOOT_CONTACT_THRESHOLD)
                {
                    mLeftGroundX = leftFootTargetX;
                }
                mLeftGroundX += (leftFootTargetX - mLeftGroundX) * followFactor;
                mLeftGroundPressure = std::max(mLeftGroundPressure, leftTargetPressure * leftTargetPressure);
            }

            if (rightTargetPressure > FOOT_CONTACT_THRESHOLD)
            {
                if (mRightGroundPressure <= FOOT_CONTACT_THRESHOLD)
                {
                    mRightGroundX = rightFootTargetX;
                }
                mRightGroundX += (rightFootTargetX - mRightGroundX) * followFactor;
                mRightGroundPressure = std::max(mRightGroundPressure, rightTargetPressure * rightTargetPressure);
            }
        }

        mVulkan.setGroundInteraction(
            mLeftGroundX,
            mRightGroundX,
            mGroundFootRadius,
            mLeftGroundPressure,
            mRightGroundPressure);
    }

    void App::render()
    {
        mVulkan.drawFrame();
    }

    void App::cleanup()
    {
        if (!mInitialized)
            return;

        mInput.shutdown();
        mHunterShotSound.reset();

        mVulkan.waitIdle();
        mVulkan.cleanup();
        mInitialized = false;
    }

} // namespace core
