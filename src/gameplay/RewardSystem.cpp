/**
 * @file RewardSystem.cpp
 * @brief تنفيذ نظام المكافآت.
 * @details حساب النقاط والمكافآت.
 */

#include "RewardSystem.h"

#include <algorithm>

namespace core
{

/**
 * @brief إعادة تعيين التقدم.
 */
void RewardSystem::reset()
{
    mProgression = {};
}

/**
 * @brief حساب المكافأة للمرحلة.
 * @param config إعدادات المرحلة.
 * @param extConfig إعدادات المرحلة الموسعة.
 * @param result نتيجة المرحلة.
 * @param skillLevel مستوى المهارة.
 * @return المكافأة المحسوبة.
 */
StageReward RewardSystem::computeReward(
    const stage::StageConfig& /*config*/,
    const ExtendedStageConfig& extConfig,
    const stage::StageResult& result,
    SkillLevel /*skillLevel*/) const
{
    StageReward reward;
    reward.scoreMultiplier = extConfig.scoreMultiplier;

    // حساب درجة النجاح
    if (result.shotsFired > 0 && result.ducksRequired > 0)
    {
        const float acc = result.accuracy;
        const float eff = static_cast<float>(result.ducksHit) /
                          static_cast<float>(std::max(result.shotsFired, 1));
        const float shotUsage = static_cast<float>(result.shotsUsed) /
                                static_cast<float>(std::max(result.shotsTotal, 1));
        const float score = acc * 0.5f + eff * 0.3f + (1.0f - shotUsage) * 0.2f;

        if (score >= 0.9f)
        {
            reward.grade = SuccessGrade::Perfect;
        }
        else if (score >= 0.75f)
        {
            reward.grade = SuccessGrade::Great;
        }
        else if (score >= 0.5f)
        {
            reward.grade = SuccessGrade::Good;
        }
        else
        {
            reward.grade = SuccessGrade::Normal;
        }
    }

    // نقاط المرحلة الأساسية
    int baseScore = result.ducksHit * 100;

    // مضاعف حسب نوع المرحلة
    baseScore = static_cast<int>(static_cast<float>(baseScore) * reward.scoreMultiplier);

    // نقاط إضافية حسب درجة النجاح
    int gradeBonus = 0;
    switch (reward.grade)
    {
    case SuccessGrade::Normal:
        gradeBonus = 0;
        break;
    case SuccessGrade::Good:
        gradeBonus = 50;
        break;
    case SuccessGrade::Great:
        gradeBonus = 150;
        break;
    case SuccessGrade::Perfect:
        gradeBonus = 300;
        break;
    }
    baseScore += gradeBonus;

    // نقاط الطلقات المتبقية
    baseScore += result.shotsRemaining * 20;

    reward.bonusScore = baseScore;

    // طلقات إضافية للمرحلة التالية
    reward.bonusShotsNext = extConfig.bonusShotsReward;

    // طلقات إضافية حسب التقييم
    if (reward.grade == SuccessGrade::Perfect)
    {
        reward.bonusShotsNext += 2;
    }
    else if (reward.grade == SuccessGrade::Great)
    {
        reward.bonusShotsNext += 1;
    }

    // مرحلة مكافأة: طلقات إضافية دائماً
    if (extConfig.type == StageType::Bonus)
    {
        reward.bonusShotsNext += 2;
    }

    // مرحلة تحدي: نقاط مضاعفة إضافية
    if (extConfig.type == StageType::Challenge || extConfig.type == StageType::Boss)
    {
        reward.bonusScore = static_cast<int>(static_cast<float>(reward.bonusScore) * 1.5f);
    }

    return reward;
}

void RewardSystem::onStageCompleted(const StageReward& reward)
{
    mProgression.totalScore += reward.bonusScore;
    mProgression.stagesCompleted++;
    mProgression.winStreak++;
    mProgression.stagesPlayed++;

    if (mProgression.winStreak > mProgression.bestWinStreak)
    {
        mProgression.bestWinStreak = mProgression.winStreak;
    }

    if (reward.grade == SuccessGrade::Perfect)
    {
        mProgression.perfectStages++;
    }

    mProgression.bonusShotsBanked += reward.bonusShotsNext;

    // فتح المراحل النادرة بعد 5 مراحل مثالية
    if (mProgression.perfectStages >= 5)
    {
        mProgression.rareUnlocked = true;
    }

    // مكافأة سلسلة انتصارات
    if (mProgression.winStreak == 3)
    {
        mProgression.bonusShotsBanked += 2;
    }
    else if (mProgression.winStreak == 5)
    {
        mProgression.bonusShotsBanked += 3;
    }
    else if (mProgression.winStreak == 10)
    {
        mProgression.bonusShotsBanked += 5;
    }
}

void RewardSystem::onStageFailed()
{
    mProgression.winStreak = 0;
    mProgression.stagesPlayed++;
}

int RewardSystem::consumeBonusShots()
{
    const int shots = mProgression.bonusShotsBanked;
    mProgression.bonusShotsBanked = 0;
    return shots;
}

bool RewardSystem::shouldShowRareStage() const
{
    return mProgression.rareUnlocked && mProgression.winStreak >= 3 && mProgression.stagesCompleted > 0;
}

bool RewardSystem::shouldShowBonusStage(int currentStageIndex) const
{
    // مرحلة مكافأة كل 3 مراحل ناجحة
    return mProgression.winStreak > 0 && mProgression.winStreak % 3 == 0 && currentStageIndex > 0;
}

} // namespace core
