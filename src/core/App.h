#pragma once

#include "Window.h"
#include "Input.h"
#include "Timer.h"
#include "../ui/Localization.h"

#include <memory>
#include <string>

namespace core {

/// حلقة التطبيق الرئيسية - تنسق بين النافذة والإدخال واللعبة
class App {
public:
    /// إعدادات التطبيق
    struct Config {
        Window::Config window;             // إعدادات النافذة
        std::string assetsPath;            // مسار مجلد الأصول

        Config() : assetsPath("assets") {}
    };

    explicit App(const Config& config = {});
    ~App();

    // منع النسخ
    App(const App&) = delete;
    App& operator=(const App&) = delete;

    /// بدء حلقة التطبيق (تستمر حتى إغلاق النافذة)
    void run();

    /// طلب إيقاف التطبيق
    void requestShutdown();

    /// هل التطبيق يعمل؟
    [[nodiscard]] bool isRunning() const;

    /// الوصول لمكونات التطبيق
    [[nodiscard]] Window& getWindow();
    [[nodiscard]] const Window& getWindow() const;
    [[nodiscard]] Input& getInput();
    [[nodiscard]] const Input& getInput() const;
    [[nodiscard]] Timer& getTimer();
    [[nodiscard]] const Timer& getTimer() const;
    [[nodiscard]] ui::Localization& getLocalization();
    [[nodiscard]] const ui::Localization& getLocalization() const;

private:
    /// تهيئة الأنظمة الفرعية
    void init();

    /// تحديث المنطق كل إطار
    void update(float deltaTime);

    /// رسم الإطار
    void render();

    /// تنظيف الموارد عند الإغلاق
    void cleanup();

    /// كشف لغة النظام تلقائيًا (macOS: CFLocale، غير ذلك: متغير LANG)
    static ui::Localization::Language detectSystemLanguage();

    // مكونات التطبيق
    Window mWindow;
    Input mInput;
    Timer mTimer;
    ui::Localization mLocalization;
    std::string mAssetsPath;
    bool mRunning = false;
    bool mInitialized = false;
};

} // namespace core
