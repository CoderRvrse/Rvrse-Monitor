#include "driver_service.h"

#include <Windows.h>
#include <filesystem>

#include "rvrse/driver_protocol.h"

namespace
{
    constexpr wchar_t kServiceName[] = L"RvrseMonitorDriver";
    constexpr wchar_t kServiceDisplayName[] = L"Rvrse Monitor Kernel Driver";

    class ScopedScHandle
    {
    public:
        explicit ScopedScHandle(SC_HANDLE handle) : handle_(handle) {}
        ~ScopedScHandle()
        {
            if (handle_)
            {
                CloseServiceHandle(handle_);
            }
        }

        SC_HANDLE get() const { return handle_; }
        SC_HANDLE release()
        {
            SC_HANDLE temp = handle_;
            handle_ = nullptr;
            return temp;
        }

    private:
        SC_HANDLE handle_ = nullptr;
    };

    std::wstring FormatMessageFromSystem(unsigned long errorCode)
    {
        if (errorCode == 0)
        {
            return L"Success";
        }

        LPWSTR buffer = nullptr;
        DWORD result = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                          FORMAT_MESSAGE_FROM_SYSTEM |
                                          FORMAT_MESSAGE_IGNORE_INSERTS,
                                      nullptr,
                                      errorCode,
                                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                      reinterpret_cast<LPWSTR>(&buffer),
                                      0,
                                      nullptr);
        if (result == 0 || !buffer)
        {
            return L"Unknown error";
        }

        std::wstring message(buffer);
        LocalFree(buffer);
        while (!message.empty() && (message.back() == L'\r' || message.back() == L'\n'))
        {
            message.pop_back();
        }
        return message;
    }

    std::wstring BuildAbsolutePath(const std::wstring &path)
    {
        namespace fs = std::filesystem;
        if (path.empty())
        {
            return {};
        }

        try
        {
            return fs::weakly_canonical(path).wstring();
        }
        catch (...)
        {
            return path;
        }
    }
}

namespace rvrse::core
{
    DriverServiceStatus DriverService::QueryStatus()
    {
        DriverServiceStatus status{};

        ScopedScHandle manager(OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT));
        if (!manager.get())
        {
            status.message = L"OpenSCManager failed: " + FormatErrorMessage(GetLastError());
            return status;
        }

        ScopedScHandle service(OpenServiceW(manager.get(), kServiceName, SERVICE_QUERY_STATUS));
        if (!service.get())
        {
            status.message = L"OpenService failed: " + FormatErrorMessage(GetLastError());
            return status;
        }

        SERVICE_STATUS_PROCESS processInfo{};
        DWORD bytesNeeded = 0;
        if (!QueryServiceStatusEx(service.get(),
                                  SC_STATUS_PROCESS_INFO,
                                  reinterpret_cast<LPBYTE>(&processInfo),
                                  sizeof(processInfo),
                                  &bytesNeeded))
        {
            status.message = L"QueryServiceStatusEx failed: " + FormatErrorMessage(GetLastError());
            return status;
        }

        status.installed = true;
        status.running = (processInfo.dwCurrentState == SERVICE_RUNNING);
        status.serviceState = processInfo.dwCurrentState;
        status.message = status.running ? L"Driver service is running." : L"Driver service is installed but not running.";
        return status;
    }

    bool DriverService::Install(const std::wstring &driverPath)
    {
        std::wstring path = driverPath.empty() ? DefaultDriverPath() : BuildAbsolutePath(driverPath);

        ScopedScHandle manager(OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE));
        if (!manager.get())
        {
            return false;
        }

        ScopedScHandle service(CreateServiceW(manager.get(),
                                              kServiceName,
                                              kServiceDisplayName,
                                              SERVICE_START | SERVICE_STOP | DELETE | SERVICE_QUERY_STATUS,
                                              SERVICE_KERNEL_DRIVER,
                                              SERVICE_DEMAND_START,
                                              SERVICE_ERROR_NORMAL,
                                              path.c_str(),
                                              nullptr,
                                              nullptr,
                                              nullptr,
                                              nullptr,
                                              nullptr));
        if (!service.get())
        {
            return false;
        }

        return true;
    }

    bool DriverService::Uninstall()
    {
        ScopedScHandle manager(OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT));
        if (!manager.get())
        {
            return false;
        }

        ScopedScHandle service(OpenServiceW(manager.get(), kServiceName, DELETE | SERVICE_STOP));
        if (!service.get())
        {
            return false;
        }

        SERVICE_STATUS status{};
        ControlService(service.get(), SERVICE_CONTROL_STOP, &status);
        BOOL deleted = DeleteService(service.get());
        return deleted == TRUE;
    }

    bool DriverService::Start()
    {
        ScopedScHandle manager(OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT));
        if (!manager.get())
        {
            return false;
        }

        ScopedScHandle service(OpenServiceW(manager.get(), kServiceName, SERVICE_START));
        if (!service.get())
        {
            return false;
        }

        if (StartServiceW(service.get(), 0, nullptr))
        {
            return true;
        }

        DWORD error = GetLastError();
        return error == ERROR_SERVICE_ALREADY_RUNNING;
    }

    bool DriverService::Stop()
    {
        ScopedScHandle manager(OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT));
        if (!manager.get())
        {
            return false;
        }

        ScopedScHandle service(OpenServiceW(manager.get(), kServiceName, SERVICE_STOP));
        if (!service.get())
        {
            return false;
        }

        SERVICE_STATUS status{};
        if (!ControlService(service.get(), SERVICE_CONTROL_STOP, &status))
        {
            return false;
        }

        return true;
    }

    std::wstring DriverService::DefaultDriverPath()
    {
        wchar_t modulePath[MAX_PATH];
        DWORD length = GetModuleFileNameW(nullptr, modulePath, std::size(modulePath));
        if (length == 0)
        {
            return L"C:\\Windows\\System32\\drivers\\RvrseMonitor.sys";
        }

        std::filesystem::path path(modulePath);
        path.remove_filename();
        path /= L"drivers";
        path /= L"RvrseMonitor.sys";
        return path.wstring();
    }

    std::wstring DriverService::FormatErrorMessage(unsigned long errorCode)
    {
        return FormatMessageFromSystem(errorCode);
    }
}
