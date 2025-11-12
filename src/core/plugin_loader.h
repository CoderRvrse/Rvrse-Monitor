#pragma once

#include <memory>
#include <string>
#include <vector>

#include <windows.h>

#include "handle_snapshot.h"
#include "process_snapshot.h"
#include "rvrse/plugin_api.h"

namespace rvrse::core
{
    class PluginLoader
    {
    public:
        PluginLoader();
        explicit PluginLoader(std::wstring pluginDirectory);
        ~PluginLoader();

        void LoadPlugins();
        void UnloadPlugins();

        void BroadcastProcessSnapshot(const ProcessSnapshot &snapshot);
        void BroadcastHandleSnapshot(const HandleSnapshot &snapshot);

    private:
        struct PluginInstance
        {
            HMODULE module = nullptr;
            std::wstring path;
            RvrsePluginInfo info{};
            RvrsePluginHooks hooks{};
            RvrsePluginShutdownFn shutdown = nullptr;
        };

        std::wstring ResolveDefaultDirectory() const;
        void LoadPluginFromPath(const std::wstring &path);

        static void RegisterMenuItemStub(const wchar_t *menuPath,
                                         RvrsePluginMenuCommand command,
                                         void *context);

        std::wstring pluginDirectory_;
        std::vector<PluginInstance> plugins_;
        RvrseHostServices hostServices_{};
    };
}
