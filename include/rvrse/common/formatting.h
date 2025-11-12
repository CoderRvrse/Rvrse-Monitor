#pragma once

#include <cstdint>
#include <string>

namespace rvrse::common
{
    // Formats a byte count into a short human-readable string (e.g., "12.3 MB").
    std::wstring FormatSize(std::uint64_t bytes);
}
