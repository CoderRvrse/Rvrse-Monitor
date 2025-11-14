# Team Onboarding - Rvrse Monitor

Welcome to the Rvrse Monitor development team! This guide will help you get started with remote development using Claude Code.

**📘 IMPORTANT:** Before you start coding, read the [Claude Code Development Guide](CLAUDE_CODE_GUIDE.md) to learn how to work effectively with Claude Code, especially the **todo list system**.

---

## 🚀 Quick Start (15 minutes)

### 1. Clone the Repository
```bash
git clone https://github.com/CoderRvrse/Rvrse-Monitor.git
cd Rvrse-Monitor
```

### 2. Install Prerequisites

**Required:**
- Windows 10/11 (64-bit)
- Visual Studio 2022 Community (or higher)
  - Include "Desktop development with C++" workload
  - Include "Windows 11 SDK"
- Git for Windows

**Optional:**
- Windows Driver Kit (WDK) - Only if working on kernel driver
- Code signing certificate - For driver signing

### 3. Build the Project

```powershell
# Quick build using the automated script
pwsh -Command "$env:PATH='C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin;'+$env:PATH; cmd /c scripts\build_release_local.cmd Release"
```

Expected output:
```
Building Release configuration...
Running tests...
[PASS] All tests passed
Build artifacts: build\Release\RvrseMonitorApp.exe
```

### 4. Run the Application
```bash
cd build\Release
./RvrseMonitorApp.exe
```

You should see the Rvrse Monitor UI with process list, CPU/memory graphs, and interactive controls.

---

## 📁 Project Structure

```
Rvrse Monitor/
├── src/
│   ├── app/                  # GUI application (Win32 API)
│   │   └── main.cpp          # Main UI logic, event loop
│   ├── core/                 # Core monitoring engine
│   │   ├── process_snapshot.cpp/h    # Process enumeration
│   │   ├── handle_snapshot.cpp/h     # Handle enumeration
│   │   ├── network_snapshot.cpp/h    # Network connections (WIP)
│   │   ├── driver_interface.cpp/h    # Kernel driver communication
│   │   └── driver_service.cpp/h      # Driver service management
│   ├── common/               # Shared utilities
│   │   └── string_utils.cpp/h        # Path normalization, string helpers
│   └── driver/               # Kernel-mode driver (WDM)
│       └── rvrse_monitor_driver/     # Driver scaffold
├── include/
│   └── rvrse/                # Public headers
│       └── driver_protocol.h         # Shared IOCTL definitions
├── tests/
│   └── main.cpp              # Unit tests
├── scripts/
│   ├── build_release_local.cmd       # Automated build + test
│   └── validate_before_push.ps1      # Pre-push validation
├── docs/
│   ├── TESTING_CHECKLIST.md          # **READ THIS BEFORE PUSHING**
│   ├── TEAM_ONBOARDING.md            # This file
│   └── design/
│       └── system-monitor-architecture.md  # System design
└── build/                    # Build output (generated)
    ├── Debug/
    └── Release/
```

---

## 🛠️ Development Workflow

### Daily Workflow
1. **Pull latest changes**
   ```bash
   git pull origin main
   ```

2. **Create a feature branch** (optional but recommended)
   ```bash
   git checkout -b feature/network-viewer
   ```

3. **Make your changes**
   - Edit code using Claude Code or your preferred editor
   - Build frequently to catch errors early
   ```bash
   msbuild RvrseMonitor.sln -p:Configuration=Debug -p:Platform=x64 -m
   ```

4. **Test your changes** (CRITICAL)
   - Run the pre-push validation script:
   ```powershell
   .\scripts\validate_before_push.ps1
   ```
   - See [TESTING_CHECKLIST.md](TESTING_CHECKLIST.md) for detailed testing procedures

5. **Commit with a good message**
   ```bash
   git add .
   git commit -m "feat: Add TCP connection enumeration to NetworkSnapshot

   - Implement GetExtendedTcpTable API wrapper
   - Add IPv4/IPv6 address formatting
   - Include unit tests for connection filtering

   Tests: All unit tests pass
   Performance: No regression detected
   "
   ```

6. **Push to GitHub**
   ```bash
   git push origin feature/network-viewer
   ```

7. **Create a Pull Request**
   - Go to GitHub repository
   - Click "Compare & pull request"
   - Fill in description, link issues
   - Request review from team lead

---

## 🎯 Current Priorities (Tier 3)

### ✅ Completed Features
- [x] Process enumeration with thread details
- [x] Handle enumeration (files, mutexes, events, etc.)
- [x] Module enumeration (on-demand, lazy loading)
- [x] Real-time CPU/Memory graphs
- [x] Performance telemetry system
- [x] Plugin architecture
- [x] Automated release packaging
- [x] UNC path normalization

### 🚧 In Progress
- [ ] **Network Connections View** (HIGH PRIORITY)
  - TCP/UDP connection enumeration
  - Per-process filtering
  - UI integration
  - Unit tests

### 📋 Planned (Tier 3+)
- [ ] Kernel driver enhancements
  - Process creation/termination notifications
  - Registry monitoring
  - File system monitoring
- [ ] Advanced filtering and search
- [ ] Export to CSV/JSON
- [ ] Dark mode UI theme

---

## 🧪 Testing Guidelines

**CRITICAL:** Read [TESTING_CHECKLIST.md](TESTING_CHECKLIST.md) before your first push.

### Quick Testing (Every Commit)
```powershell
# Automated validation
.\scripts\validate_before_push.ps1 -SkipUI
```

### Full Testing (Before Push)
```powershell
# Full validation including UI test prompts
.\scripts\validate_before_push.ps1
```

### Manual UI Testing
1. Launch `build\Release\RvrseMonitorApp.exe`
2. Click "Refresh" - process list updates
3. Double-click a process - modules dialog opens
4. Click "Handles..." - handles list populates
5. Check CPU/Memory graphs animate smoothly

**If ANY test fails, DO NOT PUSH. Fix the issue first.**

---

## 🐛 Debugging Tips

### Build Errors
- **"Cannot find msbuild.exe"**
  ```powershell
  $env:PATH = "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin;" + $env:PATH
  ```

- **"Unresolved external symbol"**
  - Check that all `.cpp` files are included in `RvrseCore.vcxproj`
  - Verify linker settings in project properties

### Runtime Errors
- **"Access Denied" when opening process**
  - Run as Administrator for full system access
  - Some protected processes (System, Registry) require kernel driver

- **Crash on startup**
  - Run from Visual Studio debugger (F5)
  - Check Event Viewer for application crashes
  - Verify all dependencies are present

### Performance Issues
- **ProcessSnapshot taking >10ms**
  - You likely added synchronous I/O or expensive operations
  - Profile with telemetry: Check `telemetry_*.json`
  - Use Visual Studio Profiler for deep analysis

### UI Glitches
- **Window not responding**
  - Check for blocking calls on UI thread
  - Move expensive operations to background threads
  - Use `InvalidateRect()` for asynchronous UI updates

---

## 📦 Dependencies & APIs

### Windows APIs Used
- **Process Management:** `NtQuerySystemInformation`, `OpenProcess`, `EnumProcessModules`
- **Handle Enumeration:** `NtQuerySystemInformation(SystemHandleInformation)`
- **Network:** `GetExtendedTcpTable`, `GetExtendedUdpTable` (iphlpapi.h)
- **Driver Communication:** `CreateFile`, `DeviceIoControl`
- **UI:** Win32 API (CreateWindow, GDI for graphs)

### Third-Party Libraries
- None currently (pure Win32/WDM)
- All dependencies are Windows SDK APIs

---

## 🔐 Security & Best Practices

### Code Review Checklist
- [ ] No hardcoded passwords/API keys
- [ ] No unbounded loops or recursion
- [ ] Proper error handling (check return values)
- [ ] No memory leaks (use RAII, smart pointers where possible)
- [ ] Input validation for user-provided data
- [ ] Buffer overflow protection (`wcsncpy_s` instead of `wcscpy`)

### Sensitive Operations
- **Administrator privileges:** Required for full process access
- **Kernel driver:** Requires code signing, test signing mode, or local debugging
- **Performance:** Avoid O(n²) algorithms in hot paths (snapshot code)

### Git Hygiene
- **DO NOT commit:**
  - Personal paths (e.g., `C:\Users\YourName\...`)
  - Large binaries (>10 MB)
  - `.env` files with secrets
  - Temporary files (`*.tmp`, `*.log`)
  - Build artifacts (`build/`, `dist/`)

- **DO commit:**
  - Source code (`.cpp`, `.h`)
  - Project files (`.vcxproj`, `.sln`)
  - Documentation (`.md`)
  - Scripts (`.ps1`, `.cmd`)

---

## 🆘 Getting Help

### Common Questions
1. **"How do I add a new feature?"**
   - Read `docs/design/system-monitor-architecture.md`
   - Check existing similar features (e.g., `process_snapshot.cpp`)
   - Ask in team chat before starting

2. **"My change broke a test, now what?"**
   - Read the test failure message carefully
   - Run the specific test in isolation
   - If you can't fix it, revert your change and ask for help

3. **"Can I work on the driver?"**
   - Driver work requires local testing (admin + signing)
   - Focus on usermode features remotely via Claude Code
   - Coordinate with office PC for driver validation

### Resources
- **Claude Code Guide:** [CLAUDE_CODE_GUIDE.md](CLAUDE_CODE_GUIDE.md) ⭐ **Read this first!**
- **Architecture:** [system-monitor-architecture.md](design/system-monitor-architecture.md)
- **Testing:** [TESTING_CHECKLIST.md](TESTING_CHECKLIST.md)
- **Driver:** [docs/driver/scaffold.md](driver/scaffold.md)
- **GitHub Repo:** https://github.com/CoderRvrse/Rvrse-Monitor

### Team Communication
- Create GitHub issues for bugs/features
- Use pull requests for code review
- Tag team lead for urgent issues

---

## 🚀 Your First Task

**Goal:** Successfully build and run the project, then make a trivial change.

1. **Build the project** (follow Quick Start above)
2. **Run the application** and verify it works
3. **Make a trivial change:**
   - Edit `README.md` and add your name to a "Contributors" section
   - Or add a comment to `src/app/main.cpp`
4. **Test your change:**
   ```powershell
   .\scripts\validate_before_push.ps1
   ```
5. **Commit and push:**
   ```bash
   git checkout -b your-name/first-commit
   git add .
   git commit -m "docs: Add my name to contributors"
   git push origin your-name/first-commit
   ```
6. **Create a pull request** on GitHub

Once that PR is merged, you're ready for real tasks! 🎉

---

## 📞 Contact

- **GitHub Issues:** https://github.com/CoderRvrse/Rvrse-Monitor/issues
- **Pull Requests:** https://github.com/CoderRvrse/Rvrse-Monitor/pulls
- **Team Lead:** (Your office PC contact)

**Welcome to the team! Happy coding! 💻**
