#include "rvrse/common/formatting.h"

#include <array>
#include <cmath>
#include <sstream>
#include <iomanip>

namespace rvrse::common
{
    std::wstring FormatSize(std::uint64_t bytes)
    {
        static constexpr std::array<const wchar_t *, 5> kUnits = {
            L"B", L"KB", L"MB", L"GB", L"TB"};

        double value = static_cast<double>(bytes);
        size_t unitIndex = 0;

        while (value >= 1024.0 && unitIndex + 1 < kUnits.size())
        {
            value /= 1024.0;
            ++unitIndex;
        }

        std::wostringstream stream;
        stream << std::fixed << std::setprecision(unitIndex == 0 ? 0 : 1) << value << L' ' << kUnits[unitIndex];
        return stream.str();
    }
}
