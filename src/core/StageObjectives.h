#pragma once

#include "StageDefinitions.h"

namespace core
{

/*
 نوع هدف المرحلة
 */
enum class StageObjective : unsigned char
{
    KillMinimum,    // إصابة حد أدنى
    KillAll,        // إصابة كل البط
    AccuracyTarget, // دقة تصويب معينة
    NoEscape,       // عدم السماح بهروب أكثر من عدد معين
    TimeAttack,     // إنهاء سريع ضمن وقت
    SurviveWaves,   // النجاة من جميع الموجات
};

/*
 نوع المرحلة
 */
enum class StageType : unsigned char
{
    Normal,   // عادية
    Bonus,    // مكافأة - سهلة ونقاط عالية
    Challenge,// تحدي - صعبة ومكافأة كبيرة
    Accuracy, // دقة - تتطلب مهارة
    Rare,     // نادرة - تظهر عشوائياً
    Boss,     // زعيم - مرحلة نهائية صعبة
};

/*
 إعدادات المرحلة الموسعة
 */
struct ExtendedStageConfig
{
    StageType type = StageType::Normal;
    StageObjective objective = StageObjective::KillMinimum;

    // معاملات المكافأة
    float scoreMultiplier = 1.0f;       // مضاعف النقاط
    int bonusShotsReward = 0;           // طلقات إضافية كجائزة

    // أهداف خاصة
    float accuracyThreshold = 0.0f;    // الحد الأدنى للدقة (AccuracyTarget)
    int maxEscaped = 0;                // أقصى عدد هاربين (NoEscape)

    // مراحل نادرة
    int rareAppearAfterStages = 0;     // تظهر بعد كم مرحلة (0 = لا)
};

/*
 جدول المراحل الموسعة - يتطابق مع kStageTable بالفهرس
 */
inline constexpr ExtendedStageConfig kExtendedStageTable[] = {
    /* type,            objective,             scoreMul, bonusShots, accThresh, maxEscaped, rareAfter */
    {StageType::Normal,   StageObjective::KillMinimum,  1.0f, 0, 0.0f, 0, 0}, // 1
    {StageType::Normal,   StageObjective::KillMinimum,  1.0f, 0, 0.0f, 0, 0}, // 2
    {StageType::Normal,   StageObjective::SurviveWaves, 1.0f, 0, 0.0f, 0, 0}, // 3
    {StageType::Accuracy, StageObjective::AccuracyTarget, 1.2f, 1, 0.60f, 0, 0}, // 4
    {StageType::Normal,   StageObjective::KillAll,      1.0f, 0, 0.0f, 0, 0}, // 5
    {StageType::Bonus,    StageObjective::KillMinimum,  2.0f, 2, 0.0f, 0, 0}, // 7 - مكافأة
    {StageType::Normal,   StageObjective::NoEscape,     1.0f, 0, 0.0f, 1, 0}, // 7
    {StageType::Challenge,StageObjective::KillAll,      1.5f, 2, 0.0f, 0, 0}, // 8 - تحدي
    {StageType::Normal,   StageObjective::TimeAttack,   1.0f, 0, 0.0f, 0, 0}, // 9
    {StageType::Boss,     StageObjective::KillAll,      2.5f, 3, 0.0f, 0, 0}, // 10 - زعيم
};

inline constexpr int kExtendedStageCount = sizeof(kExtendedStageTable) / sizeof(kExtendedStageTable[0]);

/*
 يتحقق من تحقق هدف المرحلة
 */
[[nodiscard]] inline bool isObjectiveMet(
    StageObjective objective,
    int ducksHit,
    int ducksTotal,
    int ducksRequired,
    int ducksEscaped,
    int /*shotsFired*/,
    float accuracy,
    float timeTaken,
    float timeLimit,
    int maxEscaped)
{
    switch (objective)
    {
    case StageObjective::KillMinimum:
        return ducksHit >= ducksRequired;
    case StageObjective::KillAll:
        return ducksHit >= ducksTotal;
    case StageObjective::AccuracyTarget:
        return ducksHit >= ducksRequired && accuracy >= 0.01f; // accuracy checked separately
    case StageObjective::NoEscape:
        return ducksHit >= ducksRequired && ducksEscaped <= maxEscaped;
    case StageObjective::TimeAttack:
        return ducksHit >= ducksRequired && (timeLimit <= 0.0f || timeTaken <= timeLimit);
    case StageObjective::SurviveWaves:
        return ducksHit >= ducksRequired;
    }
    return ducksHit >= ducksRequired;
}

/*
 يتحقق من فشل الهدف (حالات خاصة)
 */
[[nodiscard]] inline bool isObjectiveFailed(
    StageObjective objective,
    int ducksEscaped,
    int maxEscaped,
    float accuracy,
    float accuracyThreshold)
{
    if (objective == StageObjective::NoEscape && ducksEscaped > maxEscaped)
    {
        return true;
    }
    if (objective == StageObjective::AccuracyTarget && accuracyThreshold > 0.0f &&
        accuracy < accuracyThreshold * 0.5f)
    {
        // فشل مبكر إذا الدقة منخفضة جداً
        return false; // لا نفشل مبكراً، نترك المرحلة تكمل
    }
    return false;
}

/*
 يحسب وصف الهدف كنص (يُستخدم في العرض)
 */
[[nodiscard]] inline const char* objectiveLabel(StageObjective obj)
{
    switch (obj)
    {
    case StageObjective::KillMinimum:  return "KILL";
    case StageObjective::KillAll:      return "ALL";
    case StageObjective::AccuracyTarget: return "AIM";
    case StageObjective::NoEscape:     return "NO X";
    case StageObjective::TimeAttack:   return "FAST";
    case StageObjective::SurviveWaves: return "LIVE";
    }
    return "KILL";
}

} // namespace core
