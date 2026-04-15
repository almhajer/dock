/**
 * @file HunterController.h
 * @brief واجهة تحكم الصياد.
 * @details منطق تحكم الصياد والتصويب.
 */

#pragma once

#include "../core/Input.h"
#include "../rendering/SceneLayout.h"
#include "../assets/SpriteAnimation.h"

struct GLFWwindow;

namespace core::hunterplay
{

#pragma region GameplayTypes
/*
 @brief بيانات موضع المؤشر بعد تحويله إلى مساحة الشاشة المنطقية المستخدمة داخل منطق التصويب.
 */
struct CursorScreenPosition
{
    /*
     @brief الإحداثي الأفقي للمؤشر بعد التحويل إلى المجال المنطقي.
     */
    float x = 0.0f;

    /*
     @brief الإحداثي العمودي للمؤشر بعد التحويل إلى المجال المنطقي.
     */
    float y = 0.0f;

    /*
     @brief يحدد إن كان التحويل للمؤشر قد تم بنجاح.
     */
    bool valid = false;
};
#pragma endregion GameplayTypes

#pragma region MovementHelpers
/*
 @brief يحول إدخال لوحة المفاتيح إلى نية حركة أفقية واحدة.
 @param input مرجع حالة الإدخال الحالية.
 @return -1 لليسار، 0 للسكون، +1 لليمين.
 */
[[nodiscard]] int resolveMoveIntent(const Input& input);

/*
 @brief يحدد اتجاه الصياد بناءً على مكان المؤشر حول مركزه الحالي.
 @param cursor موضع المؤشر المحول إلى فضاء الشاشة.
 @param hunterScreenX الإحداثي الأفقي للصياد على الشاشة.
 @param fallbackFacingLeft الاتجاه الافتراضي عند عدم صلاحية المؤشر.
 @return true إذا كان الصياد يتجه يساراً.
 */
[[nodiscard]] bool resolveFacingLeftFromCursor(const CursorScreenPosition& cursor, float hunterScreenX,
                                               bool fallbackFacingLeft);

/*
 @brief يختار اتجاه الوقوف: الحركة أولاً، ثم المؤشر عند السكون.
 @param moveIntent نية الحركة من resolveMoveIntent.
 @param cursor موضع المؤشر المحول.
 @param hunterScreenX الإحداثي الأفقي للصياد على الشاشة.
 @param fallbackFacingLeft الاتجاه الافتراضي.
 @return true إذا كان الصياد يتجه يساراً.
 */
[[nodiscard]] bool resolveIdleFacingLeft(int moveIntent, const CursorScreenPosition& cursor, float hunterScreenX,
                                         bool fallbackFacingLeft);

/*
 @brief يحدث مؤقت إعادة التلقيم اليدوي ويعيد فتح الإطلاق عند انتهائه.
 @param isReloading مرجع علامة إعادة التلقيم.
 @param reloadTimer مرجع المؤقت المتبقي.
 @param deltaTime الزمن المنقضي منذ آخر إطار.
 */
void updateReloadState(bool& isReloading, float& reloadTimer, float deltaTime);

/*
 @brief يكشف هل كليب الصياد الحالي هو مشي فعلاً.
 @param state حالة الرسم المتحرك الحالية.
 @return true إذا كان الكليب مشياً.
 */
[[nodiscard]] bool isWalkingClip(const game::AnimationState& state);

#pragma endregion MovementHelpers

#pragma region AimHelpers
/*
 @brief يحول موضع مؤشر النظام إلى إحداثيات شاشة منطقية من -1 إلى +1.
 @param input مرجع حالة الإدخال.
 @param window مقبض نافذة GLFW.
 @return موضع المؤشر مع علامة الصلاحية.
 */
[[nodiscard]] CursorScreenPosition resolveCursorScreenPosition(const Input& input, GLFWwindow* window);

/*
 @brief يقرر هل يلزم استخدام وضعية الإطلاق العالي اعتماداً على زاوية التصويب.
 @param input مرجع حالة الإدخال.
 @param window مقبض نافذة GLFW.
 @param hunterScreenX الإحداثي الأفقي للصياد على الشاشة.
 @param metrics قياسات النافذة المنطقية.
 @param frame مؤشر إلى فريم الأطلس المرجعي.
 @return true إذا كانت زاوية التصويب تقتضي الوضعية العالية.
 */
[[nodiscard]] bool shouldUseHighShootPose(const Input& input, GLFWwindow* window, float hunterScreenX,
                                          const scene::WindowMetrics& metrics, const game::AtlasFrame* frame);
#pragma endregion AimHelpers

} // namespace core::hunterplay
