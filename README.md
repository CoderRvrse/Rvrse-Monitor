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

## Continuous Integration

- `.github/workflows/build-and-test.yml` runs on GitHub Actions for every push and pull request against `main`, plus manual dispatches.
- Each job checks out the repository on a `windows-latest` runner, restores dependencies via MSBuild, builds the solution for both `x64|Release` and `x64|Debug`, and runs the `RvrseMonitorTests.exe` smoke tests from `build\<Config>`.
- Successful Release builds upload the generated `.exe` binaries as workflow artifacts (`RvrseMonitor-Release-<commit>`), which can be downloaded from the run summary for quick manual validation.
- Release legs also install OpenCppCoverage, re-run the smoke tests under instrumentation, and attach a Cobertura report (`coverage-<commit>`) plus an optional Codecov upload (enabled by defining the `CODECOV_TOKEN` repository secret).
- Use the workflow to validate changes on clean Microsoft-hosted infrastructure without needing a local Visual Studio install.

### Code Coverage

1. The workflow automatically generates `coverage.xml` via OpenCppCoverage for the Release build and publishes it as a downloadable artifact.
2. Coverage uploads to [Codecov](https://codecov.io/gh/CoderRvrse/Rvrse-Monitor) using `codecov/codecov-action@v4`; for private forks, add a `CODECOV_TOKEN` repository secret so uploads succeed. Configure reporting thresholds via `.github/codecov.yml`.
