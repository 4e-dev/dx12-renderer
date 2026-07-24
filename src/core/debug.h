// Core/Debug.h
#pragma once

#include <windows.h>
#include <cstdio>

#if defined(_DEBUG)
    #define LOGF(...)                                       \
        do                                                  \
        {                                                   \
            char buffer[512];                               \
            sprintf_s(buffer, sizeof(buffer), __VA_ARGS__); \
            OutputDebugStringA(buffer);                     \
        } while (0)
#else
    #define LOGF(...) do {} while (0)
#endif
