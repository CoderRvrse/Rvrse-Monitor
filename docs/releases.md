# Release Workflow

Tier 2.3 introduces a repeatable packaging pipeline so we can distribute signed builds once we are ready for beta testing. This document explains how versions are derived, what the generated artifacts contain, and how GitHub releases are produced.

## Version Detection

- `scripts/get_version.ps1` inspects the git history and emits a SemVer-friendly string:
  - Tags must be prefixed with `v` (for example `v0.2.0`).
  - Commits ahead of a tag produce build metadata such as `0.2.0+3.ab12cde`.
  - Dirty worktrees append `.dirty` so accidental local state never sneaks into official builds.
- Run it directly for a quick check: `pwsh scripts/get_version.ps1`.
- Pass `-Json` or `-AsObject` if you need additional metadata (commit hashes, describe output, etc.).

## Packaging Locally

1. Build the desired configuration (Release by default): `scripts\build_release_local.cmd Release`.
2. Run the packager from the repo root:

   ```powershell
   pwsh scripts/package_release.ps1 -Configuration Release -OutputDirectory dist
   ```

3. The script will:
   - Stage `RvrseMonitorApp.exe`, `RvrseMonitorTests.exe`, and compiled plugins from `build\<Config>`.
   - Copy `README.md`, `LICENSE`, and `CHANGELOG.md`.
   - Add `VERSION.txt` plus an automatically generated `SHA256SUMS.txt` that covers every staged file.
   - Produce `dist/RvrseMonitor-<version>.zip` along with `dist/RvrseMonitor-<version>.zip.sha256`.

The resulting zip is self-contained and ready for sharing with external testers.

## GitHub Actions Integration

- `.github/workflows/build-and-test.yml` now invokes the packager during the Release leg. The zip and checksum are uploaded as `release-package-<sha>` artifacts on every run.
- Tag pushes (`refs/tags/v*`) trigger an additional job that downloads the packaged artifacts and publishes a GitHub Release using the tag name. Release bodies can be edited afterward to add richer notes, but the automation guarantees the binaries and hashes are attached immediately.

## Release Contents

Each archive contains:

- `RvrseMonitorApp.exe` – signed desktop UI.
- `RvrseMonitorTests.exe` – smoke/benchmark test harness.
- `plugins\SampleLogger.dll` (and any other compiled plugins).
- `README.md`, `LICENSE`, `CHANGELOG.md`, `VERSION.txt`, and `SHA256SUMS.txt`.

Consumers can verify integrity by running `Get-FileHash` (or `sha256sum`) against any file and matching it with the entries inside `SHA256SUMS.txt` or the standalone `*.zip.sha256` companion file.
