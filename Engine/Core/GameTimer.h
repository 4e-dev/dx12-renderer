#pragma once

#include <windows.h>

class GameTimer
{
public:
    GameTimer();

    double TotalTime() const;
    double DeltaTime() const;

    void Reset();
    void Start();
    void Stop();
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
