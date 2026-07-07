---
inclusion: manual
---
# AGS MCP — Build Tools Reference

## Critical Limitation

**The MCP cannot compile scripts.** The AGS Editor is required for compilation.
The workflow is always:

```
MCP writes .asc + Game.agf  →  AGS Editor compiles  →  MCP launches / exports
```

---

## `game_build_check`

Validates that all files referenced in `Game.agf` actually exist on disk.
Run this before asking the user to open the AGS Editor.

| Param | Type | Required | Notes |
|-------|------|----------|-------|
| `project_path` | string | yes | |
| `check_scripts` | bool | no | Check `.asc` files exist (default: true) |
| `check_sprites` | bool | no | Check `acsprset.spr` exists (default: true) |
| `check_rooms` | bool | no | Check `.crm` files exist (default: true) |
| `check_audio` | bool | no | Check audio source files exist (default: true) |

Returns: `{ "can_build": true/false, "checks": [ {"category": "scripts", "passed": true, "errors": []} ] }`

Each check has `category`, `passed`, `message` (if passed), `errors` (array of `{message}` objects).

---

## `game_build`

Reports the state of the existing compiled output in `Compiled/Windows/`.
**Does NOT trigger a rebuild.**

| Param | Type | Required |
|-------|------|----------|
| `project_path` | string | yes |
| `output_dir` | string | no | Defaults to `{project_path}/Compiled/Windows` |

Returns:
- `built: false` if output directory or `.ags` package is missing
- `built: true` with `output_files.game_package`, `output_files.game_exe`, `file_sizes.total_kb`
- Always includes `"warning": "No rebuild performed. Use AGS Editor to recompile scripts and assets."`

---

## `game_test_run`

Launches the compiled game executable. Detects stale builds.

| Param | Type | Required | Notes |
|-------|------|----------|-------|
| `project_path` | string | yes | |
| `windowed_mode` | bool | no | Default: true |

Returns:
- `launched: false` + message if no exe found in `Compiled/Windows/`
- `launched: true` with `process_id`, `exe_path`, `build_is_stale`
- If stale: `stale_hint` and `stale_sources` (list of modified files newer than the exe)

Stale detection compares `.asc`, `.crm`, and `Game.agf` modification times to the exe.
**A stale build still launches** — the warning is informational.

---

## `game_export`

Packages the compiled game into a zip archive.

| Param | Type | Required | Notes |
|-------|------|----------|-------|
| `project_path` | string | yes | |
| `export_dir` | string | yes | Destination folder |
| `version` | string | no | Default: `1.0.0` |

Copies `Compiled/Windows/` → `export_dir/Windows/`, then zips to
`export_dir/GameName-1.0.0-Windows.zip`. Also writes `README.txt` and `VERSION.txt`.

Returns: `{ "exported": true, "files": { "windows_zip": "...", "readme": "...", "version_txt": "..." }, "archive_sizes": {"windows_kb": 2048} }`

Returns `exported: false` if no compiled output exists (build first).

---

## Standard Build Workflow

```
1. game_build_check(project_path)
   → fix any missing files the MCP can fix (e.g. re-add missing rooms)
   → tell the user to rebuild in the AGS Editor if needed

2. [User opens AGS Editor → Build → Build game EXE]

3. game_build(project_path)
   → confirm compiled output exists

4. game_test_run(project_path, windowed_mode=true)
   → launch and test

5. game_export(project_path, export_dir="...", version="1.0.0")
   → package for distribution
```
