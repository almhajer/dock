#include "App.h"

#include <cstdlib>
#include <cstring>
#include <locale>

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif

namespace core {

App::App(const Config& config)
    : mWindow(config.window)
    , mAssetsPath(config.assetsPath) {
    init();
}

App::~App() {
    cleanup();
}

void App::init() {
    if (mInitialized) return;

    // تهيئة نظام الإدخال وربطه بالنافذة
    mInput.init(mWindow.getHandle());

    // كشف لغة النظام وتحميل الترجمة المناسبة
    ui::Localization::Language sysLang = detectSystemLanguage();
    if (!mLocalization.load(mAssetsPath, sysLang)) {
        // إذا فشل تحميل لغة النظام، نحاول الإنجليزية كاحتياطي
        mLocalization.load(mAssetsPath, ui::Localization::Language::English);
    }

    mInitialized = true;
    mRunning = true;
}

ui::Localization::Language App::detectSystemLanguage() {
#ifdef __APPLE__
    // على macOS نستخدم CFLocale لمعرفة لغة النظام بدقة
    CFArrayRef languages = CFLocaleCopyPreferredLanguages();
    if (languages) {
        const CFStringRef primaryLang = static_cast<CFStringRef>(CFArrayGetValueAtIndex(languages, 0));
        if (primaryLang) {
            char buffer[16] = {};
            CFStringGetCString(primaryLang, buffer, sizeof(buffer), kCFStringEncodingUTF8);

            CFRelease(languages);

            // الكشف عن العربية: "ar" أو "ar-" (مثل "ar-SA", "ar-EG")
            if (buffer[0] == 'a' && buffer[1] == 'r') {
                return ui::Localization::Language::Arabic;
            }
        } else {
            CFRelease(languages);
        }
    }
#else
    // على المنصات الأخرى: فحص متغير البيئة LANG
    const char* lang = std::getenv("LANG");
    if (lang && std::strncmp(lang, "ar", 2) == 0) {
        return ui::Localization::Language::Arabic;
    }
#endif

    // الافتراضي: إنجليزي
    return ui::Localization::Language::English;
}

void App::run() {
    if (!mInitialized) {
        throw std::runtime_error("لم تتم تهيئة التطبيق قبل استدعاء run()");
    }

    // حلقة التطبيق الرئيسية
    while (mRunning && mWindow.isOpen()) {
        mTimer.update();

        // معالجة أحداث النافذة (إدخال، تغيير حجم...)
        mWindow.pollEvents();
        mInput.update();

        // تحديث منطق اللعبة
        update(mTimer.getDeltaTime());

        // رسم الإطار
        render();
    }

    mRunning = false;
}

void App::requestShutdown() {
    mRunning = false;
}

bool App::isRunning() const {
    return mRunning;
}

// ─── الوصول للمكونات ───────────────────────────────────────────

Window& App::getWindow() { return mWindow; }
const Window& App::getWindow() const { return mWindow; }
Input& App::getInput() { return mInput; }
const Input& App::getInput() const { return mInput; }
Timer& App::getTimer() { return mTimer; }
const Timer& App::getTimer() const { return mTimer; }
ui::Localization& App::getLocalization() { return mLocalization; }
const ui::Localization& App::getLocalization() const { return mLocalization; }

// ─── الطرق الخاصة ──────────────────────────────────────────────

void App::update(float deltaTime) {
    // TODO: تحديث منطق اللعبة (سيُربط لاحقًا بـ game::Game)
    (void)deltaTime;
}

void App::render() {
    // TODO: رسم الإطار عبر Vulkan (سيُربط لاحقًا بـ gfx::VulkanContext)
}

void App::cleanup() {
    if (!mInitialized) return;

    // تنظيف الموارد - النافذة تنظّف نفسها عبر المدمر
    mInitialized = false;
}

} // namespace core
