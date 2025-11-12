#pragma once

#include <cstdint>
#include <vector>

namespace rvrse::core
{
    struct HandleEntry
    {
        std::uint32_t processId = 0;
        std::uint16_t handleValue = 0;
        std::uint16_t objectTypeIndex = 0;
        std::uint32_t attributes = 0;
        std::uint32_t grantedAccess = 0;
    };

    class HandleSnapshot
    {
    public:
        static HandleSnapshot Capture();

        const std::vector<HandleEntry> &Handles() const { return handles_; }

        std::vector<HandleEntry> HandlesForProcess(std::uint32_t processId) const;
        std::size_t HandleCountForProcess(std::uint32_t processId) const;

    private:
        std::vector<HandleEntry> handles_;
    };
}
