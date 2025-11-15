#define NOMINMAX
#include <windows.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cwchar>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <system_error>
#include <unordered_set>
#include <utility>
#include <vector>

#include "process_snapshot.h"
#include "network_snapshot.h"
#include "driver_interface.h"
#include "driver_service.h"
#include "handle_snapshot.h"
#include "plugin_loader.h"
#include "rvrse/common/formatting.h"
#include "rvrse/common/string_utils.h"
#include "rvrse/common/time_utils.h"

namespace
{
    int g_failures = 0;
    LARGE_INTEGER g_qpcFrequency{};
    std::wstring g_perfExportPath;
    std::wstring g_buildConfiguration;

    // Forward declarations
    void ReportFailure(const wchar_t *message);

    struct BenchmarkResult
    {
        std::wstring name;
        double averageMs;
        double thresholdMs;
        int iterations;
        bool passed;
    };

    std::vector<BenchmarkResult> g_benchmarkResults;

    std::wstring GetEnvironmentVariable(const wchar_t *name)
    {
        if (const wchar_t *value = _wgetenv(name))
        {
            return value;
        }
        return {};
    }

    void TestNetworkSnapshot()
    {
        auto snapshot = rvrse::core::NetworkSnapshot::Capture();
        const auto &connections = snapshot.Connections();
        if (!connections.empty())
        {
            std::uint32_t lastPid = connections.front().owningProcessId;
            for (const auto &connection : connections)
            {
                if (connection.owningProcessId < lastPid)
                {
                    ReportFailure(L"Network connections were not sorted by process ID.");
                    break;
                }
                lastPid = connection.owningProcessId;
            }

            // Verify IPv4 and IPv6 family assignment is consistent
            for (const auto &connection : connections)
            {
                if (connection.family != rvrse::core::AddressFamily::IPv4 &&
                    connection.family != rvrse::core::AddressFamily::IPv6)
                {
                    ReportFailure(L"Network connection has invalid address family.");
                    break;
                }
            }
        }

        DWORD currentPid = GetCurrentProcessId();
        auto forCurrent = snapshot.ConnectionsForProcess(currentPid);
        for (const auto &connection : forCurrent)
        {
            if (connection.owningProcessId != currentPid)
            {
                ReportFailure(L"ConnectionsForProcess returned entries that did not match the requested PID.");
                break;
            }
        }

        auto count = snapshot.ConnectionCountForProcess(currentPid);
        if (count != forCurrent.size())
        {
            ReportFailure(L"ConnectionCountForProcess did not match the number of filtered entries.");
        }
    }

    void TestDriverInterface()
    {
        auto status = rvrse::core::DriverInterface::EnsureDriverAvailable();
        (void)status;

        auto service = rvrse::core::DriverService::QueryStatus();
        (void)service;
    }

    std::string FormatDecimal(double value)
    {
        std::ostringstream stream;
        stream << std::fixed << std::setprecision(4) << value;
        return stream.str();
    }

    void RecordBenchmarkResult(const wchar_t *name,
                               double averageMs,
                               double thresholdMs,
                               int iterations,
                               bool passed)
    {
        g_benchmarkResults.push_back(
            BenchmarkResult{
                std::wstring{name},
                averageMs,
                thresholdMs,
                iterations,
                passed});
    }

    bool ExportBenchmarkTelemetry()
    {
        if (g_perfExportPath.empty() || g_benchmarkResults.empty())
        {
            return true;
        }

        namespace fs = std::filesystem;
        fs::path outputPath(g_perfExportPath);

        std::error_code directoryStatus;
        if (outputPath.has_parent_path())
        {
            fs::create_directories(outputPath.parent_path(), directoryStatus);
            if (directoryStatus)
            {
                ReportFailure(L"Failed to create directory for performance telemetry export.");
                return false;
            }
        }

        std::ofstream stream(outputPath, std::ios::binary | std::ios::trunc);
        if (!stream)
        {
            ReportFailure(L"Failed to open performance telemetry output file.");
            return false;
        }

        auto now = std::chrono::system_clock::now();
        auto timestampWide = rvrse::common::FormatTimestamp(now);
        std::string timestamp = rvrse::common::WideToUtf8(timestampWide);
        std::replace(timestamp.begin(), timestamp.end(), ' ', 'T');
        timestamp.append("Z");

        wchar_t computerName[MAX_COMPUTERNAME_LENGTH + 1]{};
        DWORD computerNameSize = static_cast<DWORD>(std::size(computerName));
        std::string hostName;
        if (GetComputerNameW(computerName, &computerNameSize))
        {
            hostName = rvrse::common::WideToUtf8(std::wstring(computerName, computerNameSize));
        }

        stream << "{\n";
        stream << "  \"metadata\": {\n";
        stream << "    \"timestamp\": \"" << timestamp << "\"";

        if (!hostName.empty())
        {
            stream << ",\n    \"machine\": \"" << hostName << "\"";
        }

        if (!g_buildConfiguration.empty())
        {
            stream << ",\n    \"build_configuration\": \""
                   << rvrse::common::WideToUtf8(g_buildConfiguration) << "\"";
        }

        stream << "\n  },\n";
        stream << "  \"benchmarks\": [\n";

        for (std::size_t index = 0; index < g_benchmarkResults.size(); ++index)
        {
            const auto &result = g_benchmarkResults[index];
            stream << "    {\n";
            stream << "      \"name\": \"" << rvrse::common::WideToUtf8(result.name) << "\",\n";
            stream << "      \"iterations\": " << result.iterations << ",\n";
            stream << "      \"average_ms\": " << FormatDecimal(result.averageMs) << ",\n";
            stream << "      \"threshold_ms\": " << FormatDecimal(result.thresholdMs) << ",\n";
            stream << "      \"status\": \"" << (result.passed ? "pass" : "fail") << "\"\n";
            stream << "    }" << (index + 1 == g_benchmarkResults.size() ? "" : ",") << "\n";
        }

        stream << "  ]\n";
        stream << "}\n";
        stream.flush();

        if (!stream)
        {
            ReportFailure(L"Failed while writing performance telemetry output file.");
            return false;
        }

        std::fwprintf(stdout,
                      L"[PERF] Exported telemetry to %s\n",
                      outputPath.wstring().c_str());
        return true;
    }

    void ReportFailure(const wchar_t *message)
    {
        ++g_failures;
        std::fwprintf(stderr, L"[FAIL] %s\n", message);
    }

    template <typename Callable>
    double MeasureAverageMilliseconds(Callable &&callable, int iterations)
    {
        if (g_qpcFrequency.QuadPart == 0)
        {
            QueryPerformanceFrequency(&g_qpcFrequency);
        }

        LARGE_INTEGER start{}, end{};
        auto fn = std::forward<Callable>(callable);
        QueryPerformanceCounter(&start);
        for (int i = 0; i < iterations; ++i)
        {
            fn();
        }
        QueryPerformanceCounter(&end);

        double elapsedSeconds = static_cast<double>(end.QuadPart - start.QuadPart) /
                                static_cast<double>(g_qpcFrequency.QuadPart);
        return (elapsedSeconds * 1000.0) / iterations;
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
        struct TrimCase
        {
            const wchar_t *input;
            const wchar_t *expected;
        } trimCases[] = {
            {L"  Rvrse Monitor \r\n", L"Rvrse Monitor"},
            {L"\t\n ", L""},
            {L"", L""},
            {L"\u2002UnicodeSpace\u2002", L"UnicodeSpace"},
        };

        for (const auto &test : trimCases)
        {
            auto trimmed = rvrse::common::TrimWhitespace(test.input);
            if (trimmed != test.expected)
            {
                ReportFailure(L"TrimWhitespace failed for provided input.");
            }
        }

        auto lower = rvrse::common::ToLower(L"MIXEDß");
        if (lower != L"mixedß")
        {
            ReportFailure(L"ToLower failed to convert extended characters.");
        }

        struct PathCase
        {
            const wchar_t *input;
            const wchar_t *expected;
        } pathCases[] = {
            {L"c:/temp//sub\\file.txt ", L"C:\\temp\\sub\\file.txt"},
            {L"\\\\server//share\\folder\\", L"\\\\server\\share\\folder"},
            {L"C:\\", L"C:\\"},
            {L"", L""},
        };

        for (const auto &test : pathCases)
        {
            auto normalized = rvrse::common::NormalizePath(test.input);
            if (normalized != test.expected)
            {
                ReportFailure(L"NormalizePath failed for provided input.");
            }
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

        auto longDuration = rvrse::common::FormatDuration(hours(99) + minutes(59) + seconds(59));
        if (longDuration != L"99:59:59.000")
        {
            ReportFailure(L"FormatDuration failed for large span.");
        }

        system_clock::time_point known = system_clock::time_point(seconds(1609459200));
        auto timestamp = rvrse::common::FormatTimestamp(known);
        if (timestamp != L"2021-01-01 00:00:00")
        {
            ReportFailure(L"FormatTimestamp did not produce expected UTC time.");
        }

        auto midnight = rvrse::common::FormatTimestamp(system_clock::time_point(seconds(0)));
        if (midnight != L"1970-01-01 00:00:00")
        {
            ReportFailure(L"FormatTimestamp failed for unix epoch.");
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

        auto modules = rvrse::core::ProcessSnapshot::EnumerateModules(currentPid);
        if (modules.empty())
        {
            ReportFailure(L"Enumerating modules for current process returned zero entries.");
        }
        else
        {
            std::uintptr_t previousBase = modules.front().baseAddress;
            for (size_t index = 1; index < modules.size(); ++index)
            {
                if (modules[index].baseAddress < previousBase)
                {
                    ReportFailure(L"Module entries were not sorted by base address.");
                    break;
                }
                previousBase = modules[index].baseAddress;
            }
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

    void TestProcessTreeEnumeration()
    {
        auto snapshot = rvrse::core::ProcessSnapshot::Capture();
        if (snapshot.Processes().empty())
        {
            ReportFailure(L"Process snapshot returned zero processes for tree enumeration test.");
            return;
        }

        // Build a set of all valid PIDs for parent validation
        std::unordered_set<std::uint32_t> validPids;
        for (const auto &process : snapshot.Processes())
        {
            validPids.insert(process.processId);
        }

        // System Idle Process (PID 0) should have parent 0
        const auto *idleProcess = std::find_if(
            snapshot.Processes().begin(),
            snapshot.Processes().end(),
            [](const rvrse::core::ProcessEntry &p) { return p.processId == 0; });

        if (idleProcess != snapshot.Processes().end())
        {
            if (idleProcess->parentProcessId != 0)
            {
                wchar_t buffer[256];
                std::swprintf(buffer,
                              std::size(buffer),
                              L"System Idle Process has parent PID %u (expected 0).",
                              idleProcess->parentProcessId);
                ReportFailure(buffer);
            }
        }

        // Verify current process has valid parent relationship
        DWORD currentPid = GetCurrentProcessId();
        const auto *currentProcess = std::find_if(
            snapshot.Processes().begin(),
            snapshot.Processes().end(),
            [currentPid](const rvrse::core::ProcessEntry &p) { return p.processId == currentPid; });

        if (currentProcess != snapshot.Processes().end())
        {
            if (currentProcess->parentProcessId != 0 &&
                validPids.find(currentProcess->parentProcessId) == validPids.end())
            {
                wchar_t buffer[256];
                std::swprintf(buffer,
                              std::size(buffer),
                              L"Current process (PID %u) has orphaned parent PID %u.",
                              currentPid,
                              currentProcess->parentProcessId);
                // This is a warning, not a failure, as the parent may have exited
                std::fwprintf(stdout, L"[WARN] %s\n", buffer);
            }
        }
        else
        {
            ReportFailure(L"Current process was not found in snapshot for tree enumeration test.");
            return;
        }

        // Verify parent-child relationships and detect direct children
        std::size_t processesWithChildren = 0;
        std::size_t totalChildRelationships = 0;

        for (const auto &parent : snapshot.Processes())
        {
            std::vector<std::uint32_t> children;
            for (const auto &candidate : snapshot.Processes())
            {
                if (candidate.parentProcessId == parent.processId && candidate.processId != 0)
                {
                    children.push_back(candidate.processId);
                }
            }

            if (!children.empty())
            {
                ++processesWithChildren;
                totalChildRelationships += children.size();
            }
        }

        if (totalChildRelationships == 0)
        {
            ReportFailure(L"No parent-child process relationships detected in snapshot.");
        }

        std::fwprintf(stdout,
                      L"[INFO] Process tree: %zu processes with children, %zu total child relationships.\n",
                      processesWithChildren,
                      totalChildRelationships);
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

    void BenchmarkProcessSnapshot()
    {
        const int iterations = 5;
        const double thresholdMs = 150.0;
        double averageMs = MeasureAverageMilliseconds(
            []()
            {
                auto snapshot = rvrse::core::ProcessSnapshot::Capture();
                std::wstring lastName;
                if (!snapshot.Processes().empty())
                {
                    lastName = snapshot.Processes().back().imageName;
                }
                (void)lastName;
            },
            iterations);

        std::fwprintf(stdout, L"[PERF] ProcessSnapshot avg: %.2f ms\n", averageMs);
        const bool passed = averageMs <= thresholdMs;
        if (!passed)
        {
            ReportFailure(L"ProcessSnapshot performance regression detected.");
        }

        RecordBenchmarkResult(L"ProcessSnapshot",
                              averageMs,
                              thresholdMs,
                              iterations,
                              passed);
    }

    void BenchmarkHandleSnapshot()
    {
        const int iterations = 5;
        const double thresholdMs = 200.0;
        double averageMs = MeasureAverageMilliseconds(
            []()
            {
                auto handles = rvrse::core::HandleSnapshot::Capture();
                volatile std::size_t count = handles.Handles().size();
                (void)count;
            },
            iterations);

        std::fwprintf(stdout, L"[PERF] HandleSnapshot avg: %.2f ms\n", averageMs);
        const bool passed = averageMs <= thresholdMs;
        if (!passed)
        {
            ReportFailure(L"HandleSnapshot performance regression detected.");
        }

        RecordBenchmarkResult(L"HandleSnapshot",
                              averageMs,
                              thresholdMs,
                              iterations,
                              passed);
    }

    void BenchmarkNetworkSnapshot()
    {
        const int iterations = 5;
        const double thresholdMs = 10.0;
        double averageMs = MeasureAverageMilliseconds(
            []()
            {
                auto snapshot = rvrse::core::NetworkSnapshot::Capture();
                volatile std::size_t count = snapshot.Connections().size();
                (void)count;
            },
            iterations);

        std::fwprintf(stdout, L"[PERF] NetworkSnapshot avg: %.2f ms\n", averageMs);
        const bool passed = averageMs <= thresholdMs;
        if (!passed)
        {
            ReportFailure(L"NetworkSnapshot performance regression detected.");
        }

        RecordBenchmarkResult(L"NetworkSnapshot",
                              averageMs,
                              thresholdMs,
                              iterations,
                              passed);
    }

    void BenchmarkUtf8Conversion()
    {
        const int iterations = 1000;
        const std::wstring sample = L"Rvrse Monitor UTF Benchmark \u2603";

        double toUtf8Ms = MeasureAverageMilliseconds(
            [&]()
            {
                auto utf8 = rvrse::common::WideToUtf8(sample);
                (void)utf8;
            },
            iterations);

        double toWideMs = MeasureAverageMilliseconds(
            [&]()
            {
                auto wide = rvrse::common::Utf8ToWide(rvrse::common::WideToUtf8(sample));
                (void)wide;
            },
            iterations);

        std::fwprintf(stdout, L"[PERF] WideToUtf8 avg: %.4f ms, Utf8ToWide avg: %.4f ms\n",
                      toUtf8Ms,
                      toWideMs);

        const double thresholdMs = 5.0;
        const bool utf8Passed = toUtf8Ms <= thresholdMs;
        const bool widePassed = toWideMs <= thresholdMs;

        if (!utf8Passed || !widePassed)
        {
            ReportFailure(L"UTF-8 conversion performance regression detected.");
        }

        RecordBenchmarkResult(L"WideToUtf8",
                              toUtf8Ms,
                              thresholdMs,
                              iterations,
                              utf8Passed);
        RecordBenchmarkResult(L"Utf8ToWide",
                              toWideMs,
                              thresholdMs,
                              iterations,
                              widePassed);
    }

    void TestPluginLoaderInitialization()
    {
        rvrse::core::PluginLoader loader(L".\\nonexistent_plugins_path");
        loader.LoadPlugins();

        auto snapshot = rvrse::core::ProcessSnapshot::Capture();
        auto handles = rvrse::core::HandleSnapshot::Capture();

        loader.BroadcastProcessSnapshot(snapshot);
        loader.BroadcastHandleSnapshot(handles);
    }
}

int wmain(int argc, wchar_t **argv)
{
    for (int i = 1; i < argc; ++i)
    {
        std::wstring argument = argv[i];
        const std::wstring perfPrefix = L"--perf-json=";
        const std::wstring buildPrefix = L"--build-config=";

        if (argument.rfind(perfPrefix, 0) == 0)
        {
            g_perfExportPath = argument.substr(perfPrefix.size());
            continue;
        }

        if (argument.rfind(buildPrefix, 0) == 0)
        {
            g_buildConfiguration = argument.substr(buildPrefix.size());
            continue;
        }
    }

    if (g_perfExportPath.empty())
    {
        g_perfExportPath = GetEnvironmentVariable(L"RVRSE_PERF_JSON");
    }

    if (g_buildConfiguration.empty())
    {
        g_buildConfiguration = GetEnvironmentVariable(L"RVRSE_BUILD_CONFIG");
    }

    if (g_buildConfiguration.empty())
    {
        try
        {
            g_buildConfiguration = std::filesystem::current_path().filename().wstring();
        }
        catch (...)
        {
        }
    }

    std::fwprintf(stdout, L"[TEST] Running Rvrse Monitor smoke tests...\n");

    TestFormatSize();
    TestFormatSizeMaxValues();
    TestStringHelpers();
    TestTimeFormatting();
    TestProcessSnapshot();
    TestProcessSnapshotEdgeCases();
    TestProcessTreeEnumeration();
    TestHandleSnapshot();
    TestHandleSnapshotAccessDenied();
    BenchmarkProcessSnapshot();
    BenchmarkHandleSnapshot();
    BenchmarkNetworkSnapshot();
    BenchmarkUtf8Conversion();
    TestPluginLoaderInitialization();
    TestNetworkSnapshot();
    TestDriverInterface();

    ExportBenchmarkTelemetry();

    if (g_failures == 0)
    {
        std::fwprintf(stdout, L"[PASS] All tests succeeded.\n");
        return 0;
    }

    std::fwprintf(stderr, L"[FAIL] %d tests failed.\n", g_failures);
    return 1;
}
