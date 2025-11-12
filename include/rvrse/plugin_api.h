#pragma once

#include <cstdint>

#if defined(_WIN32)
#define RVRSE_PLUGIN_API __declspec(dllexport)
#else
#define RVRSE_PLUGIN_API
#endif

#define RVRSE_PLUGIN_API_VERSION_MAJOR 1
#define RVRSE_PLUGIN_API_VERSION_MINOR 0

#ifdef __cplusplus
extern "C" {
#endif
    struct RvrsePluginInfo
    {
        const wchar_t *name;
        const wchar_t *author;
        const wchar_t *version;
        std::uint32_t apiMajor;
        std::uint32_t apiMinor;
    };

    struct RvrseProcessSnapshotView
    {
        const void *processEntries;
        std::size_t processCount;
    };

    struct RvrseHandleSnapshotView
    {
        const void *handleEntries;
        std::size_t handleCount;
    };

    struct RvrsePluginCallbacks
    {
        void (*OnProcessSnapshot)(const RvrseProcessSnapshotView *snapshot);
        void (*OnHandleSnapshot)(const RvrseHandleSnapshotView *snapshot);
        void (*RegisterMenuItem)(const wchar_t *menuPath,
                                 void (*OnCommand)(std::uint32_t processId));
    };

    typedef bool (*RvrsePluginInitializeFn)(const RvrsePluginCallbacks *callbacks,
                                            RvrsePluginInfo *outInfo);
    typedef void (*RvrsePluginShutdownFn)();

    RVRSE_PLUGIN_API bool RvrsePluginInitialize(const RvrsePluginCallbacks *callbacks,
                                                RvrsePluginInfo *outInfo);
    RVRSE_PLUGIN_API void RvrsePluginShutdown();

#ifdef __cplusplus
}
#endif
