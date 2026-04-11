#pragma once

#include "../core/SceneLayout.h"
#include "../gfx/EnvironmentAtlas.h"
#include "../gfx/EnvironmentTypes.h"

#include <cstdint>
#include <vector>

namespace game
{

/*
 @brief بيانات الحقل الهوائي المؤثر على عناصر البيئة.
 */
struct WindField
{
    /*
     @brief الزمن التراكمي للنظام الهوائي.
     */
    float time = 0.0f;
    /*
     @brief اتجاه الرياح العام.
     */
    float direction = 1.0f;
    /*
     @brief السرعة الأساسية للرياح.
     */
    float baseSpeed = 0.12f;
    /*
     @brief قوة الانحراف البطيء.
     */
    float driftStrength = 0.04f;
    /*
     @brief شدة الهبات اللحظية.
     */
    float gustStrength = 0.0f;
};

/*
 @brief حالة غيمة واحدة داخل البيئة.
 */
struct CloudState
{
    /*
     @brief موضع الغيمة الأفقي.
     */
    float x = 0.0f;
    /*
     @brief موضع الغيمة العمودي.
     */
    float y = 0.0f;
    /*
     @brief عرض الغيمة.
     */
    float width = 0.0f;
    /*
     @brief ارتفاع الغيمة.
     */
    float height = 0.0f;
    /*
     @brief سرعة الغيمة.
     */
    float speed = 0.0f;
    /*
     @brief اتجاه حركة الغيمة.
     */
    float heading = 1.0f;
    /*
     @brief طور التذبذب العمودي.
     */
    float verticalPhase = 0.0f;
    /*
     @brief شدة الانجراف العمودي.
     */
    float verticalDrift = 0.0f;
    /*
     @brief معامل العمق البصري.
     */
    float parallax = 1.0f;
    /*
     @brief شفافية الغيمة.
     */
    float alpha = 1.0f;
    /*
     @brief شدة الصبغة اللونية.
     */
    float tint = 1.0f;
    /*
     @brief نوع السبرايت المستخدم.
     */
    gfx::EnvironmentSpriteId sprite = gfx::EnvironmentSpriteId::CloudWide;
};

/*
 @brief حالة شجرة واحدة داخل البيئة.
 */
struct TreeState
{
    /*
     @brief موضع الشجرة الأفقي.
     */
    float x = 0.0f;
    /*
     @brief موضع قاعدة الشجرة.
     */
    float baseY = 0.0f;
    /*
     @brief عرض الشجرة.
     */
    float width = 0.0f;
    /*
     @brief ارتفاع الشجرة.
     */
    float height = 0.0f;
    /*
     @brief طور التمايل.
     */
    float phase = 0.0f;
    /*
     @brief سعة التمايل.
     */
    float swayAmount = 0.0f;
    /*
     @brief معامل العمق البصري.
     */
    float parallax = 1.0f;
    /*
     @brief شفافية الشجرة.
     */
    float alpha = 1.0f;
    /*
     @brief شدة الصبغة اللونية.
     */
    float tint = 1.0f;
    /*
     @brief هل الشجرة في المقدمة؟
     */
    bool foreground = false;
    /*
     @brief نوع السبرايت المستخدم.
     */
    gfx::EnvironmentSpriteId sprite = gfx::EnvironmentSpriteId::TreeRound;
};

/*
 نظام البيئة المسؤول عن تحديث الغيوم والأشجار وبناء دفعات الرسم فقط.
 */
class NatureSystem
{
  public:
#pragma region PublicInterface
    /*
     @brief يهيئ توزيع عناصر البيئة لأول مرة.
     @param metrics قياسات النافذة المنطقية.
     */
    void initialize(const core::scene::WindowMetrics& metrics);

    /*
     @brief يحدث حالة البيئة لكل إطار.
     @param deltaTime الزمن المنقضي منذ آخر إطار.
     @param metrics قياسات النافذة المنطقية.
     */
    void update(float deltaTime, const core::scene::WindowMetrics& metrics);

    /*
     @brief يبني دفعات الرسم الجاهزة من حالة البيئة الحالية.
     @param metrics قياسات النافذة المنطقية.
     @param renderData مرجع حاوية دفعات الرسم المخرجة.
     */
    void buildRenderData(const core::scene::WindowMetrics& metrics, gfx::EnvironmentRenderData& renderData) const;
#pragma endregion PublicInterface

  private:
#pragma region InternalHelpers
    /*
     @brief يعيد بناء توزيع البيئة عند تغير أبعاد المشهد.
     @param metrics قياسات النافذة المنطقية.
     */
    void rebuildLayout(const core::scene::WindowMetrics& metrics);

    /*
     @brief يعيد بناء كاش الرسم الخاص بالأشجار.
     */
    void rebuildTreeRenderCaches();

    /*
     @brief يحدث الغيوم وحركتها عبر الزمن.
     @param deltaTime الزمن المنقضي منذ آخر إطار.
     @param metrics قياسات النافذة المنطقية.
     */
    void updateClouds(float deltaTime, const core::scene::WindowMetrics& metrics);

    /*
     @brief يعيد تدوير غيمة خرجت من المشهد إلى طرفه المقابل.
     @param cloud مرجع حالة الغيمة المراد تدويرها.
     @param metrics قياسات النافذة المنطقية.
     @param wrapToLeft هل يجب التفاف الغيمة إلى الطرف الأيسر؟
     @param randomSeed بذرة عشوائية لإعادة توليد خصائص الغيمة.
     */
    void recycleCloud(CloudState& cloud, const core::scene::WindowMetrics& metrics, bool wrapToLeft,
                      float randomSeed) const;

    /*
     @brief دالة هاش بسيطة لإنتاج عشوائية حتمية.
     @param value القيمة المدخلة.
     @return قيمة عشوائية بين 0 و 1.
     */
    [[nodiscard]] static float hash01(float value);
#pragma endregion InternalHelpers

#pragma region InternalState
    /*
     @brief هل تمت تهيئة البيئة؟
     */
    bool mInitialized = false;

    /*
     @brief عرض التخطيط الحالي.
     */
    uint32_t mLayoutWidth = 0;

    /*
     @brief ارتفاع التخطيط الحالي.
     */
    uint32_t mLayoutHeight = 0;

    /*
     @brief مؤشر التدوير الدوري للغيوم.
     */
    float mRecycleCursor = 0.0f;

    /*
     @brief بيانات الحقل الهوائي الحالي.
     */
    WindField mWind;

    /*
     @brief جميع الغيوم الحية.
     */
    std::vector<CloudState> mClouds;

    /*
     @brief جميع الأشجار الحية.
     */
    std::vector<TreeState> mTrees;

    /*
     @brief كاش دفعة الأشجار الخلفية.
     */
    std::vector<gfx::EnvironmentInstance> mBackgroundTreeRenderCache;

    /*
     @brief كاش دفعة الأشجار الأمامية.
     */
    std::vector<gfx::EnvironmentInstance> mForegroundTreeRenderCache;
#pragma endregion InternalState
};

} // namespace game
