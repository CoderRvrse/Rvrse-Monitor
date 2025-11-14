# Testing Guide for Office PC

This document is specifically for **you** (office PC owner) to validate changes pushed by the remote development team.

---

## 🎯 Your Role

As the office PC owner, you are the **final quality gate** before features are considered complete. Your responsibilities:

1. **Pull and test** every push from the remote team
2. **Validate UI functionality** that remote devs can't fully test
3. **Test driver features** that require admin/signing
4. **Report bugs** immediately if found
5. **Approve or reject** pull requests

---

## ⚡ Quick Validation (After Every Push)

### Step 1: Pull Latest Changes
```bash
git pull origin main
```

### Step 2: Run Automated Build + Test
```powershell
pwsh -Command "$env:PATH='C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin;'+$env:PATH; cmd /c scripts\build_release_local.cmd Release"
```

**Expected Output:**
```
Building Release configuration...
Running tests...
[PASS] All tests succeeded.
ProcessSnapshot: avg 4.32 ms
HandleSnapshot: avg 19.50 ms
```

✅ **If this passes, 90% of validation is done.**

### Step 3: Quick UI Smoke Test (2 minutes)
```bash
cd build\Release
.\RvrseMonitorApp.exe
```

**Quick Checklist:**
- [ ] App launches without crashing
- [ ] Process list shows running processes
- [ ] Click "Refresh" - list updates
- [ ] Double-click a process - Modules dialog opens
- [ ] Modules list populates quickly (<500ms)
- [ ] Click "Handles..." - Handles list appears
- [ ] CPU/Memory graphs animate smoothly

✅ **If all checks pass → APPROVED**
❌ **If ANY check fails → Create GitHub issue immediately**

---

## 🐛 What to Do If You Find a Bug

### 1. Document the Bug
Take a screenshot and note:
- What you were doing (steps to reproduce)
- What you expected to happen
- What actually happened
- Any error messages

### 2. Create a GitHub Issue
Go to: https://github.com/CoderRvrse/Rvrse-Monitor/issues/new

**Template:**
```markdown
## Bug Report

**Description:**
Modules dialog crashes when double-clicking System process (PID 4)

**Steps to Reproduce:**
1. Launch RvrseMonitorApp.exe
2. Find "System" process (PID 4) in list
3. Double-click it
4. App crashes immediately

**Expected Behavior:**
Should show access denied error or empty modules list

**Actual Behavior:**
Application crashes with no error message

**Environment:**
- Windows 11 Pro (22H2)
- Commit: a897c65
- Build: Release x64

**Screenshot:**
[Attach screenshot here]
```

### 3. Notify the Team
- Assign the issue to the person who pushed the change
- If critical (app crashes, data loss), mark as "Priority: Critical"
- If it's a minor UI glitch, mark as "Priority: Low"

### 4. Consider Reverting (If Critical)
If the bug is **critical** (crashes, data corruption, security issue):

```bash
# Revert the bad commit
git revert <commit-hash>
git push origin main

# Then create the GitHub issue
```

**Example:**
```bash
git log --oneline -5  # Find the bad commit
git revert a897c65    # Revert it
git push origin main  # Push the revert
```

---

## 🧪 Deep Testing (New Features)

When the team implements a **new feature**, do thorough testing:

### For Network Connections Feature (Coming Soon)
1. Launch app
2. Click "Network..." or similar button
3. Verify TCP/UDP connections display
4. Check columns: Protocol, Local Address, Remote Address, State, PID, Process Name
5. Test filtering by process
6. Click "Refresh" - connections update
7. Open/close network-heavy apps (browser, game) - connections appear/disappear
8. Look for memory leaks (run for 5+ minutes, check Task Manager)

### For Driver Features (When Implemented)
1. **Install driver** (requires admin):
   ```powershell
   # Enable test signing (one-time)
   bcdedit /set testsigning on
   # Reboot

   # Install driver
   sc create RvrseMonitor type=kernel binPath=C:\path\to\rvrse_monitor_driver.sys
   sc start RvrseMonitor
   ```

2. **Test driver communication:**
   - Launch app
   - Check Event Viewer for driver errors: `eventvwr.msc` → Windows Logs → System
   - If driver features fail, check that `DriverInterface::Ping()` succeeds

3. **Stability test:**
   - Run app for 10+ minutes with driver loaded
   - Monitor for BSODs (Blue Screen of Death)
   - Check CPU usage (driver shouldn't consume >1% when idle)

4. **Unload cleanly:**
   ```powershell
   sc stop RvrseMonitor
   sc delete RvrseMonitor
   ```

**⚠️ If driver causes BSOD, create a CRITICAL issue immediately and DO NOT approve the PR.**

---

## 📊 Performance Regression Check

After each push, check telemetry metrics:

```bash
# Find latest telemetry file
ls -la telemetry_*.json

# View metrics (PowerShell)
cat telemetry_*.json | ConvertFrom-Json | % { $_.metrics }
```

**Acceptable Ranges:**
- `ProcessSnapshot`: < 10 ms ✅ / 10-20 ms ⚠️ / >20 ms ❌
- `HandleSnapshot`: < 30 ms ✅ / 30-50 ms ⚠️ / >50 ms ❌

**If performance regresses:**
1. Create a GitHub issue: "Performance regression in <feature>"
2. Include telemetry data (attach JSON file)
3. Assign to the person who pushed the change
4. Ask them to profile and optimize

---

## 🚀 Approving Changes

If **all validation passes**:
1. Comment on the GitHub PR: "✅ Tested locally, all checks pass"
2. If you have merge permissions, merge the PR
3. Otherwise, approve the PR and notify team lead

---

## 🔥 Emergency Procedures

### Build Is Broken for You
1. **Pull again** (maybe someone already fixed it):
   ```bash
   git pull origin main
   ```

2. **Check GitHub Actions** (if enabled):
   - Go to: https://github.com/CoderRvrse/Rvrse-Monitor/actions
   - See if CI build is also failing

3. **Notify the team immediately:**
   - Create issue: "Build broken on main after commit <hash>"
   - Ping the person who pushed

4. **If urgent, revert:**
   ```bash
   git revert <bad-commit-hash>
   git push origin main
   ```

### App Crashes Repeatedly
1. **Get crash details:**
   - Event Viewer: `eventvwr.msc` → Application logs
   - Look for "RvrseMonitorApp.exe" crashes

2. **Run in debugger:**
   - Open `RvrseMonitor.sln` in Visual Studio
   - Press F5 to debug
   - Note the exception and call stack

3. **Report with full details:**
   - Exception type (e.g., "Access Violation at 0x00007FF...")
   - Call stack
   - What you were doing when it crashed

### Driver Causes BSOD
1. **Boot into Safe Mode** (driver won't load)
2. **Delete the driver service:**
   ```powershell
   sc delete RvrseMonitor
   ```
3. **Disable test signing if needed:**
   ```powershell
   bcdedit /set testsigning off
   ```
4. **Create CRITICAL GitHub issue**:
   - Title: "CRITICAL: Driver causes BSOD"
   - Include minidump file from `C:\Windows\Minidump\` if available
   - Block all driver PRs until fixed

---

## 📋 Weekly Tasks

### Every Monday
- [ ] Pull latest changes: `git pull origin main`
- [ ] Full build + test: `scripts\build_release_local.cmd Release`
- [ ] Run app for 10 minutes (look for memory leaks, crashes)
- [ ] Check GitHub for new issues/PRs

### Before Team Meetings
- [ ] Review all open issues
- [ ] Test any new PRs
- [ ] Prepare feedback on what's working/broken

---

## 💡 Tips

### Speed Up Testing
- Keep `build\Release\RvrseMonitorApp.exe` pinned to taskbar
- Use `git pull && <validation-script>` as a single command
- Set up GitHub notifications for pushes

### Catch Bugs Early
- Test **immediately** after a push (don't wait)
- If something feels slow or glitchy, report it (even if it doesn't crash)
- Memory leaks are subtle - watch Task Manager during long runs

### Communicate Effectively
- 👍 "Tested commit a897c65, all checks pass" (clear, concise)
- 👎 "Something is broken" (too vague, not actionable)

---

## 📞 Need Help?

- **GitHub Issues:** https://github.com/CoderRvrse/Rvrse-Monitor/issues
- **Documentation:** Check `docs/` folder
- **Testing Checklist:** `docs/TESTING_CHECKLIST.md`

**Remember: You are the quality gatekeeper. If in doubt, ask the team to fix before approving!**
