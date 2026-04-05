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
    /// مرحلة حركة الصياد الحالية
    enum class HunterPhase : unsigned char
    {
        Locomotion,        // مشي أو وقوف
        Shoot,             // تسلسل الإطلاق العادي
        ShootHigh,         // تسلسل الإطلاق العالي
    };

    /// تهيئة الأنظمة الفرعية
    void init();

    /// تحديث المنطق كل إطار
    void update(float deltaTime);

    /// تحديث حركة الصياد في كل إطار
    void updateHunterMotion(float deltaTime);

    /// تحديث حالة الغيوم والأشجار والرياح
    void updateNatureSystem(float deltaTime);

    /// تحديث صف العشب المتحرك
    void updateGrassRenderData();

    /// تحديث شريط التربة
    void updateSoilRenderData();

    /// تحديث طبقة عرض الصياد
    void updateHunterRenderData();

    /// إرسال دفعات البيئة إلى Vulkan
    void updateNatureRenderData();

    /// تحديث أثر القدمين على العشب
    void updateGroundInteraction(float deltaTime);

    /// رسم الإطار
    void render();

    /// تنظيف الموارد عند الإغلاق
    void cleanup();

    /// كشف لغة النظام تلقائيًا
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

    // أطلس الصياد الموحد وحالته
    game::SpriteAnimation mSpriteAnim;
    game::AnimationState mHunterState;
    HunterPhase mHunterPhase = HunterPhase::Locomotion;
    std::string mAssetsPath;

    // معرّفات الطبقات داخل Vulkan
    gfx::VulkanContext::LayerId mGrassLayerId = gfx::VulkanContext::INVALID_LAYER_ID;
    gfx::VulkanContext::LayerId mHunterLayerId = gfx::VulkanContext::INVALID_LAYER_ID;
    gfx::VulkanContext::LayerId mSoilLayerId = gfx::VulkanContext::INVALID_LAYER_ID;

    // كاش تخطيط العشب
    uint32_t mGrassLayoutWidth = 0;
    uint32_t mGrassLayoutHeight = 0;
    uint32_t mSoilLayoutWidth = 0;
    uint32_t mSoilLayoutHeight = 0;

    // بيانات تفاعل القدمين مع الأرض
    float mLeftGroundX = 0.0f;
    float mRightGroundX = 0.0f;
    float mLeftGroundPressure = 0.0f;
    float mRightGroundPressure = 0.0f;
    float mGroundFootRadius = 0.06f;

    // حالة الصياد الحالية
    float mHunterX = 0.0f;
    float mHunterSpeed = 0.8f;
    float mReloadTimer = 0.0f;
    bool mIsReloading = false;
    bool mRunning = false;
    bool mInitialized = false;
};

} // namespace core
