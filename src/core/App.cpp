#include "App.h"
#include "SceneLayout.h"

#include <GLFW/glfw3.h>

#include <array>
#include <algorithm>
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
            [this](int w, int h, const unsigned char* pixels) {
                mSpriteAnim.buildHunterAtlas(w, h, pixels);
            });

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
        bool moving = false;
        const std::string lastDirection = mHunterState.flipX ? "left" : "right";

        if (mInput.isKeyPressed(GLFW_KEY_RIGHT))
        {
            mHunterX += mHunterSpeed * deltaTime;
            moving = true;
            if (mHunterState.currentClip != "walk_right" || mHunterState.finished)
            {
                mSpriteAnim.play(mHunterState, "walk_right");
            }
        }
        else if (mInput.isKeyPressed(GLFW_KEY_LEFT))
        {
            mHunterX -= mHunterSpeed * deltaTime;
            moving = true;
            if (mHunterState.currentClip != "walk_left" || mHunterState.finished)
            {
                mSpriteAnim.play(mHunterState, "walk_left");
            }
        }

        if (!moving)
        {
            const std::string idleClip = (lastDirection == "right") ? "idle_right" : "idle_left";
            if (mHunterState.currentClip != idleClip)
            {
                mSpriteAnim.play(mHunterState, idleClip);
            }
        }

        mSpriteAnim.update(mHunterState, deltaTime);
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

        for (int i = 0; i < layout.tileCount && quads.size() < scene::kMaxGrassQuads; ++i)
        {
            scene::appendGrassQuads(quads, layout, i);
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

        const game::AtlasFrame* frame = mSpriteAnim.getFrame(mHunterState.currentFrameIndex);
        if (frame == nullptr || frame->sourceWidth <= 0 || frame->sourceHeight <= 0)
        {
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
        mVulkan.updateTexturedLayer(mHunterLayerId, {quad});
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

        mVulkan.waitIdle();
        mVulkan.cleanup();
        mInitialized = false;
    }

} // namespace core
