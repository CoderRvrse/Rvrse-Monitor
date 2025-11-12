# Contributing Guide

Welcome! This document explains how to set up your environment, follow the project workflow, and land high-quality changes. Read this before opening pull requests so we can keep the main branch stable.

## Prerequisites

- Windows 10/11 x64 development machine.
- Visual Studio 2022 with the **Desktop development with C++** workload and Windows 10/11 SDK 10.0.22621+.
- Git for Windows.
- Optional: OpenCppCoverage (`choco install opencppcoverage --yes`) if you plan to generate coverage locally.
- See `docs/build/local-project.md` for detailed build instructions.

## Repository Layout

- `RvrseMonitor.sln` – solution file for all C/C++ projects.
- `src/common`, `src/core`, `src/app`, `src/plugins` – primary code modules.
- `tests` – smoke tests, benchmarks, and soon unit-test harnesses.
- `scripts` – build helpers (run `scripts\build_release_local.cmd` during development).
- `docs` – architecture, build, and testing references (including this guide).

## Workflow

1. **Fork + Clone**
   ```cmd
   git clone https://github.com/<you>/Rvrse-Monitor.git
   cd Rvrse-Monitor
   git remote add upstream https://github.com/CoderRvrse/Rvrse-Monitor.git
   ```
2. **Create a feature branch**
   ```cmd
   git checkout -b feature/<short-summary>
   ```
3. **Make your changes** (code, docs, tests).
4. **Run tests + benchmarks**
   ```cmd
   scripts\build_release_local.cmd All
   ```
   Ensure `[PASS]` and `[PERF]` outputs look healthy (see `docs/testing.md`).
5. **Check coverage** (Release build) if your change touches `src/common` or `src/core`. Local command:
   ```cmd
   OpenCppCoverage.exe --sources src --sources include ^
     --export_type cobertura:coverage.xml ^
     --working_dir build\Release ^
     --target build\Release\RvrseMonitorTests.exe
   ```
6. **Commit with a clear message**
   ```cmd
   git commit -am "core: add plugin loader skeleton"
   ```
7. **Push + open a PR**
   ```cmd
   git push origin feature/<short-summary>
   ```
   Fill out the PR template, link issues, and paste relevant test/benchmark logs.

## Coding Guidelines

- **Language**: C++17 for native code, modern STL where appropriate.
- **Warnings**: Build clean with MSVC `/W4` (already enforced). Do not introduce new warnings.
- **Headers**: Place shared declarations in `include/rvrse/...`, include via those paths.
- **Formatting**: Follow existing style (brace placement, indentation). No trailing whitespace.
- **Unicode**: Prefer wide strings (`std::wstring`) for Windows API interactions.
- **Error handling**: Use lightweight helpers or early returns; log where appropriate (logging helpers coming soon).
- **Tests**: Every new helper/function should ship with coverage in `tests/main.cpp` (or future dedicated suites).
- **Benchmarks**: If you touch performance-sensitive code, add/update `Benchmark*` functions so regressions trigger.

## Pull Request Checklist

Before requesting review:

- [ ] All builds pass locally (`Release`, `Debug`).
- [ ] `RvrseMonitorTests.exe` exits with code 0 and benchmark output looks reasonable.
- [ ] Added/updated tests cover new logic; coverage trend does not regress.
- [ ] Documentation updated (README, `docs/testing.md`, etc.) when workflows or expectations change.
- [ ] No unrelated file churn; commits are scoped to the feature/fix.
- [ ] PR description includes:
  - Summary of changes.
  - Testing/benchmark log snippets.
  - Screenshots for UI changes (if applicable).

## Handling CI Failures

- Click the failing workflow in GitHub Actions and review logs.
- Re-run locally using the same commands (`scripts\build_release_local.cmd Release`).
- For coverage failures, download the `coverage-<commit>.xml` artifact and inspect in a coverage viewer (Codecov UI or VS extension).
- Benchmark regressions must be investigated; update thresholds **only** with justification.

## Staying in Sync

Periodically rebase against upstream `main` to avoid large merge conflicts:

```cmd
git fetch upstream
git checkout main
git merge upstream/main
git checkout feature/<branch>
git rebase main
```

Resolve conflicts locally, rerun tests, and force-push (`git push -f`) to update your PR.

## Communication

- Use GitHub Issues/PRs for feature discussions and bug reports.
- Tag reviewers when ready; draft PRs are encouraged for early feedback.
- Large architectural changes (plugin system, driver work, UI overhauls) should be proposed via an issue or design doc before coding.

## When in Doubt

- Consult `docs/testing.md` and `docs/build/local-project.md`.
- Look at recent commits for examples of style and structure.
- Ask questions via issues/PR comments—better to clarify than guess.
