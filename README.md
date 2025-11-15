# Rvrse Monitor
[![Build and Test](https://github.com/CoderRvrse/Rvrse-Monitor/actions/workflows/build-and-test.yml/badge.svg)](https://github.com/CoderRvrse/Rvrse-Monitor/actions)
[![Code Coverage](https://codecov.io/gh/CoderRvrse/Rvrse-Monitor/branch/main/graph/badge.svg)](https://codecov.io/gh/CoderRvrse/Rvrse-Monitor)

Native Windows system monitor inspired by System Informer, built entirely with C/C++ and Visual Studio tooling. The goal is to deliver a legally clean fork that keeps the upstream performance profile while adding new UX and safety features.

## Layout

- `RvrseMonitor.sln` – main Visual Studio solution.
- `src/app` – Win32 entry point plus UI scaffolding.
- `src/core` – low-level NT/Win32 wrappers and data collection.
- `src/common` – shared helpers (logging, strings, config).
- `src/plugins` – extensibility SDK and sample DLLs.
- `src/driver` – optional kernel components.
- `include` – public headers shared across modules.
- `docs` – build, design, and comparison notes.
- `scripts` – repeatable build helpers.

## Quick Start

1. Install Visual Studio 2022 with the **Desktop development with C++** workload and the latest Windows 10/11 SDK.
2. Run `scripts\build_release_local.cmd [Release|Debug|All]` from a Developer Command Prompt, or open `RvrseMonitor.sln` and build the `x64` configurations. The script now builds the requested configuration(s) and executes the smoke-test console app.
3. Launch `build\<Config>\RvrseMonitorApp.exe` to exercise the native UI that lists live processes, supports filtering, and surfaces summary details.

See `docs/build/local-project.md` for full details once populated.

## Features

- Live process grid with sorting, filtering, and handle/thread summaries.
- **Kill/Terminate Process (Tier 4):** Right-click context menu with "Terminate Process" and "Terminate Process Tree" options to gracefully kill individual processes or entire process hierarchies with protection against system-critical processes.
- Module viewer window (double-click a process or click **Modules...**) that lists every loaded DLL with base address, image size, and full path.
- Network connections explorer (click **Connections...**) with full **IPv4 and IPv6 dual-stack support** that shows TCP/UDP endpoints per process, live connection states, and formatted endpoint details [addr]:port (requires elevation to inspect system-wide sockets).
- Real-time CPU and memory graphs rendered with GDI, updating alongside the 4 s snapshot cadence so you can spot spikes instantly.
- Optional kernel driver scaffold with user-mode health checks (ping/version IOCTLs) so privileged telemetry can plug in later.
- Plugin system with a working Sample Logger that consumes process and handle broadcasts.
- JSON performance telemetry exports plus automated Release packaging for reproducible builds.

## Continuous Integration

- `.github/workflows/build-and-test.yml` runs on GitHub Actions for every push and pull request against `main`, plus manual dispatches.
- Each job checks out the repository on a `windows-latest` runner, restores dependencies via MSBuild, builds the solution for both `x64|Release` and `x64|Debug`, and runs the `RvrseMonitorTests.exe` smoke tests from `build\<Config>`.
- Successful Release builds upload the generated `.exe` binaries as workflow artifacts (`RvrseMonitor-Release-<commit>`), which can be downloaded from the run summary for quick manual validation.
- Release legs also install OpenCppCoverage, re-run the smoke tests under instrumentation, and attach a Cobertura report (`coverage-<commit>`) plus an optional Codecov upload (enabled by defining the `CODECOV_TOKEN` repository secret).
- Use the workflow to validate changes on clean Microsoft-hosted infrastructure without needing a local Visual Studio install.

### Code Coverage

1. The workflow automatically generates `coverage.xml` via OpenCppCoverage for the Release build and publishes it as a downloadable artifact.
2. Coverage uploads to [Codecov](https://codecov.io/gh/CoderRvrse/Rvrse-Monitor) using `codecov/codecov-action@v4`; for private forks, add a `CODECOV_TOKEN` repository secret so uploads succeed. Configure reporting thresholds via `.github/codecov.yml`.

## Documentation & Contributing

**For Remote Developers (Claude Code):**
- `docs/START_HERE.md` 🎯 **NEW TEAM MEMBERS START HERE!** Complete 30-minute quick start guide
- `docs/CLAUDE_CODE_GUIDE.md` ⭐ Learn how to use the todo list system and work effectively with Claude Code
- `docs/TEAM_ONBOARDING.md` – Complete onboarding guide for remote development
- `docs/FEATURE_ROADMAP.md` – 3-month plan to 90% Process Hacker parity (15 features)
- `docs/TESTING_CHECKLIST.md` – Mandatory testing procedures before every push
- `scripts/validate_before_push.ps1` – Automated pre-push validation (run this before every push!)

**General Documentation:**
- `docs/build/local-project.md` – detailed local build instructions.
- `docs/testing.md` – testing strategy, coverage expectations, benchmark guidance, and manual QA checklist.
- `docs/contributing.md` – onboarding, workflow, coding standards, and PR checklist.
- `docs/plugins.md` – plugin ABI roadmap, entry points, and loader plans.
- `docs/driver/scaffold.md` – how to build/install the optional kernel-mode companion driver.
- `docs/releases.md` – versioning rules, packaging instructions, and GitHub Release automation.
- `CHANGELOG.md` – Keep-a-Changelog history that tracks every notable feature tier.

## Plugins

- Runtime plugins live under `build\<Config>\plugins\`. The solution currently builds `SampleLogger.dll`, which subscribes to process/handle snapshots and writes basic telemetry to `sample_logger.log`.
- Implement new plugins by referencing `include/rvrse/plugin_api.h` and exporting `RvrsePluginInitialize` / `RvrsePluginShutdown`.
- The host automatically loads plugins at startup and broadcasts snapshots after each refresh; future iterations will expose additional host services (menu registration, commands).

## Release Packaging

- Run `pwsh scripts/get_version.ps1` to see the version string derived from git tags (expects `v*` tags, falls back to commit metadata when untagged).
- After building `Release`, invoke `pwsh scripts/package_release.ps1` to generate `dist/RvrseMonitor-<version>.zip` plus matching SHA256 manifest files.
- GitHub Actions runs the same packaging step for the Release configuration and uploads the zip/checksum as workflow artifacts; pushing a tag automatically converts those artifacts into a published GitHub Release.
