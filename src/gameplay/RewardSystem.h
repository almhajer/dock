/**
 * @file RewardSystem.h
 * @brief واجهة نظام المكافآت.
 * @details حساب المكافآت بناءً على الأداء.
 */

#pragma once

#include "SkillAssessment.h"
#include "../core/StageObjectives.h"

namespace core
{

/*
 مكافأة مرحلة واحدة
 */
struct StageReward
{
    int bonusScore = 0;         // نقاط إضافية
    int bonusShotsNext = 0;     // طلقات إضافية للمرحلة التالية
    float scoreMultiplier = 1.0f; // مضاعف النقاط
    SuccessGrade grade = SuccessGrade::Normal;
};

/*
 بيانات تقدم اللاعب عبر اللعبة
 */
struct PlayerProgression
{
    int totalScore = 0;           // النقاط الإجمالية
    int stagesCompleted = 0;      // المراحل المكتملة
    int winStreak = 0;            // سلسلة الانتصارات الحالية
    int bestWinStreak = 0;        // أفضل سلسلة انتصارات
    int bonusShotsBanked = 0;     // طلقات إضافية مخزّنة
    int stagesPlayed = 0;         // المراحل التي لُعبت
    int perfectStages = 0;        // مراحل مثالية
    bool rareUnlocked = false;    // هل فُتحت المراحل النادرة
};

/*
 نظام المكافآت والتقدم - يحسب الجوائز ويدير التقدم طويل المدى
 */
class RewardSystem
{
  public:
    /*
     إعادة ضبط كامل (لعبة جديدة)
     */
    void reset();

    /*
     يحسب مكافأة المرحلة بناءً على الأداء
     */
    [[nodiscard]] StageReward computeReward(
        const stage::StageConfig& config,
        const ExtendedStageConfig& extConfig,
        const stage::StageResult& result,
        SkillLevel skillLevel) const;

    /*
     يُسجّل إكمال مرحلة بنجاح
     */
    void onStageCompleted(const StageReward& reward);

    /*
     يُسجّل فشل مرحلة
     */
    void onStageFailed();

    /*
     يستهلك الطلقات الإضافية المخزّنة (يُنادى عند بدء مرحلة جديدة)
     */
    int consumeBonusShots();

    /*
     بيانات التقدم
     */
    [[nodiscard]] const PlayerProgression& progression() const
    {
        return mProgression;
    }

    /*
     هل يجب عرض مرحلة نادرة؟ (بناءً على سلسلة الانتصارات)
     */
    [[nodiscard]] bool shouldShowRareStage() const;

    /*
     هل يجب عرض مرحلة مكافأة؟
     */
    [[nodiscard]] bool shouldShowBonusStage(int currentStageIndex) const;

  private:
    PlayerProgression mProgression;
};

} // namespace core
