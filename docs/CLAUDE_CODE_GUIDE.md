# Claude Code Development Guide

This guide explains how to work effectively with Claude Code as your AI development assistant for the Rvrse Monitor project.

---

## 🤖 What is Claude Code?

Claude Code is an AI-powered development assistant that can:
- Read, write, and edit code
- Run builds and tests
- Search the codebase
- Execute bash commands
- Commit and push to Git
- **Track tasks using a built-in todo list**

---

## ✅ The Todo List System (CRITICAL)

### Why Use the Todo List?

Claude Code has a **built-in task tracking system** that:
- Helps Claude remember what to do next
- Prevents Claude from forgetting important steps
- Shows you progress in real-time
- Ensures all tasks are completed before finishing

**IMPORTANT:** You should **always ask Claude to use the todo list** for any multi-step tasks.

### How to Use It

#### **Starting a New Task**
When asking Claude to do something with multiple steps:

```
YOU: "Implement network connections view with TCP/UDP enumeration,
      UI integration, and unit tests. Use the todo list to track this."
```

Claude will create a todo list like:
```
[ ] Design network connection data structures
[ ] Implement TCP enumeration
[ ] Implement UDP enumeration
[ ] Add UI viewer for connections
[ ] Write unit tests
[ ] Update documentation
```

#### **Checking Progress**
Ask Claude:
```
YOU: "Show me the current todo list"
```

Claude will display:
```
[✓] Design network connection data structures
[✓] Implement TCP enumeration
[→] Implement UDP enumeration (in progress)
[ ] Add UI viewer for connections
[ ] Write unit tests
[ ] Update documentation
```

#### **Updating the List**
If priorities change:
```
YOU: "Add a new task to the todo list: optimize memory usage in snapshots"
```

Or:
```
YOU: "Remove the documentation task from the todo list, we'll do that later"
```

#### **Clearing Completed Tasks**
When a feature is done:
```
YOU: "Clear the todo list, we're done with this feature"
```

---

## 📋 Best Practices for Working with Claude Code

### 1. **Always Validate Before Pushing**

**MANDATORY:** Run this before every push:
```powershell
.\scripts\validate_before_push.ps1
```

This checks:
- Debug and Release builds succeed
- All unit tests pass
- Performance metrics are acceptable
- No secrets in commits
- Binary size is reasonable

**DO NOT push if validation fails!**

### 2. **Use the Todo List for Complex Tasks**

For tasks with 3+ steps, always say:
```
"Use the todo list to track this task"
```

**Examples of when to use it:**
- ✅ "Implement network connections feature" (multi-step)
- ✅ "Refactor ProcessSnapshot for performance" (multi-step)
- ✅ "Add TCP/UDP filtering UI" (multi-step)
- ❌ "Fix typo in README.md" (single step, no need)
- ❌ "Add a comment to main.cpp" (trivial, no need)

### 3. **Be Specific About Requirements**

**Bad request:**
```
"Add network stuff"
```

**Good request:**
```
"Implement NetworkSnapshot class that enumerates TCP/UDP connections
using GetExtendedTcpTable and GetExtendedUdpTable APIs. Include:
- IPv4 and IPv6 support
- Per-process filtering
- Connection state tracking
- Unit tests for filtering logic
Use the todo list to track this."
```

### 4. **Ask Claude to Read Tests First**

Before implementing a feature:
```
YOU: "Read the existing unit tests in tests/main.cpp to understand
      the testing pattern, then implement network enumeration tests"
```

This ensures consistency with existing code.

### 5. **Request Performance Validation**

For snapshot or performance-critical code:
```
YOU: "After implementing this, run the build script and check telemetry
      to ensure no performance regression"
```

### 6. **Let Claude Handle Git Operations**

Claude can commit and push for you:
```
YOU: "Commit these changes with a good commit message and push to main"
```

Claude will:
- Stage the files
- Write a descriptive commit message
- Include performance metrics and test results
- Push to the remote repository

### 7. **Use Parallel Work for Speed**

For independent tasks:
```
YOU: "Read process_snapshot.cpp and network_snapshot.cpp in parallel"
```

This is faster than reading them sequentially.

---

## 🎯 Common Tasks & How to Ask

### **Implementing a New Feature**

```
YOU: "I need to add a feature that shows network connections per process.
      Requirements:
      - Use GetExtendedTcpTable for TCP connections
      - Use GetExtendedUdpTable for UDP connections
      - Add a 'Network...' button to the UI
      - Show Protocol, Local Address, Remote Address, State, PID columns
      - Include unit tests

      Use the todo list to track this feature."
```

### **Fixing a Bug**

```
YOU: "The module viewer crashes when double-clicking System process (PID 4).
      Debug this, fix it, add a test case, and ensure no regression.
      Use the todo list."
```

### **Refactoring Code**

```
YOU: "Refactor ProcessSnapshot::Capture() to reduce its complexity.
      Ensure all tests still pass and performance doesn't regress.
      Track with todo list."
```

### **Adding Tests**

```
YOU: "Add unit tests for the new NetworkSnapshot class.
      Test cases:
      - Enumerate TCP connections
      - Enumerate UDP connections
      - Filter by process ID
      - Handle empty results
      - IPv4 and IPv6 addresses

      Track with todo list."
```

### **Investigating Performance**

```
YOU: "ProcessSnapshot is taking 15ms instead of 4ms after my recent changes.
      Investigate the regression, identify the cause, and fix it.
      Use the todo list to track the investigation."
```

---

## 🔍 Debugging with Claude Code

### **Reading Error Messages**

If build fails:
```
YOU: "The build failed with linker errors. Read the error output,
      identify the issue, and fix it."
```

### **Analyzing Crashes**

```
YOU: "The app crashes when I click Modules. Run it in the debugger
      and tell me what's wrong."
```

Note: Claude **cannot** actually run GUI debuggers, but can:
- Read crash logs
- Analyze code for potential null pointer dereferences
- Check array bounds
- Review error handling

### **Performance Profiling**

```
YOU: "Check the latest telemetry_*.json file and tell me if there
      are any performance regressions."
```

---

## 🚫 What Claude Code Cannot Do

Claude Code is powerful but has limitations:

❌ **Cannot click through the UI**
- Claude can build the app but can't manually test UI interactions
- **You must test the UI manually** after Claude builds it

❌ **Cannot install kernel drivers**
- Requires admin privileges and code signing
- **You must install drivers locally** on your office PC

❌ **Cannot debug running GUI applications**
- Claude can't attach a debugger to a live process
- But can analyze code, review error logs, read crash dumps

❌ **Cannot access external services**
- No access to private APIs, databases, etc.
- Works only with the local codebase

✅ **Can do everything else:**
- Build, test, commit, push
- Read/write/edit code
- Search codebase
- Run command-line tools
- Analyze performance metrics

---

## 🎓 Advanced Tips

### **1. Iterative Development**

Work in small iterations:
```
YOU: "First, implement just the TCP enumeration. Show me the code
      before moving to UDP."
```

This lets you review progress before Claude goes too far.

### **2. Code Review Requests**

```
YOU: "Review my changes in network_snapshot.cpp and look for:
      - Memory leaks
      - Buffer overflows
      - Race conditions
      - Performance issues"
```

### **3. Ask for Alternatives**

```
YOU: "I want to filter connections by process. Show me two different
      approaches and explain the tradeoffs."
```

### **4. Request Explanations**

```
YOU: "Explain how the on-demand module enumeration works in
      ProcessSnapshot::EnumerateModules()"
```

### **5. Batch Related Tasks**

```
YOU: "Implement network enumeration, add UI integration, write tests,
      and update documentation - all in one session. Use the todo list
      to track everything."
```

---

## 📝 Commit Message Best Practices

Claude will write commit messages for you. Make sure they're good:

### **Good Commit Messages:**

```
feat: Add TCP/UDP connection enumeration to NetworkSnapshot

Implements system-wide network connection monitoring using Windows IP Helper API.

Changes:
- src/core/network_snapshot.cpp: Add GetExtendedTcpTable/UdpTable wrappers
- src/core/network_snapshot.h: Define connection data structures
- tests/main.cpp: Add unit tests for connection filtering

Performance: NetworkSnapshot avg 8.5 ms
Tests: All unit tests pass

🤖 Generated with Claude Code
Co-Authored-By: Claude <noreply@anthropic.com>
```

### **Bad Commit Messages:**

```
Update files
```

```
WIP
```

```
Fix bug
```

**If Claude writes a bad commit message, ask for a better one:**
```
YOU: "Rewrite that commit message to be more descriptive"
```

---

## 🆘 Common Issues & Solutions

### **Issue: Claude Forgot a Task**

**Solution:** Use the todo list!
```
YOU: "You forgot to write unit tests. Add that to the todo list and do it now."
```

### **Issue: Claude Made Too Many Changes at Once**

**Solution:** Request incremental changes
```
YOU: "That's too much at once. Let's do this in smaller steps.
      First, just implement the data structures. Use the todo list."
```

### **Issue: Build Passes But Tests Fail**

**Solution:**
```
YOU: "The tests are failing. Read the test output, identify which tests
      failed, and fix the issues. Update the todo list to track fixes."
```

### **Issue: Performance Regression**

**Solution:**
```
YOU: "The validation script shows ProcessSnapshot regressed to 15ms.
      Profile the code, identify the bottleneck, and optimize it.
      Track with todo list."
```

### **Issue: Claude Doesn't Understand the Codebase**

**Solution:**
```
YOU: "First, read these files to understand the architecture:
      - docs/design/system-monitor-architecture.md
      - src/core/process_snapshot.cpp
      - src/core/handle_snapshot.cpp

      Then implement the network feature following the same patterns."
```

---

## 🎯 Example: Full Feature Implementation Session

Here's a complete example of asking Claude to implement a feature:

```
YOU: "I need to implement a network connections viewer for Rvrse Monitor.

Requirements:
1. Create NetworkSnapshot class similar to ProcessSnapshot
2. Enumerate TCP connections using GetExtendedTcpTable (IPv4 + IPv6)
3. Enumerate UDP connections using GetExtendedUdpTable (IPv4 + IPv6)
4. Data structure should include: protocol, local/remote address,
   local/remote port, connection state, PID, process name
5. Add a 'Network...' button to the main UI
6. Display connections in a sortable list view
7. Add filtering by process ID
8. Write unit tests for enumeration and filtering
9. Update telemetry to track NetworkSnapshot performance
10. Ensure NetworkSnapshot completes in <10ms average

Use the todo list to track all these tasks. Before pushing, run the
validation script to ensure everything passes."
```

**Claude will:**
1. Create a todo list with all tasks
2. Implement each task step-by-step
3. Mark tasks complete as they finish
4. Run validation before pushing
5. Commit with a descriptive message
6. Push to the repository

**You will:**
1. Pull the changes: `git pull origin main`
2. Build and test UI manually: `.\RvrseMonitorApp.exe`
3. Verify network connections display correctly
4. Report any bugs via GitHub issues

---

## 📚 Resources

- **Testing Checklist:** [docs/TESTING_CHECKLIST.md](TESTING_CHECKLIST.md)
- **Team Onboarding:** [docs/TEAM_ONBOARDING.md](TEAM_ONBOARDING.md)
- **Architecture Docs:** [docs/design/system-monitor-architecture.md](design/system-monitor-architecture.md)
- **Office PC Validation:** [docs/FOR_OFFICE_PC.md](FOR_OFFICE_PC.md)

---

## 🎉 Summary

**Key Takeaways:**
1. ✅ **Always use the todo list** for multi-step tasks
2. ✅ **Run validation script** before every push
3. ✅ **Be specific** in your requests
4. ✅ **Let Claude handle git** operations
5. ✅ **Test UI manually** after Claude builds it
6. ✅ **Use incremental development** for complex features
7. ✅ **Review commit messages** before pushing

**With these practices, you'll get the most out of Claude Code and maintain high code quality!** 🚀

---

**Questions? Create a GitHub issue or check the [TEAM_ONBOARDING.md](TEAM_ONBOARDING.md) guide.**
