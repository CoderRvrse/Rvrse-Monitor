#pragma once

#include <string>

namespace rvrse::core
{
    struct DriverStatus
    {
        bool available = false;
        bool pingSucceeded = false;
        std::wstring message;
    };

    class DriverInterface
    {
    public:
        static DriverStatus EnsureDriverAvailable();
        static bool Ping();

    private:
        static void CloseHandle(void *handle);
        static void *OpenHandle();
    };
}
