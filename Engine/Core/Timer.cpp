#include "Timer.h"

Timer::Timer()
{
    // Initialize values
    mSecondsPerTick     =  0.0;
    mDeltaSeconds       = -1.0;
    mBaseTick           = 0;
    mPausedTicks        = 0;
    mStopTick           = 0;
    mPreviousTick       = 0;
    mCurrentTick        = 0;
    mStopped            = false;

    // Calculate the number of seconds that happen per 'Count'
    __int64 _ticksPerSecond;
    QueryPerformanceCounter((LARGE_INTEGER*)&_ticksPerSecond);
    mSecondsPerTick = 1.0 / (double)_ticksPerSecond;
}

double Timer::TotalTime() const
{
    return -1.0;
}

double Timer::DeltaTime() const
{
    return mDeltaSeconds;
}

void Timer::Reset() {}

void Timer::Start() {}

void Timer::Stop() {}

void Timer::Tick()
{
    // Game is paused/minimized. Do not advance frame time.
    if (mStopped)
    {
        mDeltaSeconds = 0.0;
        return;
    }

    // Get current tick-count.
    __int64 _currentTime;
    QueryPerformanceCounter((LARGE_INTEGER*)&_currentTime);
    mCurrentTick = _currentTime;
     
    // Calculate time elapsed since previous frame.
    __int64 _deltaTicks = mCurrentTick - mPreviousTick;
    mDeltaSeconds = _deltaTicks * mSecondsPerTick;

    // Prepare next frame.
    mPreviousTick = mCurrentTick;

    // Force nonnegative.
    mDeltaSeconds = max(mDeltaSeconds, 0.0);
}
