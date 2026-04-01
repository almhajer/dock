#include "App.h"
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <iostream>
#include <locale>

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif

namespace core
{

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

        // بناء أطلس حركة الصياد وتحميل النسيج من نفس البكسلات
        const std::string spritePath = mAssetsPath + "/sprite/sprite.png";
        mVulkan.loadTextureWithCallback(spritePath,
            [this](int w, int h, const unsigned char* pixels) {
                mSpriteAnim.buildHunterAtlas(w, h, pixels);
            });

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
        bool moving = false;
        std::string lastDirection = mHunterState.flipX ? "left" : "right";

        // تحريك الصياد عند الضغط المستمر على الأسهم
        if (mInput.isKeyPressed(GLFW_KEY_RIGHT))
        {
            mHunterX += mHunterSpeed * deltaTime;
            moving = true;
            if (mHunterState.currentClip != "walk_right" || mHunterState.finished)
                mSpriteAnim.play(mHunterState, "walk_right");
        }
        else if (mInput.isKeyPressed(GLFW_KEY_LEFT))
        {
            mHunterX -= mHunterSpeed * deltaTime;
            moving = true;
            if (mHunterState.currentClip != "walk_left" || mHunterState.finished)
                mSpriteAnim.play(mHunterState, "walk_left");
        }

        // عند عدم الحركة ننتقل لحالة الوقوف
        if (!moving)
        {
            std::string idleClip = (lastDirection == "right") ? "idle_right" : "idle_left";
            if (mHunterState.currentClip != idleClip)
                mSpriteAnim.play(mHunterState, idleClip);
        }

        // تحديث الحركة
        mSpriteAnim.update(mHunterState, deltaTime);

        // تحديث الـ quad: UV + موقع الصياد
        if (!mHunterState.currentClip.empty())
        {
            float u0, u1, v0, v1;
            mSpriteAnim.getFrameUV(mHunterState.currentFrameIndex, u0, u1, v0, v1);

            // عكس أفقي إذا كان الاتجاه يسار
            if (mHunterState.flipX)
                std::swap(u0, u1);

            // أبعاد النافذة
            float winW = static_cast<float>(mWindow.getWidth());
            float winH = static_cast<float>(mWindow.getHeight());

            if (winW < 1.0f || winH < 1.0f)
                return;

            float winAspect = winW / winH;

            // نستخدم أبعاد الخلية المنطقية الثابتة من الـ atlas حتى لا يتذبذب الحجم بين الفريمات المقصوصة.
            const auto& atlasData = mSpriteAnim.getData();
            const auto& curFrame = atlasData.frames[mHunterState.currentFrameIndex];
            float frameAspect = static_cast<float>(curFrame.sourceWidth) /
                                static_cast<float>(curFrame.sourceHeight);

            // الارتفاع النسبي للصياد داخل النافذة
            float scaleY = 0.5f;

            // العرض المنطقي يُحسب من خلية atlas الثابتة، وليس من الفريم المقصوص.
            float scaleX = scaleY * frameAspect / winAspect;

            // الهامش السفلي: نسبة ثابتة (4% من الارتفاع)
            float bottomMargin = 0.04f;
            float posY = 1.0f - bottomMargin * 2.0f - scaleY;

            // حدود أفقي: منع الصياد من الخروج من الشاشة
            float clampMin = -1.0f + scaleX;
            float clampMax = 1.0f - scaleX;
            if (clampMin < clampMax)
                mHunterX = std::clamp(mHunterX, clampMin, clampMax);

            const float logicalX0 = mHunterX - scaleX;
            const float logicalY0 = posY - scaleY;
            const float logicalWidth = scaleX * 2.0f;
            const float logicalHeight = scaleY * 2.0f;

            const float x0 = logicalX0 + logicalWidth *
                (static_cast<float>(curFrame.offsetX) / static_cast<float>(curFrame.sourceWidth));
            const float x1 = x0 + logicalWidth *
                (static_cast<float>(curFrame.width) / static_cast<float>(curFrame.sourceWidth));
            const float y0 = logicalY0 + logicalHeight *
                (static_cast<float>(curFrame.offsetY) / static_cast<float>(curFrame.sourceHeight));
            const float y1 = y0 + logicalHeight *
                (static_cast<float>(curFrame.height) / static_cast<float>(curFrame.sourceHeight));

            mVulkan.updateSpriteQuad(u0, u1, v0, v1, x0, x1, y0, y1);
        }
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
