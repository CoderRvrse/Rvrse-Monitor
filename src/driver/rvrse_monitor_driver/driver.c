#include <ntddk.h>
#include "rvrse/driver_protocol.h"

DRIVER_UNLOAD RvrseDriverUnload;
_Function_class_(DRIVER_INITIALIZE) NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);
_Dispatch_type_(IRP_MJ_CREATE) _Dispatch_type_(IRP_MJ_CLOSE) _Dispatch_type_(IRP_MJ_DEVICE_CONTROL)
DRIVER_DISPATCH RvrseDispatch;

static UNICODE_STRING g_deviceName;
static UNICODE_STRING g_dosDeviceName;

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING)
{
    UNREFERENCED_PARAMETER(DriverObject);

    RtlInitUnicodeString(&g_deviceName, RVRSE_KERNEL_DEVICE_PATH);
    RtlInitUnicodeString(&g_dosDeviceName, RVRSE_DOS_DEVICE_PATH);

    PDEVICE_OBJECT deviceObject = NULL;
    NTSTATUS status = IoCreateDevice(DriverObject,
                                     0,
                                     &g_deviceName,
                                     FILE_DEVICE_UNKNOWN,
                                     0,
                                     FALSE,
                                     &deviceObject);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = IoCreateSymbolicLink(&g_dosDeviceName, &g_deviceName);
    if (!NT_SUCCESS(status))
    {
        IoDeleteDevice(deviceObject);
        return status;
    }

    DriverObject->DriverUnload = RvrseDriverUnload;
    for (ULONG index = 0; index <= IRP_MJ_MAXIMUM_FUNCTION; ++index)
    {
        DriverObject->MajorFunction[index] = RvrseDispatch;
    }

    deviceObject->Flags |= DO_BUFFERED_IO;
    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    return STATUS_SUCCESS;
}

VOID RvrseDriverUnload(PDRIVER_OBJECT DriverObject)
{
    IoDeleteSymbolicLink(&g_dosDeviceName);
    IoDeleteDevice(DriverObject->DeviceObject);
}

NTSTATUS RvrseDispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    auto &stack = Irp->Tail.Overlay.CurrentStackLocation;
    NTSTATUS status = STATUS_SUCCESS;
    ULONG_PTR information = 0;

    switch (stack.MajorFunction)
    {
    case IRP_MJ_CREATE:
    case IRP_MJ_CLOSE:
        status = STATUS_SUCCESS;
        break;
    case IRP_MJ_DEVICE_CONTROL:
    {
        switch (stack.Parameters.DeviceIoControl.IoControlCode)
        {
        case IOCTL_RVRSE_PING:
            status = STATUS_SUCCESS;
            break;
        case IOCTL_RVRSE_QUERY_VERSION:
            if (stack.Parameters.DeviceIoControl.OutputBufferLength < sizeof(RVRSE_DRIVER_VERSION))
            {
                status = STATUS_BUFFER_TOO_SMALL;
            }
            else
            {
                auto version = (PRVRSE_DRIVER_VERSION)Irp->AssociatedIrp.SystemBuffer;
                version->major = RVRSE_DRIVER_VERSION_MAJOR;
                version->minor = RVRSE_DRIVER_VERSION_MINOR;
                information = sizeof(RVRSE_DRIVER_VERSION);
                status = STATUS_SUCCESS;
            }
            break;
        default:
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }
        break;
    }
    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = information;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}
