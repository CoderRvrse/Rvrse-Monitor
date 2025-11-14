#pragma once

#include <string>

namespace rvrse::core
{
    struct DriverServiceStatus
    {
        bool installed = false;
        bool running = false;
        unsigned long serviceState = 0;
        std::wstring message;
    };

    class DriverService
    {
    public:
        static DriverServiceStatus QueryStatus();
        static bool Install(const std::wstring &driverPath = L"");
        static bool Uninstall();
        static bool Start();
        static bool Stop();
        static std::wstring DefaultDriverPath();

    private:
        static std::wstring FormatErrorMessage(unsigned long errorCode);
    };
}
