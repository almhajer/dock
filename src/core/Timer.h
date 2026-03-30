#pragma once

#include <chrono>

namespace core {

/// مؤقت - يحسب الوقت بين الإطارات ومعدل الإطارات في الثانية
class Timer {
public:
    Timer();

    /// تحديث المؤقت - يُستدعى مرة كل إطار
    void update();

    /// الوقت المنقضي منذ آخر إطار بالثواني
    [[nodiscard]] float getDeltaTime() const;

    /// إجمالي الوقت المنقضي منذ بدء المؤقت بالثواني
    [[nodiscard]] float getTotalTime() const;

    /// عدد الإطارات في الثانية (محدّث كل نصف ثانية)
    [[nodiscard]] int getFPS() const;

private:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = Clock::time_point;

    TimePoint mStartTime;      // لحظة بدء المؤقت
    TimePoint mLastTime;       // لحظة آخر إطار
    TimePoint mLastFPSTime;    // لحظة آخر تحديث للـ FPS
    float mDeltaTime = 0.0f;   // الوقت بين الإطارات
    float mTotalTime = 0.0f;   // الوقت الكلي
    int mFPS = 0;              // معدل الإطارات الحالي
    int mFrameCount = 0;       // عداد الإطارات لحساب FPS
};

} // namespace core
