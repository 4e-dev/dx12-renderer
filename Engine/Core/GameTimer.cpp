#include "GameTimer.h"

GameTimer::GameTimer()
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
    __int64 ticksPerSecond;
    QueryPerformanceCounter((LARGE_INTEGER*)&ticksPerSecond);
    mSecondsPerTick = 1.0 / (double)ticksPerSecond;
}

double GameTimer::TotalTime() const { return -1.0; }

double GameTimer::DeltaTime() const
{
    return mDeltaSeconds;
}

void GameTimer::Reset() {}

void GameTimer::Start() {}

void GameTimer::Stop() {}

void GameTimer::Tick()
{
    if (mStopped)
    {
        mDeltaSeconds = 0.0;
        return;
    }

    // Get time for this frame
    __int64 _currentTime;
    QueryPerformanceCounter((LARGE_INTEGER*)&_currentTime);
    mCurrentTick = _currentTime;
     
    // Calculate delta (time between this frame and the previous frame, in seconds)
    __int64 _deltaTicks = mCurrentTick - mPreviousTick;
    mDeltaSeconds = _deltaTicks * mSecondsPerTick; // convert

    // Prepare for next frame
    mPreviousTick = mCurrentTick;

    // Force nonnegative
    mDeltaSeconds = max(mDeltaSeconds, 0.0);
}
