#include "process_snapshot.h"

#include <algorithm>
#include <cstddef>
#include <vector>

#include <Windows.h>
#include <winternl.h>

#pragma comment(lib, "ntdll.lib")

namespace
{
    // Fallback definitions for undocumented structures and constants.
    #ifndef STATUS_INFO_LENGTH_MISMATCH
    #define STATUS_INFO_LENGTH_MISMATCH ((NTSTATUS)0xC0000004L)
    #endif

    // Extended SYSTEM_PROCESS_INFORMATION with timing fields
    typedef struct _SYSTEM_PROCESS_INFORMATION_EX {
        ULONG NextEntryOffset;
        ULONG NumberOfThreads;
        LARGE_INTEGER WorkingSetPrivateSize;
        ULONG HardFaultCount;
        ULONG NumberOfThreadsHighWatermark;
        ULONGLONG CycleTime;
        LARGE_INTEGER CreateTime;
        LARGE_INTEGER UserTime;
        LARGE_INTEGER KernelTime;
        UNICODE_STRING ImageName;
        KPRIORITY BasePriority;
        HANDLE UniqueProcessId;
        HANDLE InheritedFromUniqueProcessId;
        ULONG HandleCount;
        ULONG SessionId;
        ULONG_PTR UniqueProcessKey;
        SIZE_T PeakVirtualSize;
        SIZE_T VirtualSize;
        ULONG PageFaultCount;
        SIZE_T PeakWorkingSetSize;
        SIZE_T WorkingSetSize;
        SIZE_T QuotaPeakPagedPoolUsage;
        SIZE_T QuotaPagedPoolUsage;
        SIZE_T QuotaPeakNonPagedPoolUsage;
        SIZE_T QuotaNonPagedPoolUsage;
        SIZE_T PagefileUsage;
        SIZE_T PeakPagefileUsage;
        SIZE_T PrivatePageCount;
        LARGE_INTEGER ReadOperationCount;
        LARGE_INTEGER WriteOperationCount;
        LARGE_INTEGER OtherOperationCount;
        LARGE_INTEGER ReadTransferCount;
        LARGE_INTEGER WriteTransferCount;
        LARGE_INTEGER OtherTransferCount;
    } SYSTEM_PROCESS_INFORMATION_EX, *PSYSTEM_PROCESS_INFORMATION_EX;

    // Extended SYSTEM_THREAD_INFORMATION with timing fields
    typedef struct _SYSTEM_THREAD_INFORMATION_EX {
        LARGE_INTEGER KernelTime;
        LARGE_INTEGER UserTime;
        LARGE_INTEGER CreateTime;
        ULONG WaitTime;
        PVOID StartAddress;
        CLIENT_ID ClientId;
        KPRIORITY Priority;
        LONG BasePriority;
        ULONG ContextSwitches;
        ULONG ThreadState;
        ULONG WaitReason;
    } SYSTEM_THREAD_INFORMATION_EX, *PSYSTEM_THREAD_INFORMATION_EX;

    class NtBuffer
    {
    public:
        std::vector<std::byte> buffer;
    };

    NtBuffer QuerySystemProcessInformation()
    {
        NtBuffer ntBuffer;
        ULONG bufferSize = 0x40000; // start with 256 KB

        while (true)
        {
            ntBuffer.buffer.resize(bufferSize);
            ULONG returnLength = 0;
            NTSTATUS status = NtQuerySystemInformation(SystemProcessInformation,
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

    std::wstring CaptureImageName(const UNICODE_STRING &imageName)
    {
        if (imageName.Length == 0 || imageName.Buffer == nullptr)
        {
            return L"System Idle Process";
        }

        return std::wstring(imageName.Buffer, imageName.Length / sizeof(wchar_t));
    }
}

namespace rvrse::core
{
    ProcessSnapshot ProcessSnapshot::Capture()
    {
        ProcessSnapshot snapshot;
        NtBuffer buffer = QuerySystemProcessInformation();

        if (buffer.buffer.empty())
        {
            return snapshot;
        }

        auto *current = reinterpret_cast<SYSTEM_PROCESS_INFORMATION_EX *>(buffer.buffer.data());

        while (true)
        {
            ProcessEntry entry{};
            entry.processId = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(current->UniqueProcessId));
            entry.threadCount = current->NumberOfThreads;
            entry.imageName = CaptureImageName(current->ImageName);
            entry.workingSetBytes = static_cast<std::uint64_t>(current->WorkingSetSize);
            entry.privateBytes = static_cast<std::uint64_t>(current->PrivatePageCount);
            entry.kernelTime100ns = static_cast<std::uint64_t>(current->KernelTime.QuadPart);
            entry.userTime100ns = static_cast<std::uint64_t>(current->UserTime.QuadPart);

            auto *threads = reinterpret_cast<SYSTEM_THREAD_INFORMATION_EX *>(current + 1);
            for (ULONG threadIndex = 0; threadIndex < current->NumberOfThreads; ++threadIndex)
            {
                const auto &nativeThread = threads[threadIndex];
                ThreadEntry threadEntry{};
                threadEntry.threadId = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(nativeThread.ClientId.UniqueThread));
                threadEntry.owningProcessId = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(nativeThread.ClientId.UniqueProcess));
                threadEntry.priority = nativeThread.Priority;
                threadEntry.state = nativeThread.ThreadState;
                threadEntry.waitReason = nativeThread.WaitReason;
                threadEntry.kernelTime100ns = static_cast<std::uint64_t>(nativeThread.KernelTime.QuadPart);
                threadEntry.userTime100ns = static_cast<std::uint64_t>(nativeThread.UserTime.QuadPart);
                entry.threads.push_back(std::move(threadEntry));
            }

            snapshot.processes_.push_back(std::move(entry));

            if (current->NextEntryOffset == 0)
            {
                break;
            }

            current = reinterpret_cast<SYSTEM_PROCESS_INFORMATION_EX *>(
                reinterpret_cast<std::byte *>(current) + current->NextEntryOffset);
        }

        std::sort(snapshot.processes_.begin(), snapshot.processes_.end(),
                  [](const ProcessEntry &lhs, const ProcessEntry &rhs)
                  {
                      return lhs.processId < rhs.processId;
                  });

        return snapshot;
    }
}
