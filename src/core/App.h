#pragma once

#include "Window.h"
#include "Input.h"
#include "Timer.h"
#include "../audio/SoundEffectPlayer.h"
#include "../game/DuckSpriteAtlas.h"
#include "../game/NatureSystem.h"
#include "../gfx/VulkanContext.h"
#include "../game/SpriteAnimation.h"

#include <random>
#include <string>

namespace core
{

/*
 حلقة التطبيق الرئيسية - تنسق بين النافذة والإدخال واللعبة
 */
class App
{
  public:
#pragma region PublicTypes
    /*
     إعدادات التطبيق
     */
    struct Config
    {
        /*
         إعدادات نافذة التطبيق.
         */
        Window::Config window; // إعدادات النافذة

        /*
         مسار مجلد الأصول الأساسي.
         */
        std::string assetsPath; // مسار مجلد الأصول

        Config() : assetsPath("assets")
        {
        }
    };
#pragma endregion PublicTypes

    /*
     ينشئ التطبيق ويبدأ تهيئة أنظمته.
     */
    explicit App(const Config& config = {});

    /*
     يحرر الموارد المملوكة للتطبيق.
     */
    ~App();

    // منع النسخ
    App(const App&) = delete;
    App& operator=(const App&) = delete;

#pragma region PublicLifecycle
    /*
     بدء حلقة التطبيق (تستمر حتى إغلاق النافذة)
     */
    void run();

    /*
     طلب إيقاف التطبيق
     */
    void requestShutdown();

    /*
     هل التطبيق يعمل؟
     */
    [[nodiscard]] bool isRunning() const;

    /*
     الوصول لمكونات التطبيق
     */
    [[nodiscard]] Window& getWindow();
    [[nodiscard]] const Window& getWindow() const;
    [[nodiscard]] Input& getInput();
    [[nodiscard]] const Input& getInput() const;
    [[nodiscard]] Timer& getTimer();
    [[nodiscard]] const Timer& getTimer() const;

#pragma endregion PublicLifecycle

  private:
#pragma region PrivateTypes
    /*
     مرحلة حركة الصياد الحالية
     */
    enum class HunterPhase : unsigned char
    {
        Locomotion, // مشي أو وقوف
        Shoot,      // تسلسل الإطلاق العادي
        ShootHigh,  // تسلسل الإطلاق العالي
    };

    /*
     حالة البطة الحالية
     */
    enum class DuckPhase : unsigned char
    {
        Flying,   // تطير في السماء
        Falling,  // أُصيبت وتسقط
        Grounded, // وصلت الأرض وتنتظر الاختفاء
    };
#pragma endregion PrivateTypes

#pragma region PrivateLifecycle
    /*
     تهيئة الأنظمة الفرعية
     */
    void init();

    /*
     تحديث المنطق كل إطار
     */
    void update(float deltaTime);

    /*
     تحديث حركة الصياد في كل إطار
     */
    void updateHunterMotion(float deltaTime);

    /*
     تحديث حركة البطة الاستعراضية في كل إطار
     */
    void updateDuckMotion(float deltaTime);

    /*
     يبدأ رحلة بطة جديدة بموضع واتجاه عشوائيين داخل النطاق المسموح
     */
    void resetDuckFlight();

    /*
     يحاول إصابة البطة بناءً على موضع المؤشر الحالي
     */
    [[nodiscard]] bool tryHitDuck(float cursorX, float cursorY);

    /*
     تحديث حالة الغيوم والأشجار والرياح
     */
    void updateNatureSystem(float deltaTime);

    /*
     تحديث صف العشب المتحرك
     */
    void updateGrassRenderData();

    /*
     تحديث شريط التربة
     */
    void updateSoilRenderData();

    /*
     تحديث طبقة عرض الصياد
     */
    void updateHunterRenderData();

    /*
     تحديث طبقة عرض البطة
     */
    void updateDuckRenderData();

    /*
     إرسال دفعات البيئة إلى Vulkan
     */
    void updateNatureRenderData();

    /*
     تحديث أثر القدمين على العشب
     */
    void updateGroundInteraction(float deltaTime);

    /*
     رسم الإطار
     */
    void render();

    /*
     تنظيف الموارد عند الإغلاق
     */
    void cleanup();

    /*
     كشف لغة النظام تلقائيًا
     */

#pragma endregion PrivateLifecycle

#pragma region CoreComponents
    // مكونات التطبيق الأساسية

    /*
     مدير النافذة الرئيسية.
     */
    Window mWindow;

    /*
     مدير الإدخال.
     */
    Input mInput;

    /*
     مؤقت الإطارات.
     */
    Timer mTimer;

    /*
     مخزن الترجمة الحالي.
     */

    /*
     واجهة الرسم القائمة على Vulkan.
     */
    gfx::VulkanContext mVulkan;

    /*
     صوت طلقة الصياد.
     */
    audio::SoundEffectPlayer mHunterShotSound;

    /*
     صوت وجود/تحليق البطة.
     */
    audio::SoundEffectPlayer mDuckAmbientSound;

    /*
     نظام البيئة الإجرائية.
     */
    game::NatureSystem mNatureSystem;

    /*
     دفعات البيئة الجاهزة للرسم.
     */
    gfx::EnvironmentRenderData mEnvironmentRenderData;
#pragma endregion CoreComponents

#pragma region AnimationState
    // أطلس الصياد الموحد وحالته

    /*
     مشغل أنيميشن الصياد.
     */
    game::SpriteAnimation mSpriteAnim;

    /*
     حالة أنيميشن الصياد الحالية.
     */
    game::AnimationState mHunterState;

    /*
     مشغل أنيميشن البطة.
     */
    game::SpriteAnimation mDuckAnim;

    /*
     حالة أنيميشن البطة الحالية.
     */
    game::AnimationState mDuckState;

    /*
     المرحلة الحالية لمنطق الصياد.
     */
    HunterPhase mHunterPhase = HunterPhase::Locomotion;

    /*
     المرحلة الحالية لمنطق البطة.
     */
    DuckPhase mDuckPhase = DuckPhase::Flying;

    /*
     مسار مجلد الأصول الجاري استخدامه.
     */
    std::string mAssetsPath;
#pragma endregion AnimationState

#pragma region RenderLayerIds
    // معرّفات الطبقات داخل Vulkan

    /*
     معرّف طبقة العشب.
     */
    gfx::VulkanContext::LayerId mGrassLayerId = gfx::VulkanContext::INVALID_LAYER_ID;

    /*
     معرّف طبقة الصياد.
     */
    gfx::VulkanContext::LayerId mHunterLayerId = gfx::VulkanContext::INVALID_LAYER_ID;

    /*
     معرّف طبقة البطة.
     */
    gfx::VulkanContext::LayerId mDuckLayerId = gfx::VulkanContext::INVALID_LAYER_ID;

    /*
     معرّف طبقة التربة.
     */
    gfx::VulkanContext::LayerId mSoilLayerId = gfx::VulkanContext::INVALID_LAYER_ID;
#pragma endregion RenderLayerIds

#pragma region LayoutCache
    // كاش تخطيط العشب

    /*
     آخر عرض محسوب لتخطيط العشب.
     */
    uint32_t mGrassLayoutWidth = 0;

    /*
     آخر ارتفاع محسوب لتخطيط العشب.
     */
    uint32_t mGrassLayoutHeight = 0;

    /*
     آخر عرض محسوب لتخطيط التربة.
     */
    uint32_t mSoilLayoutWidth = 0;

    /*
     آخر ارتفاع محسوب لتخطيط التربة.
     */
    uint32_t mSoilLayoutHeight = 0;
#pragma endregion LayoutCache

#pragma region GroundInteractionState
    // بيانات تفاعل القدمين مع الأرض

    /*
     موضع القدم اليسرى على الأرض.
     */
    float mLeftGroundX = 0.0f;

    /*
     موضع القدم اليمنى على الأرض.
     */
    float mRightGroundX = 0.0f;

    /*
     شدة ضغط القدم اليسرى على العشب/الأرض.
     */
    float mLeftGroundPressure = 0.0f;

    /*
     شدة ضغط القدم اليمنى على العشب/الأرض.
     */
    float mRightGroundPressure = 0.0f;

    /*
     نصف قطر تأثير القدم على الأرض.
     */
    float mGroundFootRadius = 0.06f;
#pragma endregion GroundInteractionState

#pragma region HunterAndDuckState
    // حالة الصياد الحالية

    /*
     موضع الصياد الأفقي على الشاشة.
     */
    float mHunterX = 0.0f;

    /*
     سرعة حركة الصياد الأفقية.
     */
    float mHunterSpeed = 0.8f;

    /*
     مؤقت إعادة التلقيم اليدوي.
     */
    float mReloadTimer = 0.0f;

    /*
     مؤقت تثبيت فريم الإطلاق بعد انتهاء الطلقة.
     */
    float mHunterShotRecoveryTimer = 0.0f;

    /*
     موضع البطة الأفقي.
     */
    float mDuckX = -1.15f;

    /*
     موضع البطة العمودي.
     */
    float mDuckY = -0.34f;

    /*
     الزمن التراكمي داخل رحلة البطة الحالية.
     */
    float mDuckFlightTime = 0.0f;

    /*
     مدة رحلة البطة الحالية.
     */
    float mDuckFlightDuration = 12.4f;

    /*
     موضع البداية الأفقي للرحلة.
     */
    float mDuckFlightStartX = -1.22f;

    /*
     موضع النهاية الأفقي للرحلة.
     */
    float mDuckFlightEndX = 1.22f;

    /*
     موضع البداية العمودي للرحلة.
     */
    float mDuckFlightStartY = -0.48f;

    /*
     موضع النهاية العمودي للرحلة.
     */
    float mDuckFlightEndY = -0.52f;

    /*
     نقطة التحكم الأولى لمسار البطة.
     */
    float mDuckFlightControlY1 = -0.56f;

    /*
     نقطة التحكم الثانية لمسار البطة.
     */
    float mDuckFlightControlY2 = -0.50f;

    /*
     ارتفاع قوس الرحلة الحالي.
     */
    float mDuckFlightArcHeight = 0.06f;

    /*
     عدد رفرفات الجناح داخل الرحلة.
     */
    float mDuckFlightFlapCycles = 2.8f;

    /*
     طور الاهتزاز الجانبي الخفيف في الرحلة.
     */
    float mDuckFlightSwayPhase = 0.0f;

    /*
     سرعة البطة الأفقية أثناء السقوط.
     */
    float mDuckVelocityX = 0.0f;

    /*
     سرعة البطة العمودية أثناء السقوط.
     */
    float mDuckVelocityY = 0.0f;

    /*
     زاوية البطة الحالية بالراديان.
     */
    float mDuckRotation = 0.0f;

    /*
     سرعة دوران البطة أثناء السقوط.
     */
    float mDuckAngularVelocity = 0.0f;

    /*
     مؤقت بقاء البطة على الأرض.
     */
    float mDuckGroundTimer = 0.0f;

    /*
     شفافية البطة الحالية أثناء الاختفاء.
     */
    float mDuckAlpha = 1.0f;

    /*
     اتجاه الصياد الحالي.
     */
    bool mFacingLeft = false;

    /*
     اتجاه البطة الحالي.
     */
    bool mDuckFacingLeft = false;

    /*
     هل يجب قلب البطة عمودياً في الرسم؟
     */
    bool mDuckFlipY = false;

    /*
     هل الصياد في حالة إعادة تلقيم؟
     */
    bool mIsReloading = false;

    /*
     هل الحلقة الرئيسية تعمل حالياً؟
     */
    bool mRunning = false;

    /*
     هل تمت التهيئة بنجاح؟
     */
    bool mInitialized = false;

    /*
     مولّد العشوائية الخاص بمسارات البطة.
     */
    std::mt19937 mDuckRandomEngine;
#pragma endregion HunterAndDuckState
};

} // namespace core
