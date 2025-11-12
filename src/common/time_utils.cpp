#include "rvrse/common/time_utils.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace rvrse::common
{
    std::wstring FormatDuration(std::chrono::milliseconds duration)
    {
        using Rep = std::chrono::milliseconds::rep;
        Rep totalMs = duration.count();
        bool negative = totalMs < 0;

        std::uint64_t magnitude = negative
                                      ? static_cast<std::uint64_t>(-(totalMs + 1)) + 1
                                      : static_cast<std::uint64_t>(totalMs);

        std::uint64_t hours = magnitude / (1000ULL * 60ULL * 60ULL);
        magnitude %= (1000ULL * 60ULL * 60ULL);

        std::uint64_t minutes = magnitude / (1000ULL * 60ULL);
        magnitude %= (1000ULL * 60ULL);

        std::uint64_t seconds = magnitude / 1000ULL;
        std::uint64_t milliseconds = magnitude % 1000ULL;

        std::wostringstream stream;
        if (negative)
        {
            stream << L"-";
        }

        stream << std::setfill(L'0') << std::setw(2) << hours << L":"
               << std::setw(2) << minutes << L":"
               << std::setw(2) << seconds << L"."
               << std::setw(3) << milliseconds;

        return stream.str();
    }

    std::wstring FormatTimestamp(std::chrono::system_clock::time_point timePoint)
    {
        using namespace std::chrono;
        auto timeT = system_clock::to_time_t(timePoint);

        std::tm utc{};
#if defined(_WIN32)
        gmtime_s(&utc, &timeT);
#else
        gmtime_r(&timeT, &utc);
#endif

        std::wostringstream stream;
        stream << std::put_time(&utc, L"%Y-%m-%d %H:%M:%S");
        return stream.str();
    }
}
