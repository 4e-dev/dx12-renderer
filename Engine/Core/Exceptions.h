// Core/Exceptions.h
#pragma once

#include <stdexcept>
#include <string>
#include <format>
#include <windows.h>

namespace Core
{
    class win32_error : public std::runtime_error
    {
    public:
        explicit win32_error(const std::string& message)
            : std::runtime_error(BuildMessage(message, GetLastError()))
        {
        }

        explicit win32_error(const std::string& message, DWORD errorCode)
            : std::runtime_error(BuildMessage(message, errorCode))
        {
        }

    private:
        static std::string BuildMessage(
            const std::string& message,
            DWORD errorCode)
        {
            LPSTR errorBuffer = nullptr;

            DWORD size = FormatMessageA(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                nullptr,
                errorCode,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                reinterpret_cast<LPSTR>(&errorBuffer),
                0,
                nullptr);

            std::string systemMessage =
                size > 0
                ? std::string(errorBuffer, size)
                : "Unknown Win32 error";

            LocalFree(errorBuffer);

            return std::format(
                "{}\nWin32 Error {}: {}",
                message,
                errorCode,
                systemMessage);
        }
    };
}
