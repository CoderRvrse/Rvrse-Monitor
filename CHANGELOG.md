# Changelog

All notable changes to this project will be documented in this file. The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project adheres to [Semantic Versioning](https://semver.org/) once stable releases begin.

## [Unreleased]
### Added
- Version-aware release packaging script that bundles binaries, documentation, and SHA256 manifests.
- CI automation that uploads release zips and publishes GitHub releases whenever a tag (`v*`) is pushed.
- Real-time CPU and memory graphs inside the summary pane, fed by live system telemetry.
- Module viewer window that surfaces every DLL loaded by the selected process complete with base address, size, and path metadata.
- **Network Connections View** with full IPv4 and IPv6 support:
  - Per-process network connections viewer built on `NetworkSnapshot` class
  - IPv4 TCP/UDP enumeration using `GetExtendedTcpTable` and `GetExtendedUdpTable` (AF_INET)
  - IPv6 TCP/UDP enumeration using `GetExtendedTcpTable` and `GetExtendedUdpTable` (AF_INET6)
  - 16-byte IPv6 address storage in `ConnectionEntry` structure
  - Proper IPv6 address formatting in UI with bracket notation ([addr]:port)
  - Connection state tracking for TCP (ESTABLISHED, LISTEN, SYN-SENT, etc.)
  - Per-process filtering and sorting by PID, protocol, and address
  - Performance telemetry with <10ms average capture time for both IPv4 and IPv6
  - Comprehensive unit tests for IPv4/IPv6 enumeration, filtering, and validation
  - Graceful elevation detection and user feedback for access denied scenarios
- **Kill/Terminate Process** with context menu integration:
  - Right-click context menu on process list with Terminate options
  - Single process termination using `TerminateProcess()` API with PROCESS_TERMINATE access
  - Process tree termination that recursively kills all child processes
  - Parent PID tracking in `ProcessEntry` structure via `InheritedFromUniqueProcessId`
  - Confirmation dialogs before termination with process name and PID details
  - Error handling for protected processes (System, csrss.exe, etc.) with access denied feedback
  - Success/failure feedback messages with detailed error information
  - Automatic process list refresh after termination
  - Unit tests for process tree enumeration and parent-child relationship validation
- **Process Search/Filter** with real-time filtering and visual feedback:
  - Real-time text-based filtering in process list (filters on EN_CHANGE event)
  - Case-insensitive substring matching for process names
  - Exact PID matching when filter text contains only digits
  - Clear button to quickly reset filter and show all processes
  - Visual feedback in summary panel showing "Showing X of Y processes" when filtered
  - Automatic whitespace trimming for better search accuracy
- **Suspend/Resume Process** with thread-based suspension:
  - Suspend and Resume options in right-click context menu
  - Thread-based suspension using `SuspendThread()` API with THREAD_SUSPEND_RESUME access
  - Resume functionality using `ResumeThread()` API
  - Iterates through all threads in the target process for complete suspension/resume
  - Success/failure feedback showing number of threads suspended/resumed
  - Error handling for access denied scenarios
  - Automatic process list refresh after operation
- **Process Priority Control** with confirmation dialogs:
  - Set Priority submenu in right-click context menu with 6 priority levels
  - Priority options: Realtime, High, Above Normal, Normal, Below Normal, Low
  - Uses `SetPriorityClass()` API with PROCESS_SET_INFORMATION access
  - Confirmation dialog before changing priority with readable priority names
  - Error handling for insufficient privileges (Realtime requires administrator)
  - Clear feedback messages for success and failure scenarios
  - Automatic process list refresh after successful priority change
- Driver scaffold (shared IOCTL contract, user-mode interface, kernel skeleton) so privileged telemetry can plug in incrementally.
- Claude Code SessionStart hook for automatic development environment setup
- Comprehensive Claude Code development guide with todo list best practices

### Changed
- Documented the release workflow so contributors can cut local builds that match the CI output.

## [v0.2.0] - 2025-02-17
### Added
- Tier 1 foundation: testing infrastructure, contributor documentation, plugin API, and sample plugin implementation.
- Tier 1.2 plugin loader integration throughout the app plus end-to-end sample logger coverage.
- Tier 2.2 performance telemetry export with JSON artifacts from every benchmark run.

### Fixed
- Numerous edge cases in process/thread/handle snapshotting, including ordering, duplication checks, and regression thresholds.
