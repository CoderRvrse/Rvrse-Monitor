#pragma once

#include <stdint.h>
#include <winioctl.h>

#define RVRSE_DEVICE_NAME            L"\\\\.\\RvrseMonitor"
#define RVRSE_KERNEL_DEVICE_PATH     L"\\Device\\RvrseMonitor"
#define RVRSE_DOS_DEVICE_PATH        L"\\DosDevices\\RvrseMonitor"
#define RVRSE_DRIVER_VERSION_MAJOR   0u
#define RVRSE_DRIVER_VERSION_MINOR   1u

#define IOCTL_RVRSE_PING CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_RVRSE_QUERY_VERSION CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _RVRSE_DRIVER_VERSION
{
    uint32_t major;
    uint32_t minor;
} RVRSE_DRIVER_VERSION, *PRVRSE_DRIVER_VERSION;

#ifdef __cplusplus
namespace rvrse::driver
{
    inline constexpr wchar_t kDeviceName[] = RVRSE_DEVICE_NAME;
    inline constexpr wchar_t kKernelDevicePath[] = RVRSE_KERNEL_DEVICE_PATH;
    inline constexpr wchar_t kDosDevicePath[] = RVRSE_DOS_DEVICE_PATH;

    inline constexpr std::uint32_t kDriverVersionMajor = RVRSE_DRIVER_VERSION_MAJOR;
    inline constexpr std::uint32_t kDriverVersionMinor = RVRSE_DRIVER_VERSION_MINOR;

    using DriverVersion = RVRSE_DRIVER_VERSION;
}
#endif
