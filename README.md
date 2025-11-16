# Rvrse Monitor

[![Build and Test](https://github.com/CoderRvrse/Rvrse-Monitor/actions/workflows/build-and-test.yml/badge.svg)](https://github.com/CoderRvrse/Rvrse-Monitor/actions)

A high-performance system monitor for Windows, built in native C++ with a focus on process management, real-time data visualization, and extensibility. Rvrse Monitor provides deep visibility into running processes, network connections, and system resources.

## Features

### Core Capabilities
- **Live Process Monitor** – Real-time process list with CPU, memory, and thread information
- **Process Control Suite** – Right-click context menu for:
  - **Terminate** single processes or entire process trees
  - **Suspend/Resume** processes (thread-level suspension)
  - **Set Priority** across 6 levels (Realtime, High, Above Normal, Normal, Below Normal, Low)
- **Real-time Search/Filter** – Instant filtering by process name or PID with visual feedback
- **Process Tree View** – Hierarchical view of process parent-child relationships with expand/collapse
- **Module Viewer** – Inspect all DLLs loaded by a process (base address, size, path)
- **Network Connections** – View per-process network activity with IPv4/IPv6 support
- **Performance Graphs** – Real-time CPU and memory utilization charts
- **JSON Telemetry Export** – Export performance metrics for analysis

### Technical Features
- Plugin system with sample logger plugin
- Optional kernel driver scaffold for privileged telemetry
- Comprehensive unit tests with CI/CD integration
- Environment-aware testing (supports elevated and non-elevated environments)

## Quick Start

### Prerequisites
- Windows 10/11 (x64)
- Visual Studio 2022 with C++ workload and Windows SDK

### Build & Run
```bash
# Using the build script
scripts\build_release_local.cmd Release

# Or open in Visual Studio
# Open RvrseMonitor.sln and build for x64|Release
```

The resulting executable is at `build\Release\RvrseMonitorApp.exe`.

### Running Tests
```bash
# After build, run unit tests
build\Release\RvrseMonitorTests.exe
```

## Project Structure

```
.
├── src/
│   ├── app/              # Win32 UI and main application
│   ├── core/             # Process, network, and handle snapshots
│   ├── common/           # Shared utilities (formatting, logging)
│   ├── plugins/          # Plugin SDK and sample implementations
│   └── driver/           # Optional kernel-mode driver
├── include/              # Public headers
├── tests/                # Unit tests
├── docs/                 # Documentation
├── scripts/              # Build and automation scripts
├── CHANGELOG.md          # Release history
└── RvrseMonitor.sln      # Visual Studio solution
```

## Architecture

### Key Components

**ProcessSnapshot** (`src/core/process_snapshot.h`)
```cpp
// Capture current process state
auto snapshot = rvrse::core::ProcessSnapshot::Capture();

// Access processes with parent-child relationships
for (const auto& process : snapshot.Processes()) {
    std::cout << process.imageName << " (PID " << process.processId << ")\n";

    // Get child processes
    auto children = snapshot.GetChildProcesses(process.processId);
    for (auto childPid : children) {
        // Handle process tree
    }
}

// Terminate single process
HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, processId);
TerminateProcess(hProcess, 1);
CloseHandle(hProcess);
```

**NetworkSnapshot** (`src/core/network_snapshot.h`)
```cpp
// Capture network connections
auto netSnapshot = rvrse::core::NetworkSnapshot::Capture();

// Access per-process connections
for (const auto& conn : netSnapshot.Connections()) {
    if (conn.owningProcessId == targetPid) {
        std::cout << "Connection: " << conn.remoteAddress
                  << ":" << conn.remotePort << "\n";
    }
}
```

**HandleSnapshot** (`src/core/handle_snapshot.h`)
```cpp
// Capture all open handles
auto handleSnapshot = rvrse::core::HandleSnapshot::Capture();

// Count handles per process
auto handleCount = handleSnapshot.HandleCountForProcess(processId);
std::cout << "Process has " << handleCount << " open handles\n";
```

## Usage Examples

### Filtering Processes
1. Type in the search box to filter by process name (case-insensitive)
2. Type a number to filter by exact PID match
3. Click **Clear** to reset the filter

### Terminating a Process
1. Right-click the process in the list
2. Select **Terminate Process** or **Terminate Process Tree**
3. Confirm in the dialog

### Suspending a Process (Temporary Pause)
1. Right-click the process
2. Select **Suspend Process** to freeze all threads
3. Select **Resume Process** to unfreeze

### Changing Process Priority
1. Right-click the process
2. Select **Set Priority** → choose level
3. Confirm the change (requires admin for Realtime)

## Building & Testing

### Local Build
```bash
# Build Release configuration
scripts\build_release_local.cmd Release

# Build Debug configuration
scripts\build_release_local.cmd Debug

# Build everything
scripts\build_release_local.cmd All
```

### Running Tests
Tests run automatically as part of the build script, or manually:
```bash
build\Release\RvrseMonitorTests.exe
```

### Test Coverage
The test suite validates:
- Process enumeration and tree relationships
- Network connection capture (IPv4/IPv6)
- Handle enumeration
- Search/filter logic
- Priority and suspension operations
- Performance benchmarks

## Continuous Integration

GitHub Actions automatically:
1. Builds Debug and Release configurations on every push
2. Runs unit tests
3. Uploads build artifacts
4. Tracks test results

Check `.github/workflows/build-and-test.yml` for the full CI definition.

## Documentation

- **[CHANGELOG.md](CHANGELOG.md)** – Complete feature history and releases
- **[docs/FEATURE_ROADMAP.md](docs/FEATURE_ROADMAP.md)** – Planned features and roadmap
- **[docs/build/local-project.md](docs/build/local-project.md)** – Detailed build instructions
- **[docs/contributing.md](docs/contributing.md)** – Contribution guidelines

## Plugin System

Create custom plugins to extend Rvrse Monitor:

```cpp
// Implement in your DLL
#include "rvrse/plugin_api.h"

extern "C" __declspec(dllexport)
HRESULT RvrsePluginInitialize(const PluginHost* host) {
    // Initialize your plugin
    return S_OK;
}

extern "C" __declspec(dllexport)
void RvrsePluginShutdown() {
    // Clean up
}
```

Plugins are automatically loaded from `build\<Config>\plugins\` at startup.

## Performance

Rvrse Monitor is optimized for:
- **Minimal CPU overhead** – Snapshot captures complete in < 100ms
- **Fast rendering** – GDI-based graphs update at 4s cadence
- **Memory efficient** – Lightweight data structures for process/handle enumeration

## Known Limitations

- Requires Visual Studio 2022 for building (not compatible with earlier versions)
- Kernel driver features optional (standard user-mode operation supported)
- Performance benchmarks may vary in virtualized or low-resource environments

## License

This project is provided as-is for educational and authorized security testing purposes.

## Support

- **Issues:** Report bugs via GitHub Issues
- **Pull Requests:** Submit feature branches via GitHub Pull Requests
- **Documentation:** See `docs/` directory for detailed guides
