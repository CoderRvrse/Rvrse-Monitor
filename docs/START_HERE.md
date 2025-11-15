# 🚀 START HERE - Remote Development Team

Welcome to the Rvrse Monitor project! This document is your **first step** to becoming productive immediately.

---

## 📍 You Are Here

You're joining a project to build a **high-performance Windows system monitor** that will compete with Process Hacker 2. We're currently at **40% feature parity** with a clear roadmap to **90% in 3 months**.

**Your advantage:** Everything is already set up. You just need to follow this guide.

---

## ⚡ Quick Start (30 Minutes to First Contribution)

### Step 1: Clone the Repository (2 minutes)

```bash
git clone https://github.com/CoderRvrse/Rvrse-Monitor.git
cd Rvrse-Monitor
```

### Step 2: Open in Claude Code (1 minute)

```bash
claude-code .
```

**What happens:** The SessionStart hook automatically configures your environment. You'll see:
```
[Claude Setup] Installing Node packages...
[Claude Setup] Installing Python dependencies...
[Claude Setup] Ready.
```

✅ **Environment is now configured!**

### Step 3: Read Documentation (15 minutes)

**Read these in order:**

1. **[README.md](../README.md)** (2 min)
   - Quick project overview
   - See the "For Remote Developers" section

2. **[CLAUDE_CODE_GUIDE.md](CLAUDE_CODE_GUIDE.md)** ⭐ **CRITICAL** (10 min)
   - Learn the **todo list system** (prevents forgetting tasks!)
   - Understand how to work with Claude Code
   - See example workflows

3. **[TEAM_ONBOARDING.md](TEAM_ONBOARDING.md)** (5 min)
   - Current feature status
   - Your first assignment (Network Connections)
   - Development workflow

### Step 4: Build the Project (10 minutes)

```powershell
# Add MSBuild to PATH (one-time)
$env:PATH = "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin;" + $env:PATH

# Build and test
pwsh -Command "cmd /c scripts\build_release_local.cmd Release"
```

**Expected output:**
```
Building Release configuration...
Running tests...
[PASS] All tests succeeded.
ProcessSnapshot avg: 4.32 ms
HandleSnapshot avg: 19.50 ms
```

✅ **You're ready to code!**

---

## 🎯 STATUS: Tier 3 COMPLETE ✅

**Network Connections View (IPv4 + IPv6) is LIVE!**

Commit: `3b6b592` on main branch
Performance: 0.25ms (target <10ms) 🔥
Current Parity: 60% Process Hacker 2

---

## 🚀 YOUR FIRST TASK: Tier 4 Quick Wins

This is your **starting point**. Four high-impact, lower-effort features to reach 75% parity.

### Feature 1: Kill/Terminate Process ⭐⭐⭐ START HERE (2-3 days)

**Most requested feature by users!**

**Say this to Claude Code:**

```
"Implement the Kill/Terminate Process feature.

Requirements:
- Add right-click context menu to process list
- 'Terminate' option using TerminateProcess() API
- 'Terminate Process Tree' option (kills all child processes)
- Confirmation dialog before terminating
- Error handling for protected processes (System, csrss, etc.)
- Success/failure feedback messages
- Add unit tests for process tree enumeration

Use the todo list to track all these tasks."
```

---

### Feature 2: Search/Filter Box (1-2 days)

**Prompt for Claude Code:**
```
"Implement the Search/Filter Box feature.

Requirements:
- Add text box at top of main window
- Filter process list by name (case-insensitive substring match)
- Real-time filtering (as user types)
- Clear button (X icon)
- ESC key clears filter
- Show 'Filtered: X of Y processes' in status bar
- Highlight matching processes
- Add unit tests for filtering logic

Use the todo list to track all these tasks."
```

**Impact:** Essential usability improvement

---

### Feature 3: Process Tree View (3-5 days)

**Prompt for Claude Code:**
```
"Implement the Process Tree View feature.

Requirements:
- Track parent PID during ProcessSnapshot
- Build hierarchical tree structure (std::map<PID, vector<PID>>)
- Display as expandable/collapsible tree (TreeView control or indented ListView)
- Show parent-child relationships with visual hierarchy
- Detect orphaned processes (parent terminated)
- Add icons for parent vs leaf processes
- Context menu on tree nodes
- Add unit tests for tree building logic

Use the todo list to track all these tasks."
```

**Impact:** Core diagnostic feature, differentiates from Task Manager

---

### Feature 4: System Information Panel (1-2 days)

**Prompt for Claude Code:**
```
"Implement the System Information Panel.

Requirements:
- Overall CPU usage (%)
- Physical memory used/total (GB)
- Commit charge (paged pool)
- System uptime
- Kernel/User mode CPU split
- Handle count (system-wide)
- Thread count (system-wide)
- Panel at top or side of main window
- Update every 1-2 seconds
- Use GetSystemTimes(), GlobalMemoryStatusEx(), GetTickCount64()

Use the todo list to track all these tasks."
```

**Impact:** Provides system context for intelligent monitoring

---

## 📊 Tier 4 Summary

| Feature | Effort | Impact | Priority | Est. Days |
|---------|--------|--------|----------|-----------|
| Kill/Terminate | Low | CRITICAL | ⭐⭐⭐ | 2-3 |
| Search/Filter | Low | High | ⭐⭐⭐ | 1-2 |
| Process Tree | Medium | High | ⭐⭐⭐ | 3-5 |
| System Info | Low | Medium | ⭐⭐ | 1-2 |
| **Total** | **Low-Medium** | **75% parity** | - | **7-12 days** |

---

## 📚 Essential Documentation

### Must Read
- **[CLAUDE_CODE_GUIDE.md](CLAUDE_CODE_GUIDE.md)** ⭐ Todo list system
- **[TEAM_ONBOARDING.md](TEAM_ONBOARDING.md)** - Development workflow
- **[TESTING_CHECKLIST.md](TESTING_CHECKLIST.md)** - Quality requirements

### Reference
- **[FEATURE_ROADMAP.md](FEATURE_ROADMAP.md)** - 3-month plan (15 features)
- **[FOR_OFFICE_PC.md](FOR_OFFICE_PC.md)** - Office validation workflow
- **[design/system-monitor-architecture.md](design/system-monitor-architecture.md)** - Architecture

---

## 🎓 Best Practices

### Always Use the Todo List

For **any** multi-step task (3+ steps), tell Claude:
```
"Use the todo list to track this"
```

This prevents Claude from forgetting steps and ensures complete work.

### Validate Before Every Push

```powershell
.\scripts\validate_before_push.ps1
```

**DO NOT push if validation fails!** Fix issues first.

### Write Good Commit Messages

```
feat: Implement Network Connections View with TCP/UDP enumeration

Implements system-wide network connection monitoring for Tier 3.

Changes:
- src/core/network_snapshot.h/cpp: TCP/UDP enumeration using IP Helper API
- src/app/main.cpp: Added Network button and viewer dialog
- tests/main.cpp: Unit tests for connection filtering

Performance: NetworkSnapshot avg 6.8 ms (target <10ms) ✅
Tests: All unit tests pass

🤖 Generated with Claude Code
Co-Authored-By: Claude <noreply@anthropic.com>
```

### Communicate Progress

- **Daily:** Push working code to GitHub
- **Blocked?** Create GitHub issue with details
- **Questions?** Ask in team chat or create issue

---

## 🚨 Common Issues & Solutions

### "MSBuild not found"

```powershell
$env:PATH = "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin;" + $env:PATH
```

### "Validation fails"

**DO NOT push!** Read the error output:
- Build errors? Fix the code
- Test failures? Fix the failing test
- Performance regression? Profile and optimize

### "I forgot to use the todo list"

No problem! Just ask Claude:
```
"Create a todo list for the remaining tasks in this feature"
```

### "Claude stopped mid-task"

Check the todo list:
```
"Show me the current todo list and continue with the next pending task"
```

---

## 📊 Success Metrics

### Your Goals

**Month 1:**
- ✅ Network Connections (Week 1)
- ✅ Kill/Terminate Process (Week 2)
- ✅ Search/Filter Box (Week 2)
- ✅ Process Tree View (Week 3)
- ✅ System Info Panel (Week 4)

**Result:** 75% Process Hacker parity

### Quality Targets

- ✅ All builds pass (Debug + Release)
- ✅ All unit tests pass (100% success rate)
- ✅ Performance: <10ms regression from baseline
- ✅ No crashes in manual testing
- ✅ Code follows existing style

---

## 🎯 The Big Picture

### Current Status
- **Version:** 0.3.0
- **Feature Parity:** 40% vs Process Hacker 2
- **Performance:** ✅ **2-3x FASTER** than Process Hacker!

### Your Mission
Build the remaining 50% of features over 3 months to create a competitive Process Hacker alternative.

### Why It Matters
- **Clean codebase:** No GPL licensing issues
- **Modern architecture:** Plugins, telemetry, CI/CD
- **Superior performance:** Already faster than PH
- **Active development:** vs stagnant Process Hacker

---

## 🤝 Team Communication

### GitHub Workflow

**Create issues for:**
- Bugs you find
- Features you're working on
- Questions about implementation

**Create pull requests for:**
- Major features (optional but recommended)
- Experimental changes
- When you want code review

**Direct push to main for:**
- Features that pass validation
- Bug fixes
- Small improvements

### Git Commands

```bash
# Daily workflow
git pull origin main                    # Get latest changes
# ... make changes ...
.\scripts\validate_before_push.ps1      # Validate
git add .                               # Stage files
git commit -m "feat: Add feature X"     # Commit
git push origin main                    # Push

# Feature branch (optional)
git checkout -b feature/my-feature
# ... work ...
git push origin feature/my-feature
# Create PR on GitHub
```

---

## ✅ Checklist: Are You Ready?

Before starting work, confirm:

- [ ] Repository cloned
- [ ] Claude Code opened (SessionStart hook ran)
- [ ] Read [CLAUDE_CODE_GUIDE.md](CLAUDE_CODE_GUIDE.md) (todo list system)
- [ ] Read [TEAM_ONBOARDING.md](TEAM_ONBOARDING.md) (workflow)
- [ ] Built project successfully (Release build)
- [ ] Tests pass (all unit tests green)
- [ ] Understand validation script (`validate_before_push.ps1`)
- [ ] Know how to use todo list ("Use the todo list to track this")

**If all checkboxes are ticked, you're ready to start!**

---

## 🚀 Ready to Begin?

### Your First Command to Claude Code:

```
"I need to implement the Network Connections View feature.

Requirements:
- Design NetworkConnectionEntry and NetworkSnapshot classes
- Implement TCP enumeration using GetExtendedTcpTable (IPv4 + IPv6)
- Implement UDP enumeration using GetExtendedUdpTable (IPv4 + IPv6)
- Add 'Network...' button to main UI
- Display connections in sortable list (Protocol, Local/Remote Address, State, PID)
- Add per-process filtering
- Write unit tests for enumeration and filtering
- Update telemetry to track NetworkSnapshot performance (<10ms target)

Use the todo list to track all these tasks."
```

**Claude will create a 7-task todo list and start implementing immediately!**

---

## 📞 Need Help?

- **Documentation:** Check `docs/` folder
- **GitHub Issues:** https://github.com/CoderRvrse/Rvrse-Monitor/issues
- **Feature Roadmap:** [FEATURE_ROADMAP.md](FEATURE_ROADMAP.md)
- **Architecture:** [design/system-monitor-architecture.md](design/system-monitor-architecture.md)

---

**Welcome to the team! Let's build something amazing! 🎉**

**Repository:** https://github.com/CoderRvrse/Rvrse-Monitor
**Goal:** 90% Process Hacker parity in 3 months
**Status:** ✅ You're ready to start NOW!

🚀 **Happy coding!**
