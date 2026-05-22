#pragma once

#include <windows.h>

class Timer
{
public:
    Timer();

    double TotalTime() const;
    double DeltaTime() const;

    void Reset();
    void Start();
    void Stop();

    // Calculate the time elapsed between the previous frame (0, at first) and the current frame (now).
    void Tick();

private:
    double mSecondsPerTick;
    double mDeltaSeconds;

    __int64 mBaseTick;
    __int64 mPausedTicks;
    __int64 mStopTick;
    __int64 mPreviousTick;
    __int64 mCurrentTick;

    bool mStopped;
};
