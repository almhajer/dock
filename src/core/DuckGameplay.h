#pragma once

#include "../game/SpriteAnimation.h"

namespace core::duckplay {

#pragma region MotionConstants
/// @brief ثوابت سلوك البطة مجمعة هنا حتى تبقى قرارات الحركة والاختفاء والإصابة في مكان واحد واضح.
inline constexpr float kDuckScreenHalfHeight = 0.17f;
inline constexpr float kDuckTravelMargin = 0.22f;
inline constexpr float kDuckTopSpawnMargin = 0.06f;
inline constexpr float kDuckHunterClearance = 0.04f;
inline constexpr float kDuckFlightMinSeconds = 11.8f;
inline constexpr float kDuckFlightMaxSeconds = 13.6f;
inline constexpr float kDuckGlideLift = 0.012f;
inline constexpr float kDuckFlapBobAmplitude = 0.010f;
inline constexpr float kDuckSwayAmplitude = 0.008f;
inline constexpr float kDuckLaunchPhaseRatio = 0.34f;
inline constexpr float kDuckLaunchDistanceRatio = 0.18f;
inline constexpr float kDuckArcMinHeight = 0.045f;
inline constexpr float kDuckArcMaxHeight = 0.11f;
inline constexpr float kDuckControl1MinRatio = 0.32f;
inline constexpr float kDuckControl1MaxRatio = 0.62f;
inline constexpr float kDuckControl2MinRatio = 0.08f;
inline constexpr float kDuckControl2MaxRatio = 0.28f;
inline constexpr float kDuckFlapCyclesMin = 2.35f;
inline constexpr float kDuckFlapCyclesMax = 3.25f;
inline constexpr float kDuckFlightRotationMax = 0.18f;
inline constexpr float kDuckFlapRollMax = 0.035f;
inline constexpr float kDuckFallGravity = 1.85f;
inline constexpr float kDuckFallHorizontalDrift = 0.16f;
inline constexpr float kDuckFallInitialLift = -0.24f;
inline constexpr float kDuckFallAngularSpeed = 4.2f;
inline constexpr float kDuckGroundHideDelaySeconds = 2.0f;
inline constexpr float kTau = 6.28318530718f;
inline constexpr float kPi = 3.14159265359f;
#pragma endregion MotionConstants

#pragma region MotionHelpers
/// @brief منحنى ناعم بين 0 و 1 لتجنب الحركات الميكانيكية الحادة.
/// @param t القيمة الداخلة بين 0 و 1.
/// @return القيمة الملساء بين 0 و 1.
[[nodiscard]] float smoothstep01(float t);

/// @brief يبطئ بداية الرحلة ثم يسمح بالتسارع التدريجي بعد الإقلاع.
/// @param t طور الرحلة بين 0 و 1.
/// @return الطور المعاد رسمه بين 0 و 1.
[[nodiscard]] float remapTravelPhase(float t);

/// @brief يحسب موضعاً على منحنى Bezier رباعي النقاط.
/// @param p0 نقطة التحكم الأولى (البداية).
/// @param p1 نقطة التحكم الثانية.
/// @param p2 نقطة التحكم الثالثة.
/// @param p3 نقطة التحكم الرابعة (النهاية).
/// @param t معامل الاستيفاء بين 0 و 1.
/// @return الموضع على المنحنى.
[[nodiscard]] float cubicBezier1D(float p0, float p1, float p2, float p3, float t);

/// @brief يعطي ميل المنحنى نفسه لاشتقاق زاوية الطيران.
/// @param p0 نقطة التحكم الأولى (البداية).
/// @param p1 نقطة التحكم الثانية.
/// @param p2 نقطة التحكم الثالثة.
/// @param p3 نقطة التحكم الرابعة (النهاية).
/// @param t معامل الاستيفاء بين 0 و 1.
/// @return قيمة الميل عند t.
[[nodiscard]] float cubicBezierTangent1D(float p0, float p1, float p2, float p3, float t);

/// @brief يثبت البطة على فريم الإصابة الأرضي المعتمد بعد السقوط.
/// @param state مرجع حالة الرسم المتحرك لتعديل الفريم.
void holdDuckOnGroundHitFrame(game::AnimationState& state);
#pragma endregion MotionHelpers

} // namespace core::duckplay
