# Changelog

All notable changes to this project will be documented in this file. The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project adheres to [Semantic Versioning](https://semver.org/) once stable releases begin.

## [Unreleased]
### Added
- Version-aware release packaging script that bundles binaries, documentation, and SHA256 manifests.
- CI automation that uploads release zips and publishes GitHub releases whenever a tag (`v*`) is pushed.
- Real-time CPU and memory graphs inside the summary pane, fed by live system telemetry.
- Module viewer window that surfaces every DLL loaded by the selected process complete with base address, size, and path metadata.
- Per-process network connections viewer built on a new `NetworkSnapshot` capture that enumerates IPv4 TCP/UDP endpoints (with graceful elevation detection and user feedback).
- Driver scaffold (shared IOCTL contract, user-mode interface, kernel skeleton) so privileged telemetry can plug in incrementally.

### Changed
- Documented the release workflow so contributors can cut local builds that match the CI output.

## [v0.2.0] - 2025-02-17
### Added
- Tier 1 foundation: testing infrastructure, contributor documentation, plugin API, and sample plugin implementation.
- Tier 1.2 plugin loader integration throughout the app plus end-to-end sample logger coverage.
- Tier 2.2 performance telemetry export with JSON artifacts from every benchmark run.

### Fixed
- Numerous edge cases in process/thread/handle snapshotting, including ordering, duplication checks, and regression thresholds.
