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

- `RvrseProcessSnapshotView` – lightweight description of all processes; each entry surfaces PIDs, memory counters, timing info, and an array of `RvrseThreadInfo`.
- `RvrseHandleSnapshotView` – flattened handle list with owning PID, type indices, and granted access rights.
- Treat both views as read-only and ephemeral; do not store pointers once the callback returns. Additional views (modules, services, network) will join as the core layer exposes them.

## Callback Table

- `RvrsePluginHooks` (returned by plugins) currently exposes:
  - `OnProcessSnapshot` – invoked after each snapshot capture (UI refresh cadence).
  - `OnHandleSnapshot` – invoked alongside handle captures.
- `RvrseHostServices` (provided by the host) currently only includes a placeholder `RegisterMenuItem` stub; future iterations will route UI commands through this surface.

Plugins should treat all callbacks as optional: check for `nullptr` before invoking and avoid storing snapshot pointers beyond the scope of the call.

## Loader Plan

1. `PluginLoader` (`src/core/plugin_loader.*`) scans `build\<Config>\plugins` for DLLs, loads them, validates the ABI version, and dispatches process/handle snapshots after each refresh.
2. Sample plugin: `src/plugins/sample_logger` builds into `build\<Config>\plugins\SampleLogger.dll` and logs snapshot counts to `sample_logger.log`.
3. Expose plugin enable/disable controls in the UI (Phase 2).

## Safety Considerations

- Plugins run in-process; crashes will bring down the app. Keep the ABI minimal and document best practices.
- Consider sandboxing or permission prompts for untrusted plugins in future releases.
- Loader should guard against:
  - Version mismatches.
  - Missing entry points.
  - Initialization failures (log and continue).

## Next Steps

1. Expand `plugin_api.h` as additional data (modules, services, etc.) becomes available.
2. Wire UI controls to host services (menu items, commands) instead of the current stub.
3. Add automated tests that exercise real plugins in a temporary directory.
4. Document best practices for plugin authors (threading, logging, error handling).
