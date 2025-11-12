#include "plugin_loader.h"

#include <algorithm>
#include <filesystem>
#include <iterator>
#include <string_view>
#include <system_error>
#include <utility>

namespace
{
    void LogMessage(const std::wstring &message)
    {
        OutputDebugStringW(message.c_str());
    }
}

namespace rvrse::core
{
    namespace fs = std::filesystem;

    PluginLoader::PluginLoader()
        : pluginDirectory_(ResolveDefaultDirectory())
    {
        hostServices_.RegisterMenuItem = &PluginLoader::RegisterMenuItemStub;
    }

    PluginLoader::PluginLoader(std::wstring pluginDirectory)
        : pluginDirectory_(std::move(pluginDirectory))
    {
        hostServices_.RegisterMenuItem = &PluginLoader::RegisterMenuItemStub;
    }

    PluginLoader::~PluginLoader()
    {
        UnloadPlugins();
    }

    void PluginLoader::LoadPlugins()
    {
        UnloadPlugins();

        if (pluginDirectory_.empty())
        {
            return;
        }

        std::error_code ec;
        if (!fs::exists(pluginDirectory_, ec) || !fs::is_directory(pluginDirectory_, ec))
        {
            return;
        }

        for (const auto &entry : fs::directory_iterator(pluginDirectory_, ec))
        {
            if (ec || !entry.is_regular_file())
            {
                continue;
            }

            if (!entry.path().has_extension() || entry.path().extension() != L".dll")
            {
                continue;
            }

            LoadPluginFromPath(entry.path().wstring());
        }
    }

    void PluginLoader::UnloadPlugins()
    {
        for (auto &plugin : plugins_)
        {
            if (plugin.shutdown)
            {
                plugin.shutdown();
            }
            if (plugin.module)
            {
                FreeLibrary(plugin.module);
            }
        }
        plugins_.clear();
    }

    void PluginLoader::BroadcastProcessSnapshot(const ProcessSnapshot &snapshot)
    {
        if (plugins_.empty())
        {
            return;
        }

        struct ProcessBridge
        {
            std::wstring imageName;
            std::vector<RvrseThreadInfo> threads;
            RvrseProcessInfo info{};
        };

        std::vector<ProcessBridge> bridges;
        bridges.reserve(snapshot.Processes().size());

        for (const auto &process : snapshot.Processes())
        {
            ProcessBridge bridge;
            bridge.imageName = process.imageName;
            bridge.info.processId = process.processId;
            bridge.info.threadCount = process.threadCount;
            bridge.info.workingSetBytes = process.workingSetBytes;
            bridge.info.privateBytes = process.privateBytes;
            bridge.info.kernelTime100ns = process.kernelTime100ns;
            bridge.info.userTime100ns = process.userTime100ns;

            bridge.threads.reserve(process.threads.size());
            for (const auto &thread : process.threads)
            {
                RvrseThreadInfo info{};
                info.threadId = thread.threadId;
                info.owningProcessId = thread.owningProcessId;
                info.priority = thread.priority;
                info.state = thread.state;
                info.waitReason = thread.waitReason;
                info.kernelTime100ns = thread.kernelTime100ns;
                info.userTime100ns = thread.userTime100ns;
                bridge.threads.push_back(info);
            }

            bridges.push_back(std::move(bridge));
        }

        std::vector<RvrseProcessInfo> processInfos;
        processInfos.reserve(bridges.size());
        for (auto &bridge : bridges)
        {
            bridge.info.imageName = bridge.imageName.c_str();
            bridge.info.threads = bridge.threads.empty() ? nullptr : bridge.threads.data();
            bridge.info.threadEntryCount = bridge.threads.size();
            processInfos.push_back(bridge.info);
        }

        RvrseProcessSnapshotView view{};
        view.processes = processInfos.empty() ? nullptr : processInfos.data();
        view.processCount = processInfos.size();

        for (auto &plugin : plugins_)
        {
            if (plugin.hooks.OnProcessSnapshot)
            {
                plugin.hooks.OnProcessSnapshot(&view, plugin.hooks.context);
            }
        }
    }

    void PluginLoader::BroadcastHandleSnapshot(const HandleSnapshot &snapshot)
    {
        if (plugins_.empty())
        {
            return;
        }

        std::vector<RvrseHandleInfo> handleInfos;
        handleInfos.reserve(snapshot.Handles().size());

        for (const auto &handle : snapshot.Handles())
        {
            RvrseHandleInfo info{};
            info.processId = handle.processId;
            info.handleValue = handle.handleValue;
            info.objectTypeIndex = handle.objectTypeIndex;
            info.attributes = handle.attributes;
            info.grantedAccess = handle.grantedAccess;
            handleInfos.push_back(info);
        }

        RvrseHandleSnapshotView view{};
        view.handles = handleInfos.empty() ? nullptr : handleInfos.data();
        view.handleCount = handleInfos.size();

        for (auto &plugin : plugins_)
        {
            if (plugin.hooks.OnHandleSnapshot)
            {
                plugin.hooks.OnHandleSnapshot(&view, plugin.hooks.context);
            }
        }
    }

    std::wstring PluginLoader::ResolveDefaultDirectory() const
    {
        wchar_t pathBuffer[MAX_PATH] = {0};
        DWORD result = GetModuleFileNameW(nullptr, pathBuffer, static_cast<DWORD>(std::size(pathBuffer)));
        if (result == 0 || result == std::size(pathBuffer))
        {
            return {};
        }

        fs::path exePath(pathBuffer);
        fs::path pluginsPath = exePath.parent_path() / L"plugins";
        return pluginsPath.wstring();
    }

    void PluginLoader::LoadPluginFromPath(const std::wstring &path)
    {
        HMODULE module = LoadLibraryW(path.c_str());
        if (!module)
        {
            std::wstring message = L"[PluginLoader] Failed to load ";
            message += path;
            message += L"\n";
            LogMessage(message);
            return;
        }

        auto initialize = reinterpret_cast<RvrsePluginInitializeFn>(
            GetProcAddress(module, "RvrsePluginInitialize"));
        if (!initialize)
        {
            std::wstring message = L"[PluginLoader] Missing RvrsePluginInitialize in ";
            message += path;
            message += L"\n";
            LogMessage(message);
            FreeLibrary(module);
            return;
        }

        auto shutdown = reinterpret_cast<RvrsePluginShutdownFn>(
            GetProcAddress(module, "RvrsePluginShutdown"));

        PluginInstance instance{};
        instance.module = module;
        instance.path = path;

        if (!initialize(&hostServices_, &instance.info, &instance.hooks))
        {
            std::wstring message = L"[PluginLoader] Initialization failed for ";
            message += path;
            message += L"\n";
            LogMessage(message);
            FreeLibrary(module);
            return;
        }

        if (instance.info.apiMajor != RVRSE_PLUGIN_API_VERSION_MAJOR)
        {
            std::wstring message = L"[PluginLoader] API version mismatch for ";
            message += path;
            message += L"\n";
            LogMessage(message);
            if (shutdown)
            {
                shutdown();
            }
            FreeLibrary(module);
            return;
        }

        instance.shutdown = shutdown;
        plugins_.push_back(std::move(instance));
    }

    void PluginLoader::RegisterMenuItemStub(const wchar_t *menuPath,
                                            RvrsePluginMenuCommand,
                                            void *)
    {
        std::wstring message = L"[PluginLoader] Menu registration stub called for ";
        message += (menuPath ? menuPath : L"(null)");
        message += L"\n";
        LogMessage(message);
    }
}
