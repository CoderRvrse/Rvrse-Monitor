# Upstream vs. Local Map

| Upstream Component (System Informer) | Local Status | Notes / TODO |
|-------------------------------------|--------------|--------------|
| `SystemInformer.sln` (Win32 UI + phlib + plugins) | `RvrseMonitor.sln` skeleton in place | Add dedicated core/common projects and match naming once code lands. |
| `phlib` (process/thread/handle engine) | TODO (`src/core`) | Plan to mirror `phlib` structure but keep namespaces under `rvrse`. |
| `plugins` samples | TODO (`src/plugins`) | Provide SDK header + example DLL after core ABI solidifies. |
| `kph` driver | TODO (`src/driver`) | Only bring in MIT-licensed driver pieces; ensure code signing path documented. |
| Build scripts (`build\build_release.cmd`) | `scripts\build_release_local.cmd` | Expand script matrix (Debug/driver builds) as projects appear. |
| Docs (`docs\BUILDING.txt`, wiki) | `docs/build/*.md`, `docs/design/*.md` | Flesh out with per-project instructions. |

## Gaps to close

- No NT enumeration code yet; import or reimplement the minimum subset for processes and threads.
- Driver story undefined; decide whether MVP requires kernel support or user-mode only.
- Plugin ABI unspecified; draft header in `include/` with strict versioning.

## Local-only ideas

- Strong focus on safety prompts before terminating system processes.
- Themeable UI palette with accessible defaults.
- Built-in diff/snapshot workflow for investigating transient processes.
