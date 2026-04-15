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

float Timer::getDeltaTime() const
{
    return mDeltaTime;
}

float Timer::getTotalTime() const
{
    return mTotalTime;
}

int Timer::getFPS() const
{
    return mFPS;
}

} // namespace core
