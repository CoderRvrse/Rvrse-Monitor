# Testing Guide

This document explains how we verify Rvrse Monitor across unit tests, integration checks, performance benchmarks, and manual QA. Every contribution **must** keep the workflow below green so the main branch remains releasable at all times.

## Test Layers

| Layer | Location | Purpose | Notes |
| ----- | -------- | ------- | ----- |
| Unit/Utility | `tests/main.cpp` (common helpers) | Validate pure functions (formatting, string utils, path helpers, time formatting). | Keep functions small and deterministic; avoid OS calls wherever possible. |
| Core Snapshots | `tests/main.cpp` | Smoke-test `ProcessSnapshot` and `HandleSnapshot` against the live system. | Real Windows APIs; use upcoming mock interfaces for deterministic cases. |
| Benchmarks | `tests/main.cpp` (`Benchmark*` functions) | Detect regressions in capture routines and UTF conversion helpers. | Thresholds should reflect realistic desktop hardware (<150 ms process snapshot, <200 ms handle snapshot, <5 ms UTF conversions). |
| UI / Manual | `RvrseMonitorApp.exe` | Ensure Win32 UI renders, refreshes, and filters correctly. | Until UI automation lands, follow the manual checklist below. |
| CI | `.github/workflows/build-and-test.yml` | Build Debug+Release, run tests/benchmarks, generate coverage, upload artifacts. | Release leg runs OpenCppCoverage and (optionally) Codecov uploads. |

## Running Tests Locally

### Build + Run via script

```cmd
scripts\build_release_local.cmd [Debug|Release|All]
```

- Builds the requested configuration(s).
- Drops binaries in `build\<Config>\`.
- Executes `RvrseMonitorTests.exe` for each configuration.

### Direct invocation

```cmd
msbuild RvrseMonitor.sln /p:Configuration=Debug;Platform=x64 /m
cd build\Debug
RvrseMonitorTests.exe
```

Use a **Developer Command Prompt for VS** (or any shell where `vcvars64.bat` has run) so MSVC/Windows SDK tools are on `PATH`.

### Expected output

- `[PASS] All tests succeeded.` when every test and benchmark passes.
- `[PERF] …` lines with average timings; treat them as part of the regression history.
- Non-zero exit code or `[FAIL] …` lines mean the change must be fixed before submission.

## Performance Telemetry Export

- `RvrseMonitorTests.exe` accepts `--perf-json=<path>` to write a JSON summary of every benchmark (avg ms, threshold, pass/fail, iteration count). The optional `--build-config=<name>` flag (or `RVRSE_BUILD_CONFIG`) stamps the metadata so CI dashboards can differentiate Release vs Debug.
- Equivalent environment variable: set `RVRSE_PERF_JSON` before launching the test binary if you prefer not to pass command-line flags.
- Example: `RvrseMonitorTests.exe --build-config=Release --perf-json=telemetry\perf-release.json`.
- `scripts\build_release_local.cmd` now enables this automatically, generating `build\<Config>\telemetry\perf-<Config>.json`.
- GitHub Actions uploads the same files as artifacts (`perf-<Config>-<commit>.json`) so we can trend performance across runs or plug them into future regression alarms.

## Coverage Expectations

- Target: **≥70 % combined coverage** across `src/common` and `src/core` before feature freeze.
- Release CI leg runs OpenCppCoverage and publishes `coverage-<commit>.xml` artifacts plus Codecov uploads (if `CODECOV_TOKEN` is configured).
- To debug coverage locally:

  ```cmd
  choco install opencppcoverage --yes
  OpenCppCoverage.exe ^
    --sources src ^
    --sources include ^
    --export_type cobertura:coverage.xml ^
    --working_dir build\Release ^
    --target build\Release\RvrseMonitorTests.exe
  ```

- When adding a new module, update `.github/codecov.yml` if you need to adjust ignore rules.

## Benchmark Guidance

- Benchmarks currently live alongside unit tests and run automatically:
  - `BenchmarkProcessSnapshot` – 5 iterations, fail if avg >150 ms.
  - `BenchmarkHandleSnapshot` – 5 iterations, fail if avg >200 ms.
  - `BenchmarkUtf8Conversion` – 1000 iterations, fail if avg >5 ms for either direction.
- Keep benchmarks lightweight so they run quickly in CI. Prefer higher iteration counts with smaller workloads over single heavy operations.
- When changing thresholds, justify the new numbers in the PR description and update this doc.
- Review the exported telemetry (`build\<Config>\telemetry\perf-<Config>.json`) when validating performance-sensitive changes so you can attach before/after stats to PRs.

## Manual GUI Checklist

Until UI automation is implemented (Tier 2.1), run through the following before merging UI-affecting changes:

1. `cd build\Release && RvrseMonitorApp.exe`.
2. Verify the process list populates with accurate PIDs, names, threads, and memory stats.
3. Use the search/filter box; confirm results update immediately.
4. Select multiple processes and confirm the detail pane updates (working set, private bytes, thread list).
5. Click **Refresh**; confirm the timestamp/process data updates immediately.
6. Wait ≥4 seconds to confirm auto-refresh triggers and no UI freezes occur.
7. Observe handle snapshot integration (handles column updates; no crashes under load).
8. Watch Task Manager for CPU spikes—UI should remain responsive with >100 processes.

Document any deviations or flaky behavior in the PR and file issues if persistent.

## Mock Interfaces (Coming Soon)

To make `ProcessSnapshot`/`HandleSnapshot` testable without touching real system state, Tier 1.1 includes adding mockable interfaces. Once available:

- Prefer unit tests that inject mock data over direct system captures.
- Keep smoke tests (live captures) focused on integration coverage while mocks handle edge cases.
- Update this doc with instructions for using the mock factories.

## QA Responsibilities for Contributors

- Run unit tests + benchmarks locally before opening a PR.
- Ensure coverage does not regress; add targeted tests for new code paths.
- Update `docs/testing.md` whenever you add new test suites, change thresholds, or modify the workflow.
- Include benchmark output snippets in PR descriptions for perf-sensitive changes.
