#pragma once

#include "Window.h"
#include "Input.h"
#include "Timer.h"
#include "../ui/Localization.h"
#include "../gfx/VulkanContext.h"
#include "../game/SpriteAnimation.h"

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

    /// تحديث منطق حركة الصياد واختيار المقطع المناسب
    void updateHunterMotion(float deltaTime);

    /// تحديث صف العشب المتحرك أسفل الشاشة
    void updateGrassRenderData();

    /// تحديث شريط التربة الثابت أسفل الشاشة
    void updateSoilRenderData();

    /// تحديث الـ quad الخاصة بالصياد للعرض الحالي
    void updateHunterRenderData();

    /// تحديث أثر القدمين على العشب مع بقاء الأثر لفترة قصيرة
    void updateGroundInteraction(float deltaTime);

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
    gfx::VulkanContext mVulkan;
    game::SpriteAnimation mSpriteAnim;
    game::AnimationState mHunterState;
    std::string mAssetsPath;
    gfx::VulkanContext::LayerId mGrassLayerId = gfx::VulkanContext::INVALID_LAYER_ID;
    gfx::VulkanContext::LayerId mHunterLayerId = gfx::VulkanContext::INVALID_LAYER_ID;
    gfx::VulkanContext::LayerId mSoilLayerId = gfx::VulkanContext::INVALID_LAYER_ID;
    uint32_t mGrassLayoutWidth = 0;
    uint32_t mGrassLayoutHeight = 0;
    float mLeftGroundX = 0.0f;
    float mRightGroundX = 0.0f;
    float mLeftGroundPressure = 0.0f;
    float mRightGroundPressure = 0.0f;
    float mGroundFootRadius = 0.06f;
    float mHunterX = 0.0f;       // موقع الصياد الأفقي (-1 إلى 1)
    float mHunterSpeed = 0.8f;   // سرعة الحركة (وحدات/ثانية)
    bool mRunning = false;
    bool mInitialized = false;
};

} // namespace core
