/**
 * @file Timer.h
 * @brief واجهة إدارة الوقت.
 * @details يحسب الوقت بين الإطارات ومعدل الإطارات.
 */

#pragma once

#include <chrono>

namespace core
{

/**
 * @brief مؤقت - يحسب الوقت بين الإطارات ومعدل الإطارات في الثانية.
 */
class Timer
{
  public:
#pragma region PublicInterface
    /**
     * @brief ينشئ المؤقت ويثبت لحظة البداية.
     */
    Timer();

    /**
     * @brief تحديث المؤقت - يُستدعى مرة كل إطار.
     */
    void update();

    /**
     * @brief الوقت المنقضي منذ آخر إطار بالثواني.
     * @return delta time بالثواني.
     */
    [[nodiscard]] float getDeltaTime() const;

    /**
     * @brief إجمالي الوقت المنقضي منذ بدء المؤقت بالثواني.
     * @return الوقت الإجمالي بالثواني.
     */
    [[nodiscard]] float getTotalTime() const;

    /**
     * @brief عدد الإطارات في الثانية (محدّث كل نصف ثانية).
     * @return معدل الإطارات.
     */
    [[nodiscard]] int getFPS() const;
#pragma endregion PublicInterface

  private:
#pragma region InternalTypes
    /*
     ساعة التوقيت الأساسية المعتمدة داخل المؤقت.
     */
    using Clock = std::chrono::high_resolution_clock;

    /*
     نوع النقطة الزمنية الراجع من الساعة.
     */
    using TimePoint = Clock::time_point;
#pragma endregion InternalTypes

#pragma region InternalState
    /*
     لحظة بدء المؤقت.
     */
    TimePoint mStartTime; // لحظة بدء المؤقت

    /*
     لحظة تحديث الإطار السابق.
     */
    TimePoint mLastTime; // لحظة آخر إطار

    /*
     لحظة آخر تحديث محسوب لمعدل الإطارات.
     */
    TimePoint mLastFPSTime; // لحظة آخر تحديث للـ FPS

    /*
     الزمن بين الإطار الحالي والسابق.
     */
    float mDeltaTime = 0.0f; // الوقت بين الإطارات

    /*
     الزمن الكلي منذ بدء التطبيق.
     */
    float mTotalTime = 0.0f; // الوقت الكلي

    /*
     قيمة FPS الأخيرة المحسوبة.
     */
    int mFPS = 0; // معدل الإطارات الحالي

    /*
     عداد الإطارات المستخدم لحساب FPS.
     */
    int mFrameCount = 0; // عداد الإطارات لحساب FPS
#pragma endregion InternalState
};

} // namespace core
