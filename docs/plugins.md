# Plugin System (Draft)

This document tracks the Tier 1.2 roadmap for the plugin architecture. The initial goal is to provide a stable C ABI that allows DLLs to consume process/handle snapshots and extend the UI without touching internal headers.

## ABI Versioning

- Header: `include/rvrse/plugin_api.h`.
- Version macros:
  - `RVRSE_PLUGIN_API_VERSION_MAJOR`
  - `RVRSE_PLUGIN_API_VERSION_MINOR`
- Plugins must check that `apiMajor` matches before registering callbacks. Minor version increments indicate additive changes.

## Entry Points

| Symbol | Description |
| ------ | ----------- |
| `RvrsePluginInitialize` | Called once after the DLL loads. Receives callback table and fills out `RvrsePluginInfo`. Return `false` to abort loading gracefully. |
| `RvrsePluginShutdown` | Called before unload to release resources. |

Future hooks (Phase 2):

- `RvrsePluginGetMenuItems` / `PluginRegisterMenuItem` for context-menu extensions.
- Snapshot diff hooks once the API stabilizes.

## Data Views

- `RvrseProcessSnapshotView` – opaque pointer/count pair referencing the current process entries. Actual structures live inside `src/core`; plugins treat them as read-only and must not retain pointers past the callback.
- `RvrseHandleSnapshotView` – similar pattern for handles.
- Additional views (modules, services, network) will join as the core layer exposes them.

## Callback Table

`RvrsePluginCallbacks` currently exposes:

- `OnProcessSnapshot` – invoked after each snapshot capture (UI refresh cadence).
- `OnHandleSnapshot` – invoked alongside handle captures.
- `RegisterMenuItem` – placeholder for upcoming UI integration; accepts a hierarchical menu path and a callback invoked with the selected process ID.

Plugins should treat callbacks as optional: check for `nullptr` before invoking.

## Loader Plan

1. Implement `PluginLoader` in `src/core` that:
   - Scans `plugins/` (or `%PROGRAMDATA%\RvrseMonitor\Plugins`) for DLLs.
   - Loads each DLL with `LoadLibraryEx`.
   - Resolves `RvrsePluginInitialize`/`RvrsePluginShutdown`.
   - Passes a callback table with safe wrappers.
   - Tracks loaded plugins for later unload.
2. Provide a sample plugin under `src/plugins/sample_logger` demonstrating logging snapshots to a file.
3. Expose plugin enable/disable controls in the UI (Phase 2).

## Safety Considerations

- Plugins run in-process; crashes will bring down the app. Keep the ABI minimal and document best practices.
- Consider sandboxing or permission prompts for untrusted plugins in future releases.
- Loader should guard against:
  - Version mismatches.
  - Missing entry points.
  - Initialization failures (log and continue).

## Next Steps

1. Finalize `plugin_api.h` structure definitions (may need forward declarations once core structs are exposed).
2. Build the loader skeleton and wire it into the main app startup.
3. Author `sample_logger` plugin and accompanying documentation.
4. Expand tests to cover loader edge cases (e.g., invalid DLLs, failed initialization).
