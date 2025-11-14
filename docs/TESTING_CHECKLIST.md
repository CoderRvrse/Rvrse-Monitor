# Testing Checklist - Rvrse Monitor

This document outlines the mandatory testing procedures before pushing code to the repository.

## 🔴 CRITICAL: Test Before Every Push

All team members must complete these checks before pushing to `main` or creating a pull request.

---

## 1. Build Verification (5 minutes)

### Debug Build
```powershell
# From repository root
pwsh -Command "$env:PATH='C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin;'+$env:PATH; msbuild RvrseMonitor.sln -p:Configuration=Debug -p:Platform=x64 -m"
```

**Expected Result:**
- ✅ Build succeeds with 0 errors
- ⚠️ Warnings are acceptable but should be minimized
- ❌ Any build errors = DO NOT PUSH

### Release Build
```powershell
pwsh -Command "$env:PATH='C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin;'+$env:PATH; cmd /c scripts\build_release_local.cmd Release"
```

**Expected Result:**
- ✅ Build succeeds
- ✅ Tests pass (see output)
- ✅ `build\Release\RvrseMonitorApp.exe` created
- ✅ Telemetry output shows reasonable performance metrics

---

## 2. Unit Tests (2 minutes)

```bash
# From repository root
cd build/Release
./RvrseMonitorTests.exe
```

**Expected Results:**
```
[PASS] All NormalizePath tests
[PASS] All ProcessSnapshot tests
[PASS] All HandleSnapshot tests
[PASS] All Module enumeration tests
[PASS] All Network snapshot tests (when implemented)
```

**Critical:**
- ❌ ANY test failure = DO NOT PUSH
- Fix the failing test before continuing
- If you can't fix it, create an issue and notify the team

---

## 3. Performance Validation (2 minutes)

Check the telemetry output from the Release build:

```
Expected Performance Targets:
ProcessSnapshot: ~4-6 ms average (⚠️ >10 ms = performance regression)
HandleSnapshot:  ~15-25 ms average (⚠️ >50 ms = performance regression)
```

**If Performance Regresses:**
1. Review your changes for expensive operations
2. Check if you added synchronous I/O or blocking calls
3. Profile with the built-in telemetry system
4. Consider reverting or optimizing before pushing

---

## 4. Manual UI Testing (5 minutes)

Launch the application and verify core functionality:

```bash
cd build/Release
./RvrseMonitorApp.exe
```

### 4.1 Process Viewer
- [ ] Application launches without crashing
- [ ] Process list populates (shows running processes)
- [ ] Click "Refresh" - list updates
- [ ] Verify PID, Thread Count, Working Set columns show data
- [ ] Sort by different columns - no crashes

### 4.2 Module Viewer (On-Demand)
- [ ] Double-click a process (e.g., `RobloxPlayerBeta.exe`, `chrome.exe`)
- [ ] Modules dialog opens
- [ ] Modules list populates within ~500ms
- [ ] Verify columns: Module Name, Base Address, Size, Path
- [ ] Check that paths look correct (not truncated/corrupted)
- [ ] Close dialog - no crashes

### 4.3 Handle Viewer
- [ ] Click "Handles..." button
- [ ] Handle list populates
- [ ] Verify Handle, Type, Name columns
- [ ] Handles should include: File, Process, Thread, Event, etc.
- [ ] No obvious memory leaks (check Task Manager if suspicious)

### 4.4 Network Viewer (When Implemented)
- [ ] Click "Network..." or equivalent button
- [ ] TCP/UDP connections display
- [ ] Verify Protocol, Local Address, Remote Address, State, PID columns
- [ ] Filter by process works
- [ ] No crashes when refreshing

### 4.5 Performance Graphs
- [ ] CPU graph displays and updates in real-time
- [ ] Memory graph displays and updates in real-time
- [ ] Graphs animate smoothly (no stuttering)
- [ ] Values seem reasonable (CPU 0-100%, Memory in MB/GB)

---

## 5. Driver Testing (When Applicable)

**⚠️ Only test driver changes on LOCAL MACHINE with proper signing**

### 5.1 Driver Build
```bash
# Build driver (requires WDK)
cd src/driver/rvrse_monitor_driver
msbuild rvrse_monitor_driver.vcxproj -p:Configuration=Release -p:Platform=x64
```

### 5.2 Driver Installation (ADMIN REQUIRED)
```powershell
# Enable test signing (one-time, requires reboot)
bcdedit /set testsigning on
# Reboot

# Install driver service
sc create RvrseMonitor type=kernel binPath=C:\path\to\rvrse_monitor_driver.sys
sc start RvrseMonitor
```

### 5.3 Driver Validation
- [ ] Driver service starts without error
- [ ] `DriverInterface::Ping()` succeeds from usermode
- [ ] Event Viewer shows no driver crashes/errors
- [ ] Driver unloads cleanly: `sc stop RvrseMonitor`

**❌ If driver causes BSOD or system instability = DO NOT PUSH**

---

## 6. Git Hygiene (1 minute)

Before committing:

```bash
# Check what you're about to commit
git status
git diff

# Verify no secrets/credentials
# ❌ DO NOT commit:
#    - API keys, passwords, tokens
#    - .env files with secrets
#    - Personal paths (e.g., C:\Users\YourName\...)
#    - Large binary files (>10 MB)
```

### Commit Message Format
```
<Type>: <Brief summary>

<Detailed description of changes>

- Bullet point 1
- Bullet point 2

Performance: <metrics if applicable>
Tests: <test results>

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude <noreply@anthropic.com>
```

**Types:** `feat`, `fix`, `refactor`, `test`, `docs`, `perf`, `style`

---

## 7. Pre-Push Checklist

Before `git push`:

- [ ] ✅ Debug build succeeds
- [ ] ✅ Release build succeeds
- [ ] ✅ All unit tests pass
- [ ] ✅ Performance metrics within acceptable range
- [ ] ✅ Manual UI testing completed (at least basic smoke test)
- [ ] ✅ No crashes, hangs, or obvious bugs
- [ ] ✅ Commit message is clear and descriptive
- [ ] ✅ No secrets or credentials in code
- [ ] ✅ Code follows existing style/conventions

**Only push if ALL boxes are checked.**

---

## 8. Post-Push Validation (Office PC)

**For the office PC owner:**

After team pushes changes:

```bash
# Pull latest changes
git pull origin main

# Run full validation
pwsh -Command "$env:PATH='C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin;'+$env:PATH; cmd /c scripts\build_release_local.cmd Release"

# Launch and test UI
cd build/Release
./RvrseMonitorApp.exe
```

### If Issues Found:
1. Document the bug (screenshot + steps to reproduce)
2. Create GitHub issue immediately
3. Assign to the person who pushed
4. Consider reverting the commit if critical: `git revert <commit-hash>`

---

## 9. Common Issues & Fixes

### Build Fails: "Cannot find msbuild.exe"
```powershell
# Add to PATH
$env:PATH = "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin;" + $env:PATH
```

### Tests Fail: "Cannot open file"
- Ensure you're running from `build/Release` or `build/Debug`
- Check that test data files exist

### UI Crashes on Startup
- Check for missing DLLs (run from VS Developer Command Prompt)
- Verify all dependencies are statically linked or present

### Performance Regression
- Profile with telemetry: Check `telemetry_*.json` files
- Use Visual Studio Profiler for deep analysis
- Review recent commits for O(n²) algorithms or blocking I/O

### Driver Won't Load
- Verify test signing is enabled: `bcdedit /enum {current}`
- Check driver signature: Right-click `.sys` → Properties → Digital Signatures
- Review Event Viewer: Windows Logs → System (filter by "kernel")

---

## 10. Emergency Procedures

### Critical Bug Pushed to Main
```bash
# Option 1: Revert the commit
git revert <bad-commit-hash>
git push origin main

# Option 2: Fix forward (if revert is complex)
# Create hotfix branch, fix, test, merge ASAP
git checkout -b hotfix/critical-bug
# ... make fixes ...
git commit -m "hotfix: Fix critical bug in <component>"
git push origin hotfix/critical-bug
# Create PR and merge immediately after validation
```

### Build Broken for Everyone
1. **Immediate:** Post in team chat "Build broken, investigating"
2. **Within 10 min:** Either fix or revert the breaking commit
3. **Update:** "Build fixed, please pull latest"

---

## 11. Continuous Integration (Future)

When we set up GitHub Actions:

- Automated builds on every push
- Automated unit tests
- Performance regression detection
- Automatic PR checks

Until then: **Manual testing is mandatory.**

---

## Questions?

- Check [README.md](../README.md) for build instructions
- Review [system-monitor-architecture.md](design/system-monitor-architecture.md) for design
- Ask in team chat for help

**Remember: Quality over speed. It's better to take 10 extra minutes testing than to break the build for everyone.**
