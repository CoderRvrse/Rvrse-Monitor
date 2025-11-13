#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace rvrse::core
{
    struct ModuleEntry
    {
        std::wstring name;
        std::wstring path;
        std::uintptr_t baseAddress = 0;
        std::uint32_t sizeBytes = 0;
    };

    struct ThreadEntry
    {
        std::uint32_t threadId = 0;
        std::uint32_t owningProcessId = 0;
        std::int32_t priority = 0;
        std::uint32_t state = 0;
        std::uint32_t waitReason = 0;
        std::uint64_t kernelTime100ns = 0;
        std::uint64_t userTime100ns = 0;
    };

    struct ProcessEntry
    {
        std::wstring imageName;
        std::uint32_t processId = 0;
        std::uint32_t threadCount = 0;
        std::uint64_t workingSetBytes = 0;
        std::uint64_t privateBytes = 0;
        std::uint64_t kernelTime100ns = 0;
        std::uint64_t userTime100ns = 0;
        std::vector<ThreadEntry> threads;
    };

    class ProcessSnapshot
    {
    public:
        ProcessSnapshot() = default;

        static ProcessSnapshot Capture();
        static std::vector<ModuleEntry> EnumerateModules(std::uint32_t processId);

        const std::vector<ProcessEntry> &Processes() const { return processes_; }

    private:
        std::vector<ProcessEntry> processes_;
    };
}
