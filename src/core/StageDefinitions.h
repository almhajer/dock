#pragma once

namespace core::stage
{

/*
 @brief إعدادات المرحلة - بيانات ثابتة لا تتغير أثناء اللعب.
 */
struct StageConfig
{
    int stageNumber;              // رقم المرحلة
    int shotsAllowed;             // عدد الطلقات المتاحة
    int ducksRequired;            // عدد البط المطلوب إصابته للنجاح
    float duckFlightDurationMin;  // أقل مدة رحلة البطة (ثواني)
    float duckFlightDurationMax;  // أكبر مدة رحلة البطة
    float duckArcMinHeight;       // أقل ارتفاع للقوس
    float duckArcMaxHeight;       // أكبر ارتفاع للقوس
    float timeLimitSeconds;       // 0 = بدون حد زمني
    int totalDucks;               // إجمالي البط في المرحلة
};

/*
 @brief حالة المرحلة الجارية - تتغير كل إطار.
 */
struct StageState
{
    int currentStageIndex = 0;    // فهرس المرحلة الحالية
    int shotsRemaining = 0;      // الطلقات المتبقية
    int initialShots = 0;        // الطلقات الكلية عند بداية المرحلة (تشمل المكافآت)
    int ducksHitThisStage = 0;   // عدد الإصابات في هذه المرحلة
    float stageElapsedTime = 0.0f; // الزمن المنقضي منذ بداية المرحلة
    bool stageActive = false;    // هل اللعب جارٍ في هذه المرحلة؟
};

/*
 @brief نتيجة المرحلة عند انتهائها.
 */
struct StageResult
{
    int stageNumber = 0;         // رقم المرحلة
    bool passed = false;         // هل نجح اللاعب؟
    int ducksHit = 0;            // عدد الإصابات
    int ducksRequired = 0;       // عدد البط المطلوب
    int ducksTotal = 0;          // إجمالي البط
    int ducksEscaped = 0;        // البط الذي هرب
    int shotsUsed = 0;           // الطلقات المستهلكة
    int shotsTotal = 0;          // الطلقات المتاحة
    int shotsRemaining = 0;      // الطلقات المتبقية
    float timeTaken = 0.0f;      // الزمن المستغرق
    int shotsFired = 0;          // الطلقات المُطلقة فعلياً
    float accuracy = 0.0f;       // دقة التصويب
};

/*
 @brief جدول المراحل المتدرجة الصعوبة.
 إجمالي البط المطلوب في كل مرحلة يساوي totalDucks
 والطلقات الأساسية = totalDucks + 2 كهامش عادل قبل أي مكافآت إضافية.
 */
inline constexpr StageConfig kStageTable[] = {
    /* .stageNumber, .shots, .required, .durMin, .durMax, .arcMin, .arcMax, .timeLimit, .totalDucks */
    {  1,   3,  1,  12.0f, 14.0f, 0.050f, 0.120f,  0.0f,  1 },
    {  2,   4,  2,  11.5f, 13.5f, 0.048f, 0.115f,  0.0f,  2 },
    {  3,   5,  3,  11.0f, 13.0f, 0.045f, 0.110f,  0.0f,  3 },
    {  4,   6,  4,  10.5f, 12.5f, 0.042f, 0.100f, 30.0f,  4 },
    {  5,   7,  5,  10.0f, 12.0f, 0.040f, 0.095f, 28.0f,  5 },
    {  6,   8,  6,   9.5f, 11.5f, 0.038f, 0.090f, 26.0f,  6 },
    {  7,   9,  7,   9.0f, 11.0f, 0.035f, 0.085f, 24.0f,  7 },
    {  8,  10,  8,   8.5f, 10.5f, 0.032f, 0.080f, 22.0f,  8 },
    {  9,  11,  9,   8.0f, 10.0f, 0.028f, 0.070f, 20.0f,  9 },
    { 10,  12, 10,   7.5f,  9.5f, 0.024f, 0.060f, 18.0f, 10 },
};

inline constexpr int kStageCount = sizeof(kStageTable) / sizeof(kStageTable[0]);

} // namespace core::stage
