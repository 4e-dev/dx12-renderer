#include "GameTimer.h"

GameTimer::GameTimer()
{
    // Initialize values
    secondsPerCount     =  0.0;
    deltaTime           = -1.0;
    baseTime            = 0;
    pausedTime          = 0;
    stopTime            = 0;
    previousTime        = 0;
    currentTime         = 0;
    stopped             = false;

    // Calculate the number of seconds that happen per 'Count'
    __int64 countsPerSecond;
    QueryPerformanceCounter((LARGE_INTEGER*)&countsPerSecond);
    secondsPerCount = 1.0 / (double)countsPerSecond;
}
