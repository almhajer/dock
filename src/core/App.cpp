#include "App.h"
#include "SceneLayout.h"
#include "../game/HunterSpriteAtlas.h"
#include "../game/HunterSpriteData.h"

#include <GLFW/glfw3.h>

#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string_view>
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

        enum class HunterClip : std::uint8_t
        {
            Idle,
            Walk,
            Shoot,
            ShootRecover,
            ShootReady,
            ShootHigh,
            ShootHighReady,
        };

        struct CursorScreenPosition
        {
            float x = 0.0f;
            float y = 0.0f;
            bool valid = false;
        };

        [[nodiscard]] const std::string& directionalClipKey(HunterClip clip, bool facingLeft)
        {
            static const std::string kIdleLeft = "idle_left";
            static const std::string kIdleRight = "idle_right";
            static const std::string kWalkLeft = "walk_left";
            static const std::string kWalkRight = "walk_right";
            static const std::string kShootLeft = "shoot_left";
            static const std::string kShootRight = "shoot_right";
            static const std::string kShootRecoverLeft = "shoot_recover_left";
            static const std::string kShootRecoverRight = "shoot_recover_right";
            static const std::string kShootReadyLeft = "shoot_ready_left";
            static const std::string kShootReadyRight = "shoot_ready_right";
            static const std::string kShootHighLeft = "shoot_up_left";
            static const std::string kShootHighRight = "shoot_up_right";
            static const std::string kShootHighReadyLeft = "shoot_up_ready_left";
            static const std::string kShootHighReadyRight = "shoot_up_ready_right";

            switch (clip)
            {
            case HunterClip::Idle:
                return facingLeft ? kIdleLeft : kIdleRight;
            case HunterClip::Walk:
                return facingLeft ? kWalkLeft : kWalkRight;
            case HunterClip::Shoot:
                return facingLeft ? kShootLeft : kShootRight;
            case HunterClip::ShootRecover:
                return facingLeft ? kShootRecoverLeft : kShootRecoverRight;
            case HunterClip::ShootReady:
                return facingLeft ? kShootReadyLeft : kShootReadyRight;
            case HunterClip::ShootHigh:
                return facingLeft ? kShootHighLeft : kShootHighRight;
            case HunterClip::ShootHighReady:
                return facingLeft ? kShootHighReadyLeft : kShootHighReadyRight;
            }

            return facingLeft ? kIdleLeft : kIdleRight;
        }

        [[nodiscard]] bool isDirectionalClip(const std::string& clipKey, std::string_view baseName)
        {
            auto matches = [&](std::string_view suffix)
            {
                return clipKey.size() == (baseName.size() + suffix.size()) &&
                    clipKey.compare(0, baseName.size(), baseName) == 0 &&
                    clipKey.compare(baseName.size(), suffix.size(), suffix) == 0;
            };
            return matches("_left") || matches("_right");
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

        [[nodiscard]] bool resolveCursorFacingLeft(const Input& input,
                                                   GLFWwindow* window,
                                                   float hunterX,
                                                   bool fallbackFacingLeft)
        {
            const CursorScreenPosition cursor = resolveCursorScreenPosition(input, window);
            if (!cursor.valid)
            {
                return fallbackFacingLeft;
            }

            const float cursorX = cursor.x;
            if (cursorX < hunterX)
            {
                return true;
            }
            if (cursorX > hunterX)
            {
                return false;
            }
            return fallbackFacingLeft;
        }

        [[nodiscard]] bool shouldUseHighShootPose(const Input& input,
                                                  GLFWwindow* window,
                                                  float hunterX,
                                                  const scene::WindowMetrics& metrics,
                                                  const game::AtlasFrame* frame)
        {
            if (!metrics.valid() || frame == nullptr || frame->sourceWidth <= 0 || frame->sourceHeight <= 0)
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

            // نعتبرها طلقة عالية فقط عندما يكون المؤشر أعلى الصياد بزاوية واضحة، لا بمجرد فرق بسيط.
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

        // تهيئة نظام الإدخال وربطه بالنافذة
        mInput.init(mWindow.getHandle());

        // كشف لغة النظام وتحميل الترجمة المناسبة
        ui::Localization::Language sysLang = detectSystemLanguage();
        if (!mLocalization.load(mAssetsPath, sysLang))
        {
            mLocalization.load(mAssetsPath, ui::Localization::Language::English);
        }

        // تهيئة سياق Vulkan
        mVulkan.init(mWindow.getHandle(), mWindow.getWidth(), mWindow.getHeight());

        constexpr std::array<unsigned char, 4> WHITE_PIXEL = {255, 255, 255, 255};
        mSoilLayerId = mVulkan.createTexturedLayerFromPixels(
            WHITE_PIXEL.data(),
            1,
            1,
            1);

        // الصياد في المنتصف بين التربة الخلفية والعشب الأمامي.
        const std::string spritePath = mAssetsPath + "/sprite/sprite.png";
        mHunterLayerId = mVulkan.createTexturedLayerWithCallback(spritePath, 1,
            [this](int w, int h, const unsigned char* /*pixels*/) {
                mSpriteAnim.setAtlasData(game::createHunterSpriteAtlasData(w, h));
            });

        const std::string shootSpritePath = mAssetsPath + "/sprite/hunterGun.png";
        mHunterShootLayerId = mVulkan.createTexturedLayerWithCallback(shootSpritePath, 1,
            [this](int w, int h, const unsigned char* /*pixels*/) {
                mHunterShootAnim.setAtlasData(game::createHunterShootSpriteAtlasData(w, h));
            });

        const std::string highShootSpritePath = mAssetsPath + "/sprite/hunterShotUP.png";
        mHunterHighShootLayerId = mVulkan.createTexturedLayerWithCallback(highShootSpritePath, 1,
            [this](int w, int h, const unsigned char* /*pixels*/) {
                mHunterHighShootAnim.setAtlasData(game::createHunterHighShootSpriteAtlasData(w, h));
            });

        // تحميل مؤثر إطلاق النار مرة واحدة ثم إعادة استخدامه عند كل طلقة.
        mHunterShotSound.load(mAssetsPath + "/audio/hunter_shot.mp3");

        mGrassLayerId = mVulkan.createTexturedLayerFromPixels(
            WHITE_PIXEL.data(),
            1,
            1,
            scene::kMaxGrassQuads);
        std::cout << "[Grass] تم تفعيل العشب الإجرائي عبر الشيدر" << std::endl;

        // بدء حالة الوقوف يمينًا
        mSpriteAnim.play(mHunterState, directionalClipKey(HunterClip::Idle, false));
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
            mInput.update();       // حفظ الحالة السابقة
            mWindow.pollEvents();  // تحديث الحالة الحالية عبر callbacks

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
        // نحدّث مؤقت إعادة التعبئة أولًا حتى لا يتداخل مع طلب الإطلاق الجديد.
        if (mIsReloading)
        {
            mReloadTimer = std::max(0.0f, mReloadTimer - deltaTime);
            if (mReloadTimer <= 0.0f)
            {
                mIsReloading = false;
            }
        }

        int moveIntent = 0;
        if (mInput.isKeyPressed(GLFW_KEY_A) || mInput.isKeyPressed(GLFW_KEY_LEFT))
        {
            --moveIntent;
        }
        if (mInput.isKeyPressed(GLFW_KEY_D) || mInput.isKeyPressed(GLFW_KEY_RIGHT))
        {
            ++moveIntent;
        }
        moveIntent = std::clamp(moveIntent, -1, 1);

        const bool reloadRequested = mInput.isKeyJustPressed(GLFW_KEY_R);
        const bool shotRequested = mInput.isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT) && !mIsReloading;

        // جهة الصياد أثناء الحركة تعتمد على الحركة الحالية، وأثناء الوقوف تتبع المؤشر أفقيًا.
        bool facingLeft = mHunterState.flipX;
        if (moveIntent < 0)
        {
            facingLeft = true;
        }
        else if (moveIntent > 0)
        {
            facingLeft = false;
        }
        else
        {
            facingLeft = resolveCursorFacingLeft(
                mInput,
                mWindow.getHandle(),
                mHunterX,
                facingLeft);
        }

        if (moveIntent != 0)
        {
            mHunterX += static_cast<float>(moveIntent) * mHunterSpeed * deltaTime;
        }

        const scene::WindowMetrics metrics = scene::makeWindowMetrics(mWindow.getWidth(), mWindow.getHeight());
        const game::AtlasFrame* baseFrame = mSpriteAnim.getFrame(mHunterState.currentFrameIndex);
        const bool highShotRequested = shotRequested &&
            shouldUseHighShootPose(
                mInput,
                mWindow.getHandle(),
                mHunterX,
                metrics,
                baseFrame);

        if (reloadRequested)
        {
            mIsReloading = true;
            mReloadTimer = actionTiming.reloadDurationSeconds;
        }
        else if (shotRequested)
        {
            // الإطلاق يحدد الجهة من موضع المؤشر يمين/يسار الصياد بدون تحريك موقعه.
            facingLeft = resolveCursorFacingLeft(
                mInput,
                mWindow.getHandle(),
                mHunterX,
                facingLeft);

            mHunterShootTransitionTimer = 0.0f;
            mHunterShootAutoHolster = false;

            if (highShotRequested)
            {
                mHunterShootActive = false;
                mHunterShootState = {};
                mHunterHighShootActive = true;
                mHunterHighShootState.finished = true;
                mHunterHighShootAnim.play(
                    mHunterHighShootState,
                    directionalClipKey(HunterClip::ShootHigh, facingLeft));
            }
            else
            {
                // طبقة الإطلاق العادي تعمل كسوبرلاير مؤقت فوق سبرايت الحركة الأساسية.
                mHunterHighShootActive = false;
                mHunterHighShootState = {};
                mHunterShootActive = true;
                mHunterShootState.finished = true;
                mHunterShootAnim.play(
                    mHunterShootState,
                    directionalClipKey(HunterClip::Shoot, facingLeft));
            }
            mHunterShotSound.play();
        }

        if (moveIntent != 0)
        {
            mSpriteAnim.play(mHunterState, directionalClipKey(HunterClip::Walk, facingLeft));
        }
        else
        {
            mSpriteAnim.play(mHunterState, directionalClipKey(HunterClip::Idle, facingLeft));
        }

        mSpriteAnim.update(mHunterState, deltaTime);

        if (mHunterHighShootActive)
        {
            mHunterHighShootAnim.update(mHunterHighShootState, deltaTime);

            if (isDirectionalClip(mHunterHighShootState.currentClip, "shoot_up") && mHunterHighShootState.finished)
            {
                const bool highShootFacingLeft = resolveCursorFacingLeft(
                    mInput,
                    mWindow.getHandle(),
                    mHunterX,
                    mHunterHighShootState.flipX);
                mHunterShootTransitionTimer = actionTiming.highShootReadyHoldSeconds;
                mHunterHighShootAnim.play(
                    mHunterHighShootState,
                    directionalClipKey(HunterClip::ShootHighReady, highShootFacingLeft));
            }
            else if (isDirectionalClip(mHunterHighShootState.currentClip, "shoot_up_ready"))
            {
                const bool keepHighReadyPose = moveIntent == 0 && !shotRequested && !mIsReloading;
                if (!keepHighReadyPose)
                {
                    mHunterHighShootActive = false;
                    mHunterShootTransitionTimer = 0.0f;
                    mHunterHighShootState = {};
                }
                else
                {
                    const bool readyFacingLeft = resolveCursorFacingLeft(
                        mInput,
                        mWindow.getHandle(),
                        mHunterX,
                        mHunterHighShootState.flipX);
                    if (readyFacingLeft != mHunterHighShootState.flipX)
                    {
                        mHunterHighShootAnim.play(
                            mHunterHighShootState,
                            directionalClipKey(HunterClip::ShootHighReady, readyFacingLeft));
                    }

                    mHunterShootTransitionTimer = std::max(0.0f, mHunterShootTransitionTimer - deltaTime);
                    if (mHunterShootTransitionTimer <= 0.0f)
                    {
                        mHunterHighShootActive = false;
                        mHunterHighShootState = {};
                        mHunterShootActive = true;
                        mHunterShootAutoHolster = true;
                        mHunterShootTransitionTimer = actionTiming.shootReadySettleSeconds;
                        mHunterShootAnim.play(
                            mHunterShootState,
                            directionalClipKey(HunterClip::ShootReady, readyFacingLeft));
                    }
                }
            }
        }

        if (mHunterShootActive)
        {
            mHunterShootAnim.update(mHunterShootState, deltaTime);

            // تسلسل الإطلاق: تمر الحركة عبر الفريم الخامس بدون توقف، ثم وقفة قصيرة على السادس، ثم السابع.
            if (isDirectionalClip(mHunterShootState.currentClip, "shoot") && mHunterShootState.finished)
            {
                const bool shootFacingLeft = resolveCursorFacingLeft(
                    mInput,
                    mWindow.getHandle(),
                    mHunterX,
                    mHunterShootState.flipX);
                mHunterShootTransitionTimer = actionTiming.shootRecoverHoldSeconds;
                mHunterShootAnim.play(mHunterShootState, directionalClipKey(HunterClip::ShootRecover, shootFacingLeft));
            }
            else if (isDirectionalClip(mHunterShootState.currentClip, "shoot_recover"))
            {
                const bool recoverFacingLeft = resolveCursorFacingLeft(
                    mInput,
                    mWindow.getHandle(),
                    mHunterX,
                    mHunterShootState.flipX);
                if (recoverFacingLeft != mHunterShootState.flipX)
                {
                    mHunterShootAnim.play(
                        mHunterShootState,
                        directionalClipKey(HunterClip::ShootRecover, recoverFacingLeft));
                }

                mHunterShootTransitionTimer = std::max(0.0f, mHunterShootTransitionTimer - deltaTime);
                if (mHunterShootTransitionTimer <= 0.0f)
                {
                    mHunterShootTransitionTimer = 0.0f;
                    mHunterShootAnim.play(mHunterShootState, directionalClipKey(HunterClip::ShootReady, recoverFacingLeft));
                }
            }
            else if (isDirectionalClip(mHunterShootState.currentClip, "shoot_ready"))
            {
                const bool readyFacingLeft = resolveCursorFacingLeft(
                    mInput,
                    mWindow.getHandle(),
                    mHunterX,
                    mHunterShootState.flipX);
                if (readyFacingLeft != mHunterShootState.flipX)
                {
                    mHunterShootAnim.play(
                        mHunterShootState,
                        directionalClipKey(HunterClip::ShootReady, readyFacingLeft));
                }

                if (mHunterShootAutoHolster)
                {
                    mHunterShootTransitionTimer = std::max(0.0f, mHunterShootTransitionTimer - deltaTime);
                    if (mHunterShootTransitionTimer <= 0.0f)
                    {
                        mHunterShootActive = false;
                        mHunterShootAutoHolster = false;
                        mHunterShootTransitionTimer = 0.0f;
                        mHunterShootState = {};
                    }
                }
                else
                {
                    const bool keepReadyPose = moveIntent == 0 && !shotRequested && !mIsReloading;
                    if (!keepReadyPose)
                    {
                        mHunterShootActive = false;
                        mHunterShootTransitionTimer = 0.0f;
                        mHunterShootState = {};
                    }
                }
            }
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

        // نبني بيانات الرسم في حاويات معاد استخدامها لتقليل التخصيصات داخل الحلقة الرئيسية.
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

        // نستخدم دالة محلية واحدة حتى يبقى تحديث طبقة الحركة وطبقة الإطلاق متطابقًا.
        auto buildHunterQuadForState =
            [&](gfx::VulkanContext::LayerId layerId,
                game::SpriteAnimation& animation,
                game::AnimationState& state,
                bool visible)
        {
            if (layerId == gfx::VulkanContext::INVALID_LAYER_ID)
            {
                return;
            }
            if (!visible || state.currentClip.empty())
            {
                mVulkan.clearTexturedLayer(layerId);
                return;
            }

            const game::AtlasFrame* frame = animation.getFrame(state.currentFrameIndex);
            if (frame == nullptr || frame->sourceWidth <= 0 || frame->sourceHeight <= 0)
            {
                mVulkan.clearTexturedLayer(layerId);
                return;
            }

            gfx::UvRect uv;
            animation.getFrameUV(state.currentFrameIndex, uv.u0, uv.u1, uv.v0, uv.v1);
            if (state.flipX)
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
            mVulkan.updateTexturedLayer(layerId, quad);
        };

        buildHunterQuadForState(
            mHunterLayerId,
            mSpriteAnim,
            mHunterState,
            !mHunterShootActive && !mHunterHighShootActive);

        buildHunterQuadForState(
            mHunterShootLayerId,
            mHunterShootAnim,
            mHunterShootState,
            mHunterShootActive);

        buildHunterQuadForState(
            mHunterHighShootLayerId,
            mHunterHighShootAnim,
            mHunterHighShootState,
            mHunterHighShootActive);
    }

    void App::updateGroundInteraction(float deltaTime)
    {
        mLeftGroundPressure = std::max(0.0f, mLeftGroundPressure - deltaTime * FOOTPRINT_DECAY_PER_SECOND);
        mRightGroundPressure = std::max(0.0f, mRightGroundPressure - deltaTime * FOOTPRINT_DECAY_PER_SECOND);

        const bool walking =
            mHunterState.currentClip == "walk_left" ||
            mHunterState.currentClip == "walk_right";

        const scene::WindowMetrics metrics = scene::makeWindowMetrics(mWindow.getWidth(), mWindow.getHeight());
        const game::AtlasFrame* frame = mSpriteAnim.getFrame(mHunterState.currentFrameIndex);
        if (walking && metrics.valid() &&
            frame != nullptr && frame->sourceWidth > 0 && frame->sourceHeight > 0)
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

        // فصل Input أولًا لمنع callbacks من الكتابة على ذاكرة مُدمّرة
        mInput.shutdown();

        // تحرير الصوت قبل تنظيف Vulkan حتى تبقى دورة الإغلاق مرتبة وواضحة.
        mHunterShotSound.reset();

        mVulkan.waitIdle();
        mVulkan.cleanup();
        mInitialized = false;
    }

} // namespace core
