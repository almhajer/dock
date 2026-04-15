/**
 * @file Timer.cpp
 * @brief تنفيذ المؤقت.
 * @details حساب الوقت والإطارات.
 */

#include "Timer.h"

namespace core
{

Timer::Timer() : mStartTime(Clock::now()), mLastTime(Clock::now()), mLastFPSTime(Clock::now())
{
}

/**
 * @brief تحديث المؤقت.
 */
void Timer::update()
{
    auto currentTime = Clock::now();

    // حساب الوقت بين الإطارات
    mDeltaTime = std::chrono::duration<float>(currentTime - mLastTime).count();

    // حساب الوقت الكلي منذ البداية
    mTotalTime = std::chrono::duration<float>(currentTime - mStartTime).count();

    mLastTime = currentTime;
    mFrameCount++;

    // تحديث FPS كل نصف ثانية
    float timeSinceLastFPS = std::chrono::duration<float>(currentTime - mLastFPSTime).count();
    if (timeSinceLastFPS >= 0.5f)
    {
        mFPS = static_cast<int>(mFrameCount / timeSinceLastFPS);
        mFrameCount = 0;
        mLastFPSTime = currentTime;
    }
}

/**
 * @brief الحصول على الوقت بين الإطارات.
 * @return الوقت بين الإطارات بالثواني.
 */
float Timer::getDeltaTime() const
{
    return mDeltaTime;
}

/**
 * @brief الحصول على الوقت الإجمالي منذ البداية.
 * @return الوقت الإجمالي بالثواني.
 */
float Timer::getTotalTime() const
{
    return mTotalTime;
}

/**
 * @brief الحصول على الإطارات في الثانية.
 * @return عدد الإطارات في الثانية.
 */
int Timer::getFPS() const
{
    return mFPS;
}

} // namespace core
