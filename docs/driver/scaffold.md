# Driver Scaffold

Tier 3 introduces a kernel-mode companion that can surface privileged telemetry (process security attributes, protected handle duplication, etc.). This document captures the current scaffold so contributors can extend it safely.

## Overview

- Device name: `\\Device\\RvrseMonitor` (`\\.\RvrseMonitor` from user mode).
- IOCTLs:
  - `IOCTL_RVRSE_PING` – no payload, lets user mode verify that the driver is loaded/responding.
  - `IOCTL_RVRSE_QUERY_VERSION` – returns `RVRSE_DRIVER_VERSION { major, minor }`.
- Source: `src/driver/rvrse_monitor_driver/driver.c` (plain WDM skeleton).
- Shared contract: `include/rvrse/driver_protocol.h` (C-friendly macros with C++ wrappers).

## Building the Driver

1. Install Visual Studio 2022 + the matching Windows Driver Kit (WDK).
2. In VS, create a new *Empty Kernel Mode Driver (KMDF)* project under `src/driver/rvrse_monitor_driver` or add `driver.c` to an existing KM solution.
3. Ensure these properties are set per configuration:
   - `ConfigurationType=Driver`
   - `PlatformToolset=WindowsKernelModeDriver10.0`
   - `DriverType=WDM`
   - Include path contains `$(SolutionDir)include` so `driver_protocol.h` resolves.
4. Build the `x64|Release` configuration to produce `RvrseMonitor.sys`.
5. Install the driver manually or via `sc create RvrseMonitor type= kernel binPath= %SystemRoot%\System32\drivers\RvrseMonitor.sys` (test signing required for development).

## User-Mode Interface

- `src/core/driver_interface.(h|cpp)` exposes `DriverInterface::EnsureDriverAvailable()` / `Ping()` for quick health checks.
- `src/core/driver_service.(h|cpp)` wraps the Service Control Manager so contributors can install/uninstall/start/stop the kernel service without writing repetitive SCM boilerplate.
- Tests call both layers to ensure the APIs behave deterministically even when the driver/service is missing (the expectation today is graceful failure).

## Future Work

- signed test certificate + deployment scripts
- IOCTL surface for privileged snapshots (handle duplication, kernel memory metrics)
- Optional service that auto-loads the driver on startup
