#define NOMINMAX
#include <windows.h>

#include <cstdio>
#include <cwchar>
#include <filesystem>
#include <iterator>
#include <string>

#include "rvrse/plugin_api.h"

namespace
{
    std::wstring GetLogPath()
    {
        wchar_t modulePath[MAX_PATH] = {0};
        HMODULE module = nullptr;
        if (!GetModuleHandleExW(
                GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                reinterpret_cast<LPCWSTR>(&GetLogPath),
                &module))
        {
            return L"sample_logger.log";
        }

        GetModuleFileNameW(module, modulePath, static_cast<DWORD>(std::size(modulePath)));
        std::filesystem::path path(modulePath);
        return (path.parent_path() / L"sample_logger.log").wstring();
    }

    void AppendLogLine(const std::wstring &line)
    {
        static const std::wstring logPath = GetLogPath();
        FILE *file = nullptr;
        _wfopen_s(&file, logPath.c_str(), L"a+, ccs=UTF-8");
        if (!file)
        {
            return;
        }

        std::fwprintf(file, L"%s\n", line.c_str());
        std::fclose(file);
    }

    void OnProcessSnapshot(const RvrseProcessSnapshotView *snapshot, void *)
    {
        std::size_t processCount = snapshot ? snapshot->processCount : 0;
        wchar_t buffer[128];
        std::swprintf(buffer,
                      std::size(buffer),
                      L"[SampleLogger] Processes observed: %zu",
                      processCount);
        AppendLogLine(buffer);
    }

    void OnHandleSnapshot(const RvrseHandleSnapshotView *snapshot, void *)
    {
        std::size_t handleCount = snapshot ? snapshot->handleCount : 0;
        wchar_t buffer[128];
        std::swprintf(buffer,
                      std::size(buffer),
                      L"[SampleLogger] Handles observed: %zu",
                      handleCount);
        AppendLogLine(buffer);
    }
}

RVRSE_PLUGIN_EXPORT bool RvrsePluginInitialize(const RvrseHostServices *,
                                               RvrsePluginInfo *outInfo,
                                               RvrsePluginHooks *outHooks)
{
    if (!outInfo || !outHooks)
    {
        return false;
    }

    static const wchar_t kName[] = L"Sample Logger Plugin";
    static const wchar_t kAuthor[] = L"Rvrse Monitor";
    static const wchar_t kVersion[] = L"1.0.0";

    outInfo->name = kName;
    outInfo->author = kAuthor;
    outInfo->version = kVersion;
    outInfo->apiMajor = RVRSE_PLUGIN_API_VERSION_MAJOR;
    outInfo->apiMinor = RVRSE_PLUGIN_API_VERSION_MINOR;

    outHooks->OnProcessSnapshot = &OnProcessSnapshot;
    outHooks->OnHandleSnapshot = &OnHandleSnapshot;
    outHooks->context = nullptr;

    AppendLogLine(L"[SampleLogger] Initialized");
    return true;
}

RVRSE_PLUGIN_EXPORT void RvrsePluginShutdown()
{
    AppendLogLine(L"[SampleLogger] Shutdown");
}
