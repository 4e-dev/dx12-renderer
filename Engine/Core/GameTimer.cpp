#include "GameTimer.h"

GameTimer::GameTimer()
{
    // Initialize values
    mSecondsPerCount     =  0.0;
    mDeltaTime           = -1.0;
    mBaseTime            = 0;
    mPausedTime          = 0;
    mStopTime            = 0;
    mPreviousTime        = 0;
    mCurrentTime         = 0;
    mStopped             = false;

    // Calculate the number of seconds that happen per 'Count'
    __int64 countsPerSecond;
    QueryPerformanceCounter((LARGE_INTEGER*)&countsPerSecond);
    mSecondsPerCount = 1.0 / (double)countsPerSecond;
}

float GameTimer::GameTime() const { return -1.0; }

float GameTimer::DeltaTime() const
{
    return (float)mDeltaTime;
}

void GameTimer::Reset() {}

void GameTimer::Start() {}

void GameTimer::Stop() {}

void GameTimer::Tick()
{
    if (mStopped)
    {
        mDeltaTime = 0.0;
        return;
    }

    // Get time for this frame
    __int64 _currentTime;
    QueryPerformanceCounter((LARGE_INTEGER*)&_currentTime);
    mCurrentTime = _currentTime;
     
    // Calculate delta (time between this frame and the previous frame, in seconds)
    mDeltaTime = (mCurrentTime - mPreviousTime) * mSecondsPerCount;

    // Prepare for next frame
    mPreviousTime = mCurrentTime;

    // Force nonnegative
    mDeltaTime = mDeltaTime < 0.0 ? 0.0 : mDeltaTime;
}
