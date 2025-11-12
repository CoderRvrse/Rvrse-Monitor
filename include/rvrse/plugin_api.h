#pragma once

#include <cstddef>
#include <cstdint>

#if defined(_WIN32)
#define RVRSE_PLUGIN_EXPORT extern "C" __declspec(dllexport)
#else
#define RVRSE_PLUGIN_EXPORT extern "C"
#endif

#define RVRSE_PLUGIN_API_VERSION_MAJOR 1U
#define RVRSE_PLUGIN_API_VERSION_MINOR 0U

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RvrsePluginInfo
{
    const wchar_t *name;
    const wchar_t *author;
    const wchar_t *version;
    std::uint32_t apiMajor;
    std::uint32_t apiMinor;
} RvrsePluginInfo;

typedef struct RvrseThreadInfo
{
    std::uint32_t threadId;
    std::uint32_t owningProcessId;
    std::int32_t priority;
    std::uint32_t state;
    std::uint32_t waitReason;
    std::uint64_t kernelTime100ns;
    std::uint64_t userTime100ns;
} RvrseThreadInfo;

typedef struct RvrseProcessInfo
{
    const wchar_t *imageName;
    std::uint32_t processId;
    std::uint32_t threadCount;
    std::uint64_t workingSetBytes;
    std::uint64_t privateBytes;
    std::uint64_t kernelTime100ns;
    std::uint64_t userTime100ns;
    const RvrseThreadInfo *threads;
    std::size_t threadEntryCount;
} RvrseProcessInfo;

typedef struct RvrseHandleInfo
{
    std::uint32_t processId;
    std::uint16_t handleValue;
    std::uint16_t objectTypeIndex;
    std::uint32_t attributes;
    std::uint32_t grantedAccess;
} RvrseHandleInfo;

typedef struct RvrseProcessSnapshotView
{
    const RvrseProcessInfo *processes;
    std::size_t processCount;
} RvrseProcessSnapshotView;

typedef struct RvrseHandleSnapshotView
{
    const RvrseHandleInfo *handles;
    std::size_t handleCount;
} RvrseHandleSnapshotView;

typedef void (*RvrsePluginMenuCommand)(std::uint32_t processId, void *context);

typedef struct RvrseHostServices
{
    void (*RegisterMenuItem)(const wchar_t *menuPath,
                             RvrsePluginMenuCommand command,
                             void *context);
} RvrseHostServices;

typedef struct RvrsePluginHooks
{
    void (*OnProcessSnapshot)(const RvrseProcessSnapshotView *snapshot, void *context);
    void (*OnHandleSnapshot)(const RvrseHandleSnapshotView *snapshot, void *context);
    void *context;
} RvrsePluginHooks;

typedef bool (*RvrsePluginInitializeFn)(const RvrseHostServices *hostServices,
                                        RvrsePluginInfo *outInfo,
                                        RvrsePluginHooks *outHooks);
typedef void (*RvrsePluginShutdownFn)();

RVRSE_PLUGIN_EXPORT bool RvrsePluginInitialize(const RvrseHostServices *hostServices,
                                               RvrsePluginInfo *outInfo,
                                               RvrsePluginHooks *outHooks);
RVRSE_PLUGIN_EXPORT void RvrsePluginShutdown();

#ifdef __cplusplus
}
#endif
