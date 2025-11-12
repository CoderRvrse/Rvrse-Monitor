#pragma once

#include <string>
#include <string_view>

namespace rvrse::common
{
    // Removes leading/trailing whitespace characters (spaces, tabs, newlines).
    std::wstring TrimWhitespace(std::wstring_view input);

    // Converts the input to lowercase using Unicode-aware towlower.
    std::wstring ToLower(std::wstring_view input);

    // Normalizes Windows paths (backslashes, collapsed separators, uppercase drive letter).
    std::wstring NormalizePath(std::wstring_view input);

    // UTF-8 <-> UTF-16 conversions backed by Win32 helpers.
    std::wstring Utf8ToWide(std::string_view input);
    std::string WideToUtf8(std::wstring_view input);
}
