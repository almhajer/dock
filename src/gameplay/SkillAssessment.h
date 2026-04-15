/**
 * @file SkillAssessment.h
 * @brief واجهة تقييم المهارة.
 * @details تقييم أداء اللاعب.
 */

#pragma once

namespace core
{

/*
 مستوى مهارة اللاعب
 */
enum class SkillLevel : unsigned char
{
    Poor,      // ضعيف
    Average,   // مقبول
    Good,      // جيد
    Excellent, // ممتاز
    Perfect,   // احترافي
};

/*
 تقييم نجاح المرحلة
 */
enum class SuccessGrade : unsigned char
{
    Normal,   // عادي
    Good,     // جيد
    Great,    // رائع
    Perfect,  // مثالي
};

/*
 إحصائيات أداء اللاعب
 */
struct PerformanceStats
{
    int shotsFired = 0;
    int ducksHit = 0;
    int ducksEscaped = 0;
    float stageTime = 0.0f;
};

/*
 نظام تقييم أداء اللاعب - يقيّم الأداء ويُعدّل الصعوبة
 */
class SkillAssessment
{
  public:
    /*
     إعادة ضبط التقييم لمرحلة جديدة
     */
    void reset();

    /*
     يُسجّل إطلاق طلقة
     */
    void onShotFired(bool hit);

    /*
     يُسجّل هروب بطة
     */
    void onDuckEscaped();

    /*
     يُحدّث الزمن
     */
    void update(float deltaTime);

    /*
     يقيّم المستوى الحالي للاعب
     */
    [[nodiscard]] SkillLevel evaluate() const;

    /*
     يحسب درجة النجاح بناءً على أداء المرحلة المنتهية
     */
    [[nodiscard]] SuccessGrade computeGrade(int ducksRequired, int shotsAllowed) const;

    /*
     يُرجع معامل الصعوبة (0.7 = أسهل، 1.0 = عادي، 1.3 = أصعب)
     */
    [[nodiscard]] float difficultyMultiplier() const;

    /*
     دقة التصويب كنسبة (0-1)
     */
    [[nodiscard]] float accuracy() const;

    /*
     الإحصائيات الحالية
     */
    [[nodiscard]] const PerformanceStats& stats() const
    {
        return mStats;
    }

    /*
     عدد الطلقات المُطلقة
     */
    [[nodiscard]] int shotsFired() const
    {
        return mStats.shotsFired;
    }

  private:
    PerformanceStats mStats;
};

} // namespace core
