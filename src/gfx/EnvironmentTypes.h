#pragma once

#include "RenderTypes.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace gfx
{

#pragma region EnvironmentConstants
/*
 @brief عدد رؤوس quad القياسي المستخدم مع instancing.
 */
inline constexpr std::size_t ENVIRONMENT_BASE_VERTEX_COUNT = 6;

/*
 @brief نوع عنصر البيئة المرسوم في الشيدر.
 */
enum class EnvironmentElementKind : uint32_t
{
    Cloud = 0,
    Tree = 1,
};

/*
 @brief معرّف السبرايت داخل أطلس البيئة.
 */
enum class EnvironmentSpriteId : uint32_t
{
    CloudWide = 0,
    CloudTower = 1,
    CloudWisp = 2,
    TreeRound = 3,
    TreeTall = 4,
    TreeLean = 5,
};
#pragma endregion EnvironmentConstants

#pragma region EnvironmentTypes
/*
 هندسة quad ثابتة تُستخدم مع instancing لرسم عناصر البيئة.
 */
struct EnvironmentQuadVertex
{
    /*
     الإحداثي الأفقي للرأس.
     */
    float x = 0.0f;

    /*
     الإحداثي العمودي للرأس.
     */
    float y = 0.0f;

    /*
     إحداثي UV الأفقي.
     */
    float u = 0.0f;

    /*
     إحداثي UV العمودي.
     */
    float v = 0.0f;
};

/*
 بيانات instance واحدة لعناصر البيئة: غيمة أو شجرة.
 */
struct EnvironmentInstance
{
    /*
     مركز العنصر أفقياً.
     */
    float centerX = 0.0f;

    /*
     مركز العنصر عمودياً.
     */
    float centerY = 0.0f;

    /*
     نصف العرض.
     */
    float halfWidth = 0.0f;

    /*
     نصف الارتفاع.
     */
    float halfHeight = 0.0f;

    /*
     بداية UV الأفقية.
     */
    float u0 = 0.0f;

    /*
     بداية UV العمودية.
     */
    float v0 = 0.0f;

    /*
     نهاية UV الأفقية.
     */
    float u1 = 0.0f;

    /*
     نهاية UV العمودية.
     */
    float v1 = 0.0f;

    /*
     قناة الصبغة الحمراء.
     */
    float tintR = 1.0f;

    /*
     قناة الصبغة الخضراء.
     */
    float tintG = 1.0f;

    /*
     قناة الصبغة الزرقاء.
     */
    float tintB = 1.0f;

    /*
     شفافية العنصر.
     */
    float alpha = 1.0f;

    /*
     وزن تأثير الرياح.
     */
    float windWeight = 0.0f;

    /*
     طور الرياح الخاص بالعنصر.
     */
    float windPhase = 0.0f;

    /*
     استجابة العنصر للرياح.
     */
    float windResponse = 0.0f;

    /*
     معامل اختلاف العمق البصري.
     */
    float parallax = 1.0f;

    /*
     نوع العنصر داخل الشيدر.
     */
    float kind = 0.0f;

    /*
     نعومة حواف العنصر.
     */
    float softness = 0.0f;

    /*
     مقدار التمايل المرئي.
     */
    float swayAmount = 0.0f;

    /*
     حقل محجوز للتوسعات المستقبلية.
     */
    float reserved = 0.0f;
};

/*
 دفعات البيئة مفصولة حسب النوع لتفادي خلط مسار الغيم مع مسار الشجر داخل نفس الـ shader.
 */
struct EnvironmentRenderData
{
    /*
     دفعة الغيوم.
     */
    std::vector<EnvironmentInstance> cloudInstances;

    /*
     دفعة الأشجار الخلفية.
     */
    std::vector<EnvironmentInstance> backgroundTreeInstances;

    /*
     دفعة الأشجار الأمامية.
     */
    std::vector<EnvironmentInstance> foregroundTreeInstances;
};
#pragma endregion EnvironmentTypes

} // namespace gfx
