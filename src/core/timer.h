#pragma once

#include <windows.h>

/*
REFERENCE:
ticks/counts = queryperformancecounter
counts per second = queryperformancefrequency
seconds per count = 1 / (counts per second)
seconds = ticks * (seconds per tick)
*/

class Timer {
public:
    // Initializes `Timer` class member values.
    Timer();

    float GameTime() const;

    // Public getter for `mDeltaTime`.
    float DeltaTime() const;

    void Reset();
    void Start();
    void Stop();

    // Updates delta time per frame (per 'tick').
    void Tick();

private:
    double mSecondsPerCount;
    double mDeltaTime;

    __int64 mBaseTime;
    __int64 mPausedTime;
    __int64 mStopTime;

    // Primarily used for mDeltaTime calculation.
    __int64 mCurrCount;
    __int64 mPrevCount;

    bool mStopped;
};
