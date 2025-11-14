#include "driver_interface.h"

#include <Windows.h>

#include "driver_service.h"
#include "rvrse/driver_protocol.h"

namespace
{
    class ScopedHandle
    {
    public:
        explicit ScopedHandle(HANDLE handle) : handle_(handle) {}
        ~ScopedHandle()
        {
            if (handle_ && handle_ != INVALID_HANDLE_VALUE)
            {
                CloseHandle(handle_);
            }
        }

        HANDLE get() const { return handle_; }
        HANDLE release()
        {
            HANDLE temp = handle_;
            handle_ = nullptr;
            return temp;
        }

    private:
        HANDLE handle_{INVALID_HANDLE_VALUE};
    };
}

namespace rvrse::core
{
    DriverStatus DriverInterface::EnsureDriverAvailable()
    {
        DriverStatus status{};
        auto serviceStatus = DriverService::QueryStatus();
        status.message = serviceStatus.message;

        ScopedHandle handle(OpenHandle());
        if (!handle.get() || handle.get() == INVALID_HANDLE_VALUE)
        {
            status.message += L" Device open failed.";
            return status;
        }

        status.available = true;
        status.pingSucceeded = Ping();
        status.message = status.pingSucceeded ? L"Driver responded successfully." : L"Driver did not respond to ping.";
        return status;
    }

    bool DriverInterface::Ping()
    {
        ScopedHandle handle(OpenHandle());
        if (!handle.get() || handle.get() == INVALID_HANDLE_VALUE)
        {
            return false;
        }

        DWORD bytesReturned = 0;
        return DeviceIoControl(handle.get(),
                               IOCTL_RVRSE_PING,
                               nullptr,
                               0,
                               nullptr,
                               0,
                               &bytesReturned,
                               nullptr) == TRUE;
    }

    void *DriverInterface::OpenHandle()
    {
        return CreateFileW(rvrse::driver::kDeviceName,
                           GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           nullptr,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           nullptr);
    }

    void DriverInterface::CloseHandle(void *handle)
    {
        if (handle && handle != INVALID_HANDLE_VALUE)
        {
            ::CloseHandle(static_cast<HANDLE>(handle));
        }
    }
}
