#define NOMINMAX
#include <windows.h>

#include <cstdio>
#include <cwchar>
#include <string>

#include "process_snapshot.h"
#include "handle_snapshot.h"
#include "rvrse/common/formatting.h"

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
}

int wmain()
{
    std::fwprintf(stdout, L"[TEST] Running Rvrse Monitor smoke tests...\n");

    TestFormatSize();
    TestProcessSnapshot();
    TestHandleSnapshot();

    if (g_failures == 0)
    {
        std::fwprintf(stdout, L"[PASS] All tests succeeded.\n");
        return 0;
    }

    std::fwprintf(stderr, L"[FAIL] %d tests failed.\n", g_failures);
    return 1;
}
