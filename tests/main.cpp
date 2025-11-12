#define NOMINMAX
#include <windows.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cwchar>
#include <limits>
#include <string>
#include <unordered_set>

#include "process_snapshot.h"
#include "handle_snapshot.h"
#include "rvrse/common/formatting.h"
#include "rvrse/common/string_utils.h"
#include "rvrse/common/time_utils.h"

namespace
{
    int g_failures = 0;

    void ReportFailure(const wchar_t *message)
    {
        ++g_failures;
        std::fwprintf(stderr, L"[FAIL] %s\n", message);
    }

    void TestFormatSize()
    {
        struct TestCase
        {
            std::uint64_t bytes;
            const wchar_t *expected;
        } cases[] = {
            {512, L"512 B"},
            {1536, L"1.5 KB"},
            {1572864, L"1.5 MB"},
        };

        for (const auto &test : cases)
        {
            auto result = rvrse::common::FormatSize(test.bytes);
            if (result != test.expected)
            {
                wchar_t buffer[256];
                std::swprintf(buffer, std::size(buffer), L"FormatSize(%llu) => %s (expected %s)",
                              static_cast<unsigned long long>(test.bytes),
                              result.c_str(),
                              test.expected);
                ReportFailure(buffer);
            }
        }
    }

    void TestFormatSizeMaxValues()
    {
        struct TestCase
        {
            std::uint64_t bytes;
            const wchar_t *expected;
        } cases[] = {
            {0, L"0 B"},
            {1023, L"1023 B"},
            {1024, L"1.0 KB"},
            {1048576, L"1.0 MB"},
            {static_cast<std::uint64_t>(1) << 40, L"1.0 TB"},
            {std::numeric_limits<std::uint64_t>::max(), L"16777216.0 TB"},
        };

        for (const auto &test : cases)
        {
            auto result = rvrse::common::FormatSize(test.bytes);
            if (result != test.expected)
            {
                wchar_t buffer[256];
                std::swprintf(buffer,
                              std::size(buffer),
                              L"[FormatSize] %llu => %s (expected %s)",
                              static_cast<unsigned long long>(test.bytes),
                              result.c_str(),
                              test.expected);
                ReportFailure(buffer);
            }
        }
    }

    void TestStringHelpers()
    {
        auto trimmed = rvrse::common::TrimWhitespace(L"  Rvrse Monitor \r\n");
        if (trimmed != L"Rvrse Monitor")
        {
            ReportFailure(L"TrimWhitespace failed to remove surrounding whitespace.");
        }

        auto emptyTrim = rvrse::common::TrimWhitespace(L"\t\n ");
        if (!emptyTrim.empty())
        {
            ReportFailure(L"TrimWhitespace should return empty for whitespace-only strings.");
        }

        auto lower = rvrse::common::ToLower(L"MixedCase 123");
        if (lower != L"mixedcase 123")
        {
            ReportFailure(L"ToLower failed to convert string.");
        }

        auto normalized = rvrse::common::NormalizePath(L"c:/temp//sub\\file.txt ");
        if (normalized != L"C:\\temp\\sub\\file.txt")
        {
            ReportFailure(L"NormalizePath did not collapse separators or uppercase drive letter.");
        }

        const char *utf8Input = u8"Rvrse \u2603";
        auto wide = rvrse::common::Utf8ToWide(utf8Input);
        if (wide != L"Rvrse \u2603")
        {
            ReportFailure(L"Utf8ToWide failed to convert UTF-8 payload.");
        }

        auto roundtrip = rvrse::common::WideToUtf8(wide);
        if (roundtrip != utf8Input)
        {
            ReportFailure(L"WideToUtf8 failed round-trip conversion.");
        }
    }

    void TestTimeFormatting()
    {
        using namespace std::chrono;

        auto duration = rvrse::common::FormatDuration(milliseconds(3723456));
        if (duration != L"01:02:03.456")
        {
            ReportFailure(L"FormatDuration failed for positive span.");
        }

        auto negative = rvrse::common::FormatDuration(milliseconds(-1890));
        if (negative != L"-00:00:01.890")
        {
            ReportFailure(L"FormatDuration failed for negative span.");
        }

        system_clock::time_point known = system_clock::time_point(seconds(1609459200));
        auto timestamp = rvrse::common::FormatTimestamp(known);
        if (timestamp != L"2021-01-01 00:00:00")
        {
            ReportFailure(L"FormatTimestamp did not produce expected UTC time.");
        }
    }

    void TestProcessSnapshot()
    {
        auto snapshot = rvrse::core::ProcessSnapshot::Capture();
        if (snapshot.Processes().empty())
        {
            ReportFailure(L"Process snapshot returned zero processes.");
            return;
        }

        DWORD currentPid = GetCurrentProcessId();
        bool foundSelf = false;
        for (const auto &process : snapshot.Processes())
        {
            if (process.processId == currentPid)
            {
                foundSelf = true;
                if (process.threads.empty())
                {
                    ReportFailure(L"Current process did not report any thread entries.");
                }
                break;
            }
        }

        if (!foundSelf)
        {
            ReportFailure(L"Current process was not present in snapshot.");
        }
    }

    void TestProcessSnapshotEdgeCases()
    {
        auto snapshot = rvrse::core::ProcessSnapshot::Capture();
        if (snapshot.Processes().empty())
        {
            ReportFailure(L"Process snapshot returned zero processes for edge-case coverage.");
            return;
        }

        std::unordered_set<std::uint32_t> seenPids;
        std::uint32_t previousPid = 0;
        bool first = true;
        std::size_t threadEntryTotal = 0;

        for (const auto &process : snapshot.Processes())
        {
            if (!first && process.processId < previousPid)
            {
                ReportFailure(L"Process snapshot results were not sorted by process ID.");
                break;
            }

            if (!seenPids.insert(process.processId).second)
            {
                ReportFailure(L"Process snapshot returned duplicate process IDs.");
                break;
            }

            if (process.threadCount != process.threads.size())
            {
                wchar_t buffer[256];
                std::swprintf(buffer,
                              std::size(buffer),
                              L"Process %u reported %u threads but captured %zu entries.",
                              process.processId,
                              process.threadCount,
                              process.threads.size());
                ReportFailure(buffer);
                break;
            }

            if (process.threadCount > 0)
            {
                const bool ownsAtLeastOneThread = std::any_of(
                    process.threads.begin(),
                    process.threads.end(),
                    [&](const rvrse::core::ThreadEntry &thread)
                    {
                        return thread.owningProcessId == process.processId;
                    });

                if (!ownsAtLeastOneThread)
                {
                    ReportFailure(L"Process snapshot threads lost owning PID metadata.");
                    break;
                }
            }

            threadEntryTotal += process.threads.size();
            previousPid = process.processId;
            first = false;
        }

        if (threadEntryTotal == 0)
        {
            ReportFailure(L"Thread enumeration returned zero entries.");
        }
    }

    void TestHandleSnapshot()
    {
        auto handles = rvrse::core::HandleSnapshot::Capture();
        if (handles.Handles().empty())
        {
            ReportFailure(L"Handle snapshot returned zero handles.");
            return;
        }

        DWORD pid = GetCurrentProcessId();
        auto handleCount = handles.HandleCountForProcess(pid);
        if (handleCount == 0)
        {
            ReportFailure(L"Handle snapshot did not include any handles for current process.");
        }
    }

    void TestHandleSnapshotAccessDenied()
    {
        auto handles = rvrse::core::HandleSnapshot::Capture();
        if (handles.Handles().empty())
        {
            ReportFailure(L"Handle snapshot returned zero handles for access coverage.");
            return;
        }

        DWORD currentPid = GetCurrentProcessId();
        auto handlesForSelf = handles.HandlesForProcess(currentPid);
        auto countForSelf = handles.HandleCountForProcess(currentPid);
        if (handlesForSelf.size() != countForSelf)
        {
            ReportFailure(L"Handle snapshot count and filtered size diverged for current process.");
        }

        const std::uint32_t invalidPid = std::numeric_limits<std::uint32_t>::max();
        if (!handles.HandlesForProcess(invalidPid).empty() || handles.HandleCountForProcess(invalidPid) != 0)
        {
            ReportFailure(L"Handle snapshot returned entries for an invalid PID.");
        }
    }
}

int wmain()
{
    std::fwprintf(stdout, L"[TEST] Running Rvrse Monitor smoke tests...\n");

    TestFormatSize();
    TestFormatSizeMaxValues();
    TestStringHelpers();
    TestTimeFormatting();
    TestProcessSnapshot();
    TestProcessSnapshotEdgeCases();
    TestHandleSnapshot();
    TestHandleSnapshotAccessDenied();

    if (g_failures == 0)
    {
        std::fwprintf(stdout, L"[PASS] All tests succeeded.\n");
        return 0;
    }

    std::fwprintf(stderr, L"[FAIL] %d tests failed.\n", g_failures);
    return 1;
}
