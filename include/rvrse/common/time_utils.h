#pragma once

#include <chrono>
#include <string>

namespace rvrse::common
{
    // Formats durations as HH:MM:SS.mmm (with optional leading minus).
    std::wstring FormatDuration(std::chrono::milliseconds duration);

    // Formats a UTC timestamp as YYYY-MM-DD HH:MM:SS.
    std::wstring FormatTimestamp(std::chrono::system_clock::time_point timePoint);
}
