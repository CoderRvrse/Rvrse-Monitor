# System Monitor Architecture (Draft)

## Goals

1. Stay as close as possible to System Informer abstractions so upstream fixes can be ported.
2. Keep a clear separation between data collection (core/driver) and presentation (ui) to enable future UI refreshes without touching low-level code.
3. Provide obvious extension points for plugins without forcing consumers to dig through internal headers.

## Proposed layout

- `src/core` – NT system wrappers, process/thread/module/handle enumeration, data caching, notification pumps.
- `src/common` – shared utilities: trace logging, string formatting, config persistence, security helpers.
- `src/ui` – Win32/WTL front end: main window, process tree, property dialogs, graphs.
- `src/plugins` – plugin SDK headers plus built-in samples; uses a stable C ABI mirroring upstream `phlib`.
- `src/driver` – optional signed kernel components for advanced telemetry or handle operations.
- `include` – headers exposed across modules (e.g., plugin contracts, product constants).

Core libraries should build as static libs/DLLs that feed the UI project. Keep inter-module communication in terms of opaque handles or small POD structs so ABI churn is minimal.

## Capability buckets

| Capability                 | Status      | Notes                                                      |
|---------------------------|-------------|------------------------------------------------------------|
| Process/thread/handle UI  | MVP         | Core experience; reuse upstream data model terminology.    |
| Module/service viewers    | MVP         | Enables parity with upstream properties dialogs.           |
| Memory + CPU graphs       | MVP         | Native GDI plots inside summary pane.                      |
| Network connections       | v1.x        | After MVP; depends on stable table and filtering UI.       |
| Disk and I/O monitoring   | v1.x        | Requires ETW integration; plan as extension module.        |
| Plugin system             | MVP         | Lightweight API for sampling features.                     |
| Snapshot/diff utilities   | v2+         | Build once core enumeration is rock solid.                 |
| Safety guardrails         | v1.x        | Read-only mode, protected process warnings.                |

The summary pane now renders lightweight CPU and memory graphs via the `ResourceGraphView` control in `src/app/main.cpp`, giving immediate visual feedback without introducing a heavyweight charting dependency.

## Data flow

1. `core` polls NT APIs (or subscribes to callbacks) and publishes immutable snapshots.
2. `ui` subscribes to snapshot events via a message bus / observer pattern.
3. Plugins can register for either raw NT data (for analytics) or UI hooks (for contextual menus).
4. Drivers feed privileged data back into `core` through IOCTL contracts.

## Immediate action items

- Stand up dedicated static library projects for `core` and `common`.
- Define plugin ABI header in `include/` with versioning macros.
- Mirror System Informer’s `kph` driver layout once user space pieces mature.
