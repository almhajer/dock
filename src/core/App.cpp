#include "App.h"
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <cmath>
#include <array>
#include <iostream>
#include <locale>
#include <vector>

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif

namespace core
{

    namespace
    {
        constexpr float HUNTER_SCREEN_HALF_HEIGHT = 0.5f;
        constexpr float HUNTER_BOTTOM_MARGIN = 0.04f;
        constexpr float GRASS_SCREEN_HEIGHT = 0.30f;
        constexpr float SOIL_SCREEN_HEIGHT = 0.115f;
        constexpr float GRASS_OVERLAP_RATIO = 0.48f;
        constexpr float GRASS_CLUMP_ASPECT = 0.92f;
        constexpr std::size_t MAX_GRASS_QUADS = 560;
        constexpr float FOOTPRINT_DECAY_PER_SECOND = 1.85f;
        constexpr float FOOTPRINT_FOLLOW_PER_SECOND = 18.0f;
        constexpr float FOOT_CONTACT_THRESHOLD = 0.08f;

        float hash01(float value)
        {
            const float raw = std::sin(value * 127.1f + 311.7f) * 43758.5453f;
            return raw - std::floor(raw);
        }

        gfx::TexturedQuad buildHunterQuad(const game::AtlasFrame& frame,
                                          float hunterX,
                                          float windowAspect)
        {
            const float logicalHalfHeight = HUNTER_SCREEN_HALF_HEIGHT;
            const float logicalHeight = logicalHalfHeight * 2.0f;
            const float logicalHalfWidth = logicalHalfHeight *
                                           (static_cast<float>(frame.sourceWidth) /
                                            static_cast<float>(frame.sourceHeight)) /
                                           windowAspect;
            const float logicalX0 = hunterX - logicalHalfWidth;
            // تثبيت أسفل الخلية المنطقية فوق هامش الشاشة السفلي.
            const float logicalY0 = 1.0f - HUNTER_BOTTOM_MARGIN * 2.0f - logicalHeight;
            const float logicalWidth = logicalHalfWidth * 2.0f;

            gfx::TexturedQuad quad;
            quad.screen.x0 = logicalX0 + logicalWidth *
                (static_cast<float>(frame.offsetX) / static_cast<float>(frame.sourceWidth));
            quad.screen.x1 = quad.screen.x0 + logicalWidth *
                (static_cast<float>(frame.width) / static_cast<float>(frame.sourceWidth));
            quad.screen.y0 = logicalY0 + logicalHeight *
                (static_cast<float>(frame.offsetY) / static_cast<float>(frame.sourceHeight));
            quad.screen.y1 = quad.screen.y0 + logicalHeight *
                (static_cast<float>(frame.height) / static_cast<float>(frame.sourceHeight));
            return quad;
        }

        gfx::TexturedQuad buildGrassQuad(float tileLeft,
                                         float rowBottom,
                                         float tileWidth,
                                         float tileHeight)
        {
            gfx::TexturedQuad quad;
            quad.screen.x0 = tileLeft;
            quad.screen.x1 = tileLeft + tileWidth;
            quad.screen.y1 = rowBottom;
            quad.screen.y0 = rowBottom - tileHeight;
            return quad;
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
            [this](int w, int h, const unsigned char* pixels) {
                mSpriteAnim.buildHunterAtlas(w, h, pixels);
            });

        mGrassLayerId = mVulkan.createTexturedLayerFromPixels(
            WHITE_PIXEL.data(),
            1,
            1,
            MAX_GRASS_QUADS);
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

        const uint32_t soilWidth = mWindow.getWidth();
        const uint32_t soilHeight = mWindow.getHeight();
        if (soilWidth < 1 || soilHeight < 1)
        {
            mVulkan.updateTexturedLayer(mSoilLayerId, {});
            return;
        }

        gfx::TexturedQuad soilQuad;
        soilQuad.screen.x0 = -1.02f;
        soilQuad.screen.x1 = 1.02f;
        soilQuad.screen.y1 = 1.0f;
        soilQuad.screen.y0 = 1.0f - SOIL_SCREEN_HEIGHT;
        soilQuad.uv = {0.0f, 1.0f, 0.0f, 1.0f};
        soilQuad.materialType = gfx::QUAD_MATERIAL_PROCEDURAL_SOIL;
        mVulkan.updateTexturedLayer(mSoilLayerId, {soilQuad});
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

        const uint32_t grassLayoutWidth = mWindow.getWidth();
        const uint32_t grassLayoutHeight = mWindow.getHeight();
        const float winW = static_cast<float>(grassLayoutWidth);
        const float winH = static_cast<float>(grassLayoutHeight);
        if (winW < 1.0f || winH < 1.0f)
        {
            mGrassLayoutWidth = 0;
            mGrassLayoutHeight = 0;
            mVulkan.updateTexturedLayer(mGrassLayerId, {});
            return;
        }
        if (mGrassLayoutWidth == grassLayoutWidth &&
            mGrassLayoutHeight == grassLayoutHeight)
        {
            return;
        }

        mGrassLayoutWidth = grassLayoutWidth;
        mGrassLayoutHeight = grassLayoutHeight;

        const float winAspect = winW / winH;
        const float tileHeight = GRASS_SCREEN_HEIGHT;
        const float tileWidth = tileHeight * GRASS_CLUMP_ASPECT / winAspect;
        const float tileStep = std::max(tileWidth * GRASS_OVERLAP_RATIO, 0.0001f);
        const float overflowPadding = tileWidth;
        const float startX = -1.0f - overflowPadding;
        const float coverageWidth = 2.0f + overflowPadding * 2.0f;
        const int tileCount = std::max(
            1,
            static_cast<int>(std::ceil(coverageWidth / tileStep)) + 1);

        std::vector<gfx::TexturedQuad> quads;
        quads.reserve(MAX_GRASS_QUADS);

        for (int i = 0; i < tileCount && quads.size() < MAX_GRASS_QUADS; ++i)
        {
            const float clumpIndex = static_cast<float>(i);
            const float randomA = hash01(clumpIndex + 1.37f);
            const float randomB = hash01(clumpIndex * 1.91f + 4.12f);
            const float randomC = hash01(clumpIndex * 2.73f + 9.81f);
            const float randomD = hash01(clumpIndex * 4.07f + 2.11f);

            const float heightScale = 0.98f + randomA * 0.06f;
            const float widthScale = 0.88f + randomB * 0.14f;
            const float xJitter = (randomC - 0.5f) * tileStep * 0.08f;
            const float alpha = 0.86f + randomB * 0.10f;

            gfx::TexturedQuad backQuad = buildGrassQuad(
                startX + tileStep * static_cast<float>(i) - tileStep * 0.12f + xJitter * 0.45f,
                1.0f,
                tileWidth * (widthScale * (0.99f + randomD * 0.06f)),
                tileHeight * (heightScale * (0.98f + randomC * 0.04f)));
            backQuad.uv = {0.0f, 1.0f, 0.0f, 1.0f};
            backQuad.alpha = 0.52f + randomD * 0.18f;
            backQuad.windWeight = 1.0f;
            backQuad.windPhase = clumpIndex * 0.47f + randomD * 7.0f;
            backQuad.windResponse = 0.62f + heightScale * 0.30f;
            backQuad.materialType = gfx::QUAD_MATERIAL_PROCEDURAL_GRASS;
            quads.push_back(backQuad);

            if (quads.size() >= MAX_GRASS_QUADS)
            {
                break;
            }

            gfx::TexturedQuad frontQuad = buildGrassQuad(
                startX + tileStep * static_cast<float>(i) + xJitter,
                1.0f,
                tileWidth * (widthScale * (0.97f + randomA * 0.03f)),
                tileHeight * heightScale);
            frontQuad.uv = {0.0f, 1.0f, 0.0f, 1.0f};
            frontQuad.alpha = alpha;
            frontQuad.windWeight = 1.0f;
            frontQuad.windPhase = clumpIndex * 0.61f + randomA * 5.0f + randomC * 1.7f;
            frontQuad.windResponse = 0.74f + heightScale * 0.38f + randomB * 0.08f;
            frontQuad.materialType = gfx::QUAD_MATERIAL_PROCEDURAL_GRASS;
            quads.push_back(frontQuad);
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

        const float winW = static_cast<float>(mWindow.getWidth());
        const float winH = static_cast<float>(mWindow.getHeight());
        if (winW < 1.0f || winH < 1.0f)
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

        const float winAspect = winW / winH;
        const float logicalHalfWidth = HUNTER_SCREEN_HALF_HEIGHT *
                                       (static_cast<float>(frame->sourceWidth) /
                                        static_cast<float>(frame->sourceHeight)) /
                                       winAspect;
        const float clampMin = -1.0f + logicalHalfWidth;
        const float clampMax = 1.0f - logicalHalfWidth;
        if (clampMin < clampMax)
        {
            mHunterX = std::clamp(mHunterX, clampMin, clampMax);
        }

        gfx::TexturedQuad quad = buildHunterQuad(*frame, mHunterX, winAspect);
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

        const float winW = static_cast<float>(mWindow.getWidth());
        const float winH = static_cast<float>(mWindow.getHeight());
        const game::AtlasFrame* frame = mSpriteAnim.getFrame(mHunterState.currentFrameIndex);
        if (walking && winW >= 1.0f && winH >= 1.0f &&
            frame != nullptr && frame->sourceWidth > 0 && frame->sourceHeight > 0)
        {
            const float winAspect = winW / winH;
            const float logicalHalfWidth = HUNTER_SCREEN_HALF_HEIGHT *
                                           (static_cast<float>(frame->sourceWidth) /
                                            static_cast<float>(frame->sourceHeight)) /
                                           winAspect;
            const float hunterWidth = logicalHalfWidth * 2.0f;
            const float gait = std::sin(mHunterState.elapsed * 9.5f);
            const float leftTargetPressure = std::max(0.0f, -gait);
            const float rightTargetPressure = std::max(0.0f, gait);
            const float leftFootTargetX = mHunterX - hunterWidth * 0.14f - gait * hunterWidth * 0.07f;
            const float rightFootTargetX = mHunterX + hunterWidth * 0.14f - gait * hunterWidth * 0.07f;
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
