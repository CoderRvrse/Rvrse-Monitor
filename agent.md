# agent: System Monitor Fork Lead

## Mission

You are the lead developer for a new Windows system monitor inspired by **System Informer** (successor to Process Hacker 2).

**Hard constraint:**
- Keep the **same primary languages and platform style as Process Hacker 2 / System Informer**:
  - Native **C/C++** (Win32/NT, low-level Windows APIs).
  - Use the same style of build tooling as upstream (Visual Studio solutions, `.vcxproj`, batch build scripts, etc.).
  - Only use C#/.NET where upstream already does (e.g., tools), *not* as a replacement for the core app.

Your job is to:

- Use **winsiderss/systeminformer** (MIT) as the *primary upstream reference*.
- Build a **clean, legally-compliant, well-structured fork** with the user's own branding, UX, and feature priorities.
- Never "rip and reskin" lazily - always understand the code, then adapt or extend it.
- Keep all work aligned with the existing project style and goals in this repo.

The user already has a project here that reflects what they want; your job is to align upstream System Informer concepts with this project's architecture and style **without changing the core language/toolchain**.

---

## Legal & Ethical Guardrails (non-negotiable)

- **Upstream base**: Treat **System Informer** as the canonical reference:
  - GitHub: `https://github.com/winsiderss/systeminformer`
  - License: **MIT** - forkable and adaptable with attribution.
- **Avoid GPL contamination**:
  - Original **Process Hacker 2.x** is GPL-3.0; do **not** pull in unique GPL-only code or text from old trees.
  - Work off the MIT-licensed System Informer code and patterns.
- **Attribution**:
  - Preserve upstream license headers where present.
  - Keep a clear `THIRD_PARTY.md` / `CREDITS` section listing Winsider Seminars & Solutions, upstream repo links, and any other reused MIT components.
- **No trademark confusion**:
  - Do **not** use the names "System Informer" or "Process Hacker" as the product name, logo, or icon.
  - You may reference them in docs as "Upstream project: System Informer (MIT)" or "Process Hacker lineage".

If any task conflicts with these rules, you must call it out explicitly and propose a safe alternative.

---

## Phase 0 - Repo Discovery & Ground Truth

**Goal:** Understand both upstream and this repo before touching code.

- [ ] Locate and skim the upstream project:
  - [ ] `README.md`, `CONTRIBUTING.md`, `LICENSE`, and any `build/` docs.
  - [ ] Identify primary components:
    - C/C++ core engine (process / thread / module / handle / memory / network inspection)
    - Kernel driver(s), if any
    - Native GUI frontend (Win32/WTL/custom)
    - Plugin system, helper tools
- [ ] In this repo (the user's project):
  - [ ] Read `README`, any `ARCHITECTURE.md`, `CLAUDE.md`, `DESIGN.md`, etc.
  - [ ] Map:
    - [ ] Where the C/C++ code lives (src, phlib, driver, plugins, ui, tools, etc.)
    - [ ] Visual Studio solutions & projects currently used
    - [ ] Any custom build scripts (batch, PowerShell)
  - [ ] Write a short architecture note in `docs/notes/upstream-vs-local.md`:
    - [ ] "Upstream component -> Local equivalent or TODO"
    - [ ] Gaps where upstream has a feature but this project does not
    - [ ] Features this project has that upstream doesn't

Output: a **high-level comparison map**, no intrusive edits yet.

---

## Phase 1 - Build & Environment Setup (Native C/C++)

**Goal:** Ensure we can build both upstream and this project reliably using the same style of toolchain.

### 1. Upstream System Informer build

- [ ] Document prerequisites in `docs/build/upstream-system-informer.md`:
  - [ ] Visual Studio version (e.g., VS 2022) + "Desktop development with C++" workload.
  - [ ] Any required Windows SDK versions.
- [ ] Clone upstream locally (read-only reference).
- [ ] Run documented build scripts, e.g.:
  - [ ] `build\build_init.cmd`
  - [ ] `build\build_release.cmd`
- [ ] If build fails, log:
  - Exact error text
  - VS/SDK versions
  - Fix/workaround if known
- [ ] Confirm:
  - [ ] The resulting EXE starts, shows process tree, and basic views.

### 2. Local project build

- [ ] Write `docs/build/local-project.md` capturing:
  - [ ] Which `.sln` to open.
  - [ ] Target config (Debug/Release, x64/x86).
  - [ ] Any required environment variables or SDK/tool dependencies.
- [ ] Ensure a one-shot script exists, e.g.:
  - `scripts/build_release_local.cmd` or `.ps1` that calls MSBuild on the main solution.

**Important:** Do *not* introduce new languages or frameworks (like WPF or Qt). Stay in native C/C++ Windows land unless you are mirroring upstream's own patterns.

Output: reproducible build steps for **both** upstream and local project, using C/C++ + Visual Studio.

---

## Phase 2 - Architecture Mapping (Upstream -> Local)

**Goal:** Design a clean integration of upstream concepts into this project, still in C/C++.

- [ ] Create `docs/design/system-monitor-architecture.md`:
  - [ ] Describe your desired layout in C/C++ terms, for example:

    - `src/core/` - core engine: NT/Win32 wrappers, process/thread/module/handle abstractions
    - `src/ui/` - Win32/WTL views: process tree, properties dialogs, graphs
    - `src/plugins/` - plugin interfaces and sample plugins
    - `src/driver/` - kernel drivers (if used)
    - `src/common/` - shared utilities, string helpers, logging

- [ ] From upstream, list **capability buckets**:
  - [ ] Process/Thread/Handle views
  - [ ] Service manager
  - [ ] Network connections
  - [ ] Disk/IO, memory usage, graphs
  - [ ] System information
  - [ ] Plugin system
- [ ] For each bucket:
  - [ ] Mark as:
    - [ ] "MVP: must ship in v1"
    - [ ] "Nice-to-have: v1.x"
    - [ ] "Later: v2+"
- [ ] Align with local project:
  - [ ] Reuse any C/C++ modules already present.
  - [ ] Plan adapter layers instead of forking everything blindly.

Output: a **C/C++-focused design doc** with a migration/extension plan.

---

## Phase 3 - Fork, Names, and Branding (Native App)

**Goal:** Create a clearly separate native app identity.

- [ ] Decide a working codename (e.g., `Rvrse Monitor` - final name set by the user).
- [ ] Update resource & metadata files:
  - [ ] `.rc` files: product name, description, version, company.
  - [ ] Any `PHAPP_NAME`/`APPNAME` macros in headers or source.
- [ ] Replace icons:
  - [ ] Introduce a **new** `.ico` set for the app.
  - [ ] Do **not** reuse System Informer or Process Hacker icons.
- [ ] Add/verify `LICENSE`:
  - [ ] Select a compatible license (MIT recommended).
  - [ ] Include attribution sections mentioning System Informer and authors.
- [ ] Add `CREDITS.md`:
  - [ ] List upstream System Informer (MIT) and any other third-party C/C++ libs.

Output: a native C/C++ binary that has a distinct name, icon, and identity.

---

## Phase 4 - Core Feature Parity (MVP, C/C++ Implementation)

**Goal:** Implement a minimal but solid feature set in native C/C++.

**MVP checklist:**

- [ ] **Process list window**:
  - [ ] C/C++ implementation of a process tree or list using Win32/WTL/custom controls.
  - [ ] Columns: PID, process name, CPU, memory, user.
  - [ ] Context menu actions: terminate, properties, open location.
- [ ] **Process properties dialog**:
  - [ ] Tabs: `General`, `Threads`, `Modules`, `Handles`, `Memory`.
  - [ ] All implemented using native dialog templates and C/C++ handlers.
- [ ] **System summary**:
  - [ ] Basic CPU/RAM graphs drawn with GDI/GDI+ or same technique as upstream.
  - [ ] Overall process count, thread count, handle count.
- [ ] **Filter/search**:
  - [ ] Simple text filter for process name / PID, implemented in C/C++.
- [ ] **Plugin hook**:
  - [ ] A basic plugin interface (C API or C++ classes) similar to upstream, with at least one example plugin compiled as a DLL.

For each feature:

- [ ] Encapsulate low-level NT/Win32 calls in reusable C/C++ helper modules.
- [ ] Avoid inline API calls spread across the UI; keep core logic in `core/`.

Output: a working native system monitor that feels similar to Process Hacker/System Informer but branded and structured for this project.

---

## Phase 5 - "Better than Upstream" (Still Native)

**Goal:** Improve UX, observability, and safety while staying in C/C++.

Examples:

- **UX**:
  - [ ] Faster keyboard shortcuts for common actions (terminate, properties, search).
  - [ ] More readable color scheme and customizable themes (still using native drawing).
- **Observability**:
  - [ ] Snapshot/diff of process lists implemented as C/C++ utilities.
  - [ ] Per-process history graphs for CPU/RAM (using the same drawing stack as main graphs).
- **Safety**:
  - [ ] Warning layer in C/C++ before killing critical processes.
  - [ ] Optional read-only mode controlled via config or command-line.

For each enhancement:

- [ ] Document design in `docs/design/enhancements/NAME.md`.
- [ ] Implement in C/C++ aligned with the current project style.

---

## Phase 6 - Quality, Tests, and Release Flow

**Goal:** Make the C/C++ project production-grade.

- [ ] Add at least basic tests (where feasible):
  - [ ] Core process enumeration modules.
  - [ ] Any parsing/serialization logic.
- [ ] Add CI (GitHub Actions or similar) that:
  - [ ] Installs required VS build tools.
  - [ ] Builds the main solution in Release x64.
  - [ ] Runs any tests.
  - [ ] Publishes artifacts on tags.
- [ ] Create `RELEASE_CHECKLIST.md`:
  - [ ] Version bump in resource files.
  - [ ] Changelog updated.
  - [ ] CI green.
  - [ ] Artifacts built (and optionally code-signed).

---

## How to Use This Agent File

When an AI code assistant is loaded on this repo:

1. **Read this `agent.md` completely.**
2. Respect the hard constraint: **do not change the core language/toolchain away from native C/C++ Windows.**
3. Treat Phases and checklists as the roadmap.
4. For each user request:
   - Identify the relevant Phase.
   - Explain what you're touching (files / modules).
   - Make minimal, focused edits consistent with upstream style and this repo's conventions.
5. Never violate the Legal & Ethical Guardrails above.

The goal is a **native Windows system monitor**, same language family as Process Hacker 2 / System Informer, but cleaner, better structured, and branded for the user's ecosystem.



