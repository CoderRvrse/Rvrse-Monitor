# Changelog

All notable changes to this project will be documented in this file. The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project adheres to [Semantic Versioning](https://semver.org/) once stable releases begin.

## [Unreleased]
### Added
- Version-aware release packaging script that bundles binaries, documentation, and SHA256 manifests.
- CI automation that uploads release zips and publishes GitHub releases whenever a tag (`v*`) is pushed.
- Real-time CPU and memory graphs inside the summary pane, fed by live system telemetry.
- Module viewer window that surfaces every DLL loaded by the selected process complete with base address, size, and path metadata.
- Per-process network connections viewer built on a new `NetworkSnapshot` capture that enumerates **IPv4 and IPv6** TCP/UDP endpoints with dual-stack support, proper address formatting ([addr]:port for IPv6), and graceful elevation detection.
- Driver scaffold (shared IOCTL contract, user-mode interface, kernel skeleton) so privileged telemetry can plug in incrementally.
- **Kill/Terminate Process feature (Tier 4 - Feature 1):**
  - Right-click context menu on process list with "Terminate Process" and "Terminate Process Tree" options.
  - Support for terminating individual processes using the Windows `TerminateProcess()` API.
  - Recursive process tree termination that kills all child processes while respecting parent-child relationships.
  - Parent PID tracking in `ProcessSnapshot` for accurate process hierarchy enumeration.
  - Robust error handling for system-protected processes (System, csrss.exe, services.exe, lsass.exe, svchost.exe).
  - User confirmation dialogs before terminating any process to prevent accidental termination.
  - Success/failure feedback messages with count of terminated and failed processes.
  - Unit tests validating process tree enumeration and child process collection.

### Changed
- Documented the release workflow so contributors can cut local builds that match the CI output.

## [v0.2.0] - 2025-02-17
### Added
- Tier 1 foundation: testing infrastructure, contributor documentation, plugin API, and sample plugin implementation.
- Tier 1.2 plugin loader integration throughout the app plus end-to-end sample logger coverage.
- Tier 2.2 performance telemetry export with JSON artifacts from every benchmark run.

### Fixed
- Numerous edge cases in process/thread/handle snapshotting, including ordering, duplication checks, and regression thresholds.
