#pragma once

#include "Window.h"
#include "Input.h"
#include "Timer.h"
#include "../audio/SoundEffectPlayer.h"
#include "../game/NatureSystem.h"
#include "../ui/Localization.h"
#include "../gfx/VulkanContext.h"
#include "../game/SpriteAnimation.h"

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

    /// تحديث الحركة الأساسية وطبقات الإطلاق الخاصة بالصياد في كل إطار
    void updateHunterMotion(float deltaTime);

    /// تحديث حالة الغيوم والأشجار والرياح بعيدًا عن كود الرسم
    void updateNatureSystem(float deltaTime);

    /// تحديث صف العشب المتحرك أسفل الشاشة
    void updateGrassRenderData();

    /// تحديث شريط التربة الثابت أسفل الشاشة
    void updateSoilRenderData();

    /// تحديث طبقات عرض الصياد: الحركة الأساسية ثم طبقات الإطلاق فوقها عند الحاجة
    void updateHunterRenderData();

    /// إرسال دفعات البيئة إلى Vulkan بعد انتهاء التحديث المنطقي
    void updateNatureRenderData();

    /// تحديث أثر القدمين على العشب مع بقاء الأثر لفترة قصيرة
    void updateGroundInteraction(float deltaTime);

    /// رسم الإطار
    void render();

    /// تنظيف الموارد عند الإغلاق
    void cleanup();

    /// كشف لغة النظام تلقائيًا (macOS: CFLocale، غير ذلك: متغير LANG)
    static ui::Localization::Language detectSystemLanguage();

    // مكونات التطبيق الأساسية
    Window mWindow;
    Input mInput;
    Timer mTimer;
    ui::Localization mLocalization;
    gfx::VulkanContext mVulkan;
    audio::SoundEffectPlayer mHunterShotSound;
    game::NatureSystem mNatureSystem;
    gfx::EnvironmentRenderData mEnvironmentRenderData;

    // أطلس الحركة الأساسية للصياد، وأطلسا الإطلاق العادي والعالي فوقها.
    game::SpriteAnimation mSpriteAnim;
    game::SpriteAnimation mHunterShootAnim;
    game::SpriteAnimation mHunterHighShootAnim;
    game::AnimationState mHunterState;
    game::AnimationState mHunterShootState;
    game::AnimationState mHunterHighShootState;
    std::string mAssetsPath;

    // معرّفات الطبقات داخل Vulkan مرتبة من الخلف إلى الأمام.
    gfx::VulkanContext::LayerId mGrassLayerId = gfx::VulkanContext::INVALID_LAYER_ID;
    gfx::VulkanContext::LayerId mHunterLayerId = gfx::VulkanContext::INVALID_LAYER_ID;
    gfx::VulkanContext::LayerId mHunterShootLayerId = gfx::VulkanContext::INVALID_LAYER_ID;
    gfx::VulkanContext::LayerId mHunterHighShootLayerId = gfx::VulkanContext::INVALID_LAYER_ID;
    gfx::VulkanContext::LayerId mSoilLayerId = gfx::VulkanContext::INVALID_LAYER_ID;

    // كاش بسيط لتخطيط العشب حتى لا نعيد بناء نفس الرقع دون داع.
    uint32_t mGrassLayoutWidth = 0;
    uint32_t mGrassLayoutHeight = 0;
    uint32_t mSoilLayoutWidth = 0;
    uint32_t mSoilLayoutHeight = 0;

    // بيانات تفاعل القدمين مع الأرض.
    float mLeftGroundX = 0.0f;
    float mRightGroundX = 0.0f;
    float mLeftGroundPressure = 0.0f;
    float mRightGroundPressure = 0.0f;
    float mGroundFootRadius = 0.06f;

    // حالة الصياد الحالية.
    float mHunterX = 0.0f;       // موقع الصياد الأفقي (-1 إلى 1)
    float mHunterSpeed = 0.8f;   // سرعة الحركة (وحدات/ثانية)
    float mReloadTimer = 0.0f;
    float mHunterShootTransitionTimer = 0.0f; // مؤقت الانتقال بين فريمات ما بعد الإطلاق
    bool mHunterShootActive = false;
    bool mHunterHighShootActive = false;
    bool mHunterShootAutoHolster = false;
    bool mIsReloading = false;
    bool mRunning = false;
    bool mInitialized = false;
};

} // namespace core
