#include "timer.h"

Timer::Timer() {
    mDeltaTime = -1.0;
    mSecondsPerCount = 0.0;

    mBaseTime = 0;
    mPausedTime = 0;
    mStopTime = 0;

    mCurrCount = 0;
    mPrevCount = 0;

    mStopped = false;

    __int64 countsPerSecond;
    QueryPerformanceCounter((LARGE_INTEGER*)&countsPerSecond);
    mSecondsPerCount = 1.0 / (double)countsPerSecond;
}

// TODO(bao): Define the declarations

float Timer::GameTime() const {
    return ...;
}

float Timer::DeltaTime() const {
    return static_cast<float>(mDeltaTime);
}

void Timer::Reset() {
    return ...;
}

void Timer::Start() {
    return ...;
}

void Timer::Stop() {
    return ...;
}

void Timer::Tick() {
    if (mStopped) {
        mDeltaTime = 0.0;
        return;
    }

    // Get time of current frame
    QueryPerformanceCounter((LARGE_INTEGER*)&mCurrCount);

    // Update delta time
    mDeltaTime = (mCurrCount - mPrevCount) * mSecondsPerCount;
    mDeltaTime = max(mDeltaTime, 0.0);

    // Prepare for the next frame
    mPrevCount = mCurrCount;
}
