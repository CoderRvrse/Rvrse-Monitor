# Driver

Kernel-mode components live here so they can be built/tested with the Windows Driver Kit without polluting the user-mode projects.

## Layout

- `rvrse_monitor_driver/driver.c` – minimal WDM driver that exposes `\\Device\\RvrseMonitor` and implements two IOCTLs: `IOCTL_RVRSE_PING` and `IOCTL_RVRSE_QUERY_VERSION`.
- `../include/rvrse/driver_protocol.h` – shared contract between user space and the driver (device paths, IOCTL codes, version payload).

## Building

1. Install the latest Windows Driver Kit (WDK) alongside Visual Studio 2022.
2. Create a new *Empty WDM Driver* project inside `src/driver/rvrse_monitor_driver` or add `driver.c` to an existing KM project.
3. Set the target platform to `x64`, enable /GS, and sign the output with your test certificate.
4. Produce `RvrseMonitor.sys`, copy it to `%SystemRoot%\System32\drivers`, and register a service that creates the symbolic link `\\.\RvrseMonitor`.

The driver currently supports only two operations (ping + version query), which is enough for the user-mode scaffolding to verify connectivity. Future work will extend the IOCTL surface for privileged telemetry.
