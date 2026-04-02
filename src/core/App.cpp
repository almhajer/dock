#include "App.h"
#include "SceneLayout.h"
#include "../game/HunterSpriteAtlas.h"
#include "../game/HunterSpriteData.h"

#include <GLFW/glfw3.h>

#include <array>
#include <algorithm>
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

        [[nodiscard]] std::string directionalClip(const char* baseName, bool facingLeft)
        {
            return std::string(baseName) + (facingLeft ? "_left" : "_right");
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

        // تحميل مؤثر إطلاق النار مرة واحدة ثم إعادة استخدامه عند كل طلقة.
        mHunterShotSound.load(mAssetsPath + "/audio/hunter_shot.mp3");

        mGrassLayerId = mVulkan.createTexturedLayerFromPixels(
            WHITE_PIXEL.data(),
            1,
            1,
            scene::kMaxGrassQuads);
        std::cout << "[Grass] تم تفعيل العشب الإجرائي عبر الشيدر" << std::endl;

        // بدء حالة الوقوف يمينًا
        mSpriteAnim.play(mHunterState, "idle_right");

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
        updateHunterMotion(deltaTime);
        updateSoilRenderData();
        updateGrassRenderData();
        updateHunterRenderData();
        updateGroundInteraction(deltaTime);
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
            mVulkan.updateTexturedLayer(mSoilLayerId, {});
            return;
        }

        mVulkan.updateTexturedLayer(
            mSoilLayerId,
            {scene::buildSoilQuad()});
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

        bool facingLeft = mHunterState.flipX;
        if (moveIntent < 0)
        {
            facingLeft = true;
        }
        else if (moveIntent > 0)
        {
            facingLeft = false;
        }

        if (moveIntent != 0)
        {
            mHunterX += static_cast<float>(moveIntent) * mHunterSpeed * deltaTime;
        }

        const bool reloadRequested = mInput.isKeyJustPressed(GLFW_KEY_R);
        const bool shotRequested = mInput.isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT) && !mIsReloading;
        const bool aiming = mInput.isMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT);

        if (reloadRequested)
        {
            mIsReloading = true;
            mReloadTimer = actionTiming.reloadDurationSeconds;
        }
        else if (shotRequested)
        {
            // طبقة الإطلاق تعمل كسوبرلاير مؤقت فوق سبرايت الحركة الأساسية.
            mHunterShootActive = true;
            mHunterShootTransitionTimer = 0.0f;
            mHunterShootState.finished = true;
            mHunterShootAnim.play(mHunterShootState, directionalClip("shoot", facingLeft));
            mHunterShotSound.play();
        }

        if (moveIntent != 0)
        {
            mSpriteAnim.play(mHunterState, directionalClip("walk", facingLeft));
        }
        else
        {
            (void)aiming;
            mSpriteAnim.play(mHunterState, directionalClip("idle", facingLeft));
        }

        mSpriteAnim.update(mHunterState, deltaTime);

        if (mHunterShootActive)
        {
            mHunterShootAnim.update(mHunterShootState, deltaTime);

            // تسلسل الإطلاق: حركة إطلاق -> تثبيت على الفريم السادس -> انتظار -> الفريم السابع.
            if (isDirectionalClip(mHunterShootState.currentClip, "shoot") && mHunterShootState.finished)
            {
                mHunterShootTransitionTimer = actionTiming.shootRecoverHoldSeconds;
                mHunterShootAnim.play(mHunterShootState, directionalClip("shoot_recover", mHunterShootState.flipX));
            }
            else if (isDirectionalClip(mHunterShootState.currentClip, "shoot_recover"))
            {
                mHunterShootTransitionTimer = std::max(0.0f, mHunterShootTransitionTimer - deltaTime);
                if (mHunterShootTransitionTimer <= 0.0f)
                {
                    mHunterShootTransitionTimer = actionTiming.shootReadySettleSeconds;
                    mHunterShootAnim.play(mHunterShootState, directionalClip("shoot_ready", mHunterShootState.flipX));
                }
            }
            else if (isDirectionalClip(mHunterShootState.currentClip, "shoot_ready"))
            {
                const bool keepReadyPose = moveIntent == 0 && !shotRequested;
                if (!keepReadyPose)
                {
                    mHunterShootActive = false;
                    mHunterShootTransitionTimer = 0.0f;
                    mHunterShootState = {};
                }
                else
                {
                    mHunterShootTransitionTimer = std::max(0.0f, mHunterShootTransitionTimer - deltaTime);
                    if (mHunterShootTransitionTimer <= 0.0f)
                    {
                        mHunterShootActive = false;
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
            mVulkan.updateTexturedLayer(mGrassLayerId, {});
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
                mVulkan.updateTexturedLayer(layerId, {});
                return;
            }

            const game::AtlasFrame* frame = animation.getFrame(state.currentFrameIndex);
            if (frame == nullptr || frame->sourceWidth <= 0 || frame->sourceHeight <= 0)
            {
                mVulkan.updateTexturedLayer(layerId, {});
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
            mVulkan.updateTexturedLayer(layerId, {quad});
        };

        buildHunterQuadForState(
            mHunterLayerId,
            mSpriteAnim,
            mHunterState,
            !mHunterShootActive);

        buildHunterQuadForState(
            mHunterShootLayerId,
            mHunterShootAnim,
            mHunterShootState,
            mHunterShootActive);
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
