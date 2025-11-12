#include "handle_snapshot.h"

#include <cstddef>
#include <vector>

#include <Windows.h>
#include <winternl.h>

namespace
{
    // Fallback definitions for older SDKs and undocumented structures.
    #ifndef STATUS_INFO_LENGTH_MISMATCH
    #define STATUS_INFO_LENGTH_MISMATCH ((NTSTATUS)0xC0000004L)
    #endif

    // SystemHandleInformation class
    constexpr SYSTEM_INFORMATION_CLASS SystemHandleInformation = static_cast<SYSTEM_INFORMATION_CLASS>(16);

    typedef struct _SYSTEM_HANDLE_TABLE_ENTRY_INFO
    {
        USHORT UniqueProcessId;
        USHORT CreatorBackTraceIndex;
        UCHAR ObjectTypeIndex;
        UCHAR HandleAttributes;
        USHORT HandleValue;
        PVOID Object;
        ULONG GrantedAccess;
    } SYSTEM_HANDLE_TABLE_ENTRY_INFO, *PSYSTEM_HANDLE_TABLE_ENTRY_INFO;

    typedef struct _SYSTEM_HANDLE_INFORMATION
    {
        ULONG NumberOfHandles;
        SYSTEM_HANDLE_TABLE_ENTRY_INFO Handles[1];
    } SYSTEM_HANDLE_INFORMATION, *PSYSTEM_HANDLE_INFORMATION;

    class NtHandleBuffer
    {
    public:
        std::vector<std::byte> buffer;
    };

    NtHandleBuffer QuerySystemHandles()
    {
        NtHandleBuffer ntBuffer;
        ULONG bufferSize = 0x20000; // 128 KB to start

        while (true)
        {
            ntBuffer.buffer.resize(bufferSize);
            ULONG returnLength = 0;

            NTSTATUS status = NtQuerySystemInformation(SystemHandleInformation,
                                                       ntBuffer.buffer.data(),
                                                       bufferSize,
                                                       &returnLength);

            if (status == STATUS_INFO_LENGTH_MISMATCH)
            {
                bufferSize = returnLength + 0x10000;
                continue;
            }

            if (!NT_SUCCESS(status))
            {
                ntBuffer.buffer.clear();
            }

            break;
        }

        return ntBuffer;
    }
}

namespace rvrse::core
{
    HandleSnapshot HandleSnapshot::Capture()
    {
        HandleSnapshot snapshot;
        NtHandleBuffer buffer = QuerySystemHandles();

        if (buffer.buffer.empty())
        {
            return snapshot;
        }

        auto *info = reinterpret_cast<SYSTEM_HANDLE_INFORMATION *>(buffer.buffer.data());
        snapshot.handles_.reserve(info->NumberOfHandles);

        for (ULONG i = 0; i < info->NumberOfHandles; ++i)
        {
            const auto &nativeHandle = info->Handles[i];

            HandleEntry entry{};
            entry.processId = nativeHandle.UniqueProcessId;
            entry.handleValue = nativeHandle.HandleValue;
            entry.objectTypeIndex = nativeHandle.ObjectTypeIndex;
            entry.attributes = nativeHandle.HandleAttributes;
            entry.grantedAccess = nativeHandle.GrantedAccess;

            snapshot.handles_.push_back(entry);
        }

        return snapshot;
    }

    std::vector<HandleEntry> HandleSnapshot::HandlesForProcess(std::uint32_t processId) const
    {
        std::vector<HandleEntry> filtered;
        for (const auto &handle : handles_)
        {
            if (handle.processId == processId)
            {
                filtered.push_back(handle);
            }
        }
        return filtered;
    }

    std::size_t HandleSnapshot::HandleCountForProcess(std::uint32_t processId) const
    {
        std::size_t count = 0;
        for (const auto &handle : handles_)
        {
            if (handle.processId == processId)
            {
                ++count;
            }
        }
        return count;
    }
}
