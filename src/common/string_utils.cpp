#define NOMINMAX
#include <windows.h>

#include "rvrse/common/string_utils.h"

#include <cwctype>
#include <string_view>

namespace rvrse::common
{
    std::wstring TrimWhitespace(std::wstring_view input)
    {
        if (input.empty())
        {
            return {};
        }

        std::size_t first = 0;
        while (first < input.size() && std::iswspace(static_cast<wint_t>(input[first])))
        {
            ++first;
        }

        if (first == input.size())
        {
            return {};
        }

        std::size_t last = input.size() - 1;
        while (last > first && std::iswspace(static_cast<wint_t>(input[last])))
        {
            --last;
        }

        return std::wstring(input.substr(first, last - first + 1));
    }

    std::wstring ToLower(std::wstring_view input)
    {
        std::wstring result(input.begin(), input.end());
        for (auto &ch : result)
        {
            ch = static_cast<wchar_t>(std::towlower(ch));
        }
        return result;
    }

    std::wstring NormalizePath(std::wstring_view input)
    {
        std::wstring trimmed = TrimWhitespace(input);
        if (trimmed.empty())
        {
            return {};
        }

        std::wstring normalized;
        normalized.reserve(trimmed.size());
        bool lastWasSeparator = false;

        for (wchar_t ch : trimmed)
        {
            wchar_t mapped = (ch == L'/') ? L'\\' : ch;

            if (mapped == L'\\')
            {
                if (normalized.empty())
                {
                    normalized.push_back(mapped);
                }
                else if (!lastWasSeparator)
                {
                    normalized.push_back(mapped);
                }
                lastWasSeparator = true;
                continue;
            }

            normalized.push_back(mapped);
            lastWasSeparator = false;
        }

        if (normalized.size() > 1 && normalized.back() == L'\\' && normalized[normalized.size() - 2] != L':')
        {
            normalized.pop_back();
        }

        if (normalized.size() >= 2 && normalized[1] == L':')
        {
            normalized[0] = static_cast<wchar_t>(std::towupper(normalized[0]));
        }

        return normalized;
    }

    std::wstring Utf8ToWide(std::string_view input)
    {
        if (input.empty())
        {
            return {};
        }

        int length = MultiByteToWideChar(CP_UTF8,
                                         MB_ERR_INVALID_CHARS,
                                         input.data(),
                                         static_cast<int>(input.size()),
                                         nullptr,
                                         0);
        if (length <= 0)
        {
            return {};
        }

        std::wstring wide(length, L'\0');
        MultiByteToWideChar(CP_UTF8,
                            MB_ERR_INVALID_CHARS,
                            input.data(),
                            static_cast<int>(input.size()),
                            wide.data(),
                            length);
        return wide;
    }

    std::string WideToUtf8(std::wstring_view input)
    {
        if (input.empty())
        {
            return {};
        }

        int length = WideCharToMultiByte(CP_UTF8,
                                         WC_ERR_INVALID_CHARS,
                                         input.data(),
                                         static_cast<int>(input.size()),
                                         nullptr,
                                         0,
                                         nullptr,
                                         nullptr);
        if (length <= 0)
        {
            return {};
        }

        std::string utf8(length, '\0');
        WideCharToMultiByte(CP_UTF8,
                            WC_ERR_INVALID_CHARS,
                            input.data(),
                            static_cast<int>(input.size()),
                            utf8.data(),
                            length,
                            nullptr,
                            nullptr);
        return utf8;
    }
}
