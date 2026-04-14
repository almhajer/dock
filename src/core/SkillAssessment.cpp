#include "SkillAssessment.h"

#include <algorithm>
#include <cmath>

namespace core
{

void SkillAssessment::reset()
{
    mStats = {};
}

void SkillAssessment::onShotFired(bool hit)
{
    ++mStats.shotsFired;
    if (hit)
    {
        ++mStats.ducksHit;
    }
}

void SkillAssessment::onDuckEscaped()
{
    ++mStats.ducksEscaped;
}

void SkillAssessment::update(float deltaTime)
{
    mStats.stageTime += deltaTime;
}

SkillLevel SkillAssessment::evaluate() const
{
    if (mStats.shotsFired <= 0)
    {
        return SkillLevel::Poor;
    }

    const float acc = accuracy();
    const int totalDucks = mStats.ducksHit + mStats.ducksEscaped;
    const float hitRate = totalDucks > 0 ? static_cast<float>(mStats.ducksHit) / static_cast<float>(totalDucks) : 0.0f;

    // نقاط مركبة: دقة + نسبة إصابة
    const float score = acc * 0.6f + hitRate * 0.4f;

    if (score >= 0.9f)
    {
        return SkillLevel::Perfect;
    }
    if (score >= 0.75f)
    {
        return SkillLevel::Excellent;
    }
    if (score >= 0.55f)
    {
        return SkillLevel::Good;
    }
    if (score >= 0.35f)
    {
        return SkillLevel::Average;
    }
    return SkillLevel::Poor;
}

SuccessGrade SkillAssessment::computeGrade(int ducksRequired, int shotsAllowed) const
{
    if (mStats.shotsFired <= 0 || ducksRequired <= 0)
    {
        return SuccessGrade::Normal;
    }

    const float acc = accuracy();
    const float efficiency = static_cast<float>(mStats.ducksHit) / static_cast<float>(mStats.shotsFired);

    // نسبة الطلقات المستهلكة
    const float shotUsage = static_cast<float>(mStats.shotsFired) / static_cast<float>(shotsAllowed);

    // نقاط مركبة
    const float score = acc * 0.5f + efficiency * 0.3f + (1.0f - shotUsage) * 0.2f;

    if (score >= 0.9f)
    {
        return SuccessGrade::Perfect;
    }
    if (score >= 0.75f)
    {
        return SuccessGrade::Great;
    }
    if (score >= 0.5f)
    {
        return SuccessGrade::Good;
    }
    return SuccessGrade::Normal;
}

float SkillAssessment::difficultyMultiplier() const
{
    const SkillLevel level = evaluate();
    switch (level)
    {
    case SkillLevel::Perfect:
        return 1.3f;
    case SkillLevel::Excellent:
        return 1.15f;
    case SkillLevel::Good:
        return 1.0f;
    case SkillLevel::Average:
        return 0.9f;
    case SkillLevel::Poor:
        return 0.8f;
    }
    return 1.0f;
}

float SkillAssessment::accuracy() const
{
    if (mStats.shotsFired <= 0)
    {
        return 0.0f;
    }
    return std::clamp(static_cast<float>(mStats.ducksHit) / static_cast<float>(mStats.shotsFired), 0.0f, 1.0f);
}

} // namespace core
