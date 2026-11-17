# Sources

## People (as of 2026-11)

| GitHub | Forum | Role |
|---|---|---|
| ivan-mogilko | Crimson Wizard | lead maintainer since 2012: engine, editor, releases, most designs; 72 of the last 115 merged PRs |
| ericoporto | eri0o | maintainer: CI, ports (Android, Web, macOS), tools, deps, `atbx`, test games |
| fernewelten | fernewelten | new script compiler (`Compiler/script2`), delegates/overloading designs |
| AlanDrake | Alan v.Drake | contributor: editor, D3D11, PO, 0-255 ranges; AGS Draconian fork (`AlanDrake/ags-draconian`) |
| edmundito | edmundito | contributor: AI policy #3025, editor design, process |
| morganwillcock | morganw | contributor: packaging, Linux |
| a-riddle-fan | a-riddle-fan | contributor (2026): `ags-for-mac` (3.6.x editor on .NET 10 + Avalonia, AI-assisted), non-Windows debugger |
| messengerbag | Snarky | expert user, modules (TotalLipSync), design reviewer |
| persn · tarnos12 · selmiak · Max833 · PacketLauncher · rathann | | SDK csproj + open sprites · editor UX · editor UX · old-game compat reports · font fixes · Fedora packaging |
| dreammaster | | ScummVM (AGS engine there) |

A maintainer's statement is the strongest signal short of merged code; still "proposed".

## Places

- GitHub issues (294 open): labels `type:` (bug/enhancement/question) · `what:` (editor/engine/tools/engine plugin) · `context:` (script api, script compiler, bw-compat, game building, …) · `ags 4` / `ags3` · `res: in consideration`; milestones `4.0 Beta`, `4.0 final`, `3.6.3`, `3.6.2 maintenance`, `AGS 3: Minimal standalone tool set`; 224 open issues have no milestone
- GitHub wiki (23 pages, last edit 2026-10-15): clone `https://github.com/adventuregamestudio/ags.wiki.git` (one page name contains `:`; on Windows read it with `git show`)
- forum boards "Editor Development", "Engine Development": maintainers' proposals and user requests (~480 threads; list below); thread URL + `?all` = whole thread, browser User-Agent required
- AGS Discord: quick polls (e.g. #2605 naming 2026-04); not archived
- `Changes.txt`: what shipped, per version

Repos: `ags-manual-source` (`master` 3.x, `ags4` 4.0; user manual, one `.md` per script type) ·
`ags-manual` (pandoc build, CHM/HTML) · `ags-test-games` (auto-test games, built with `atbx`,
released as zips CI downloads) · `ags-template-source` · `ivan-mogilko/ags-refactoring`
(experiment branches: `draft--upgradesaves`, `ags4--draft-memwatch`,
`experiment--baseclassinscript`, `ags4--scriptfunctionoverload`) · `ericoporto/agstoolbox` (`atbx`)
· `rofl0r/agsutils` · `adventuregamestudio/scummvm` branch `ags`.

## Fetching

```sh
gh issue list -R adventuregamestudio/ags --milestone "4.0 Beta" --limit 100
gh issue view 2316 -R adventuregamestudio/ags            # body
gh issue view 2316 -R adventuregamestudio/ags --comments # comments only; run both
gh pr list -R adventuregamestudio/ags --state open
gh api repos/adventuregamestudio/ags/milestones --jq '.[] | "\(.title) open=\(.open_issues)"'
curl -sL -A "Mozilla/5.0" "https://www.adventuregamestudio.co.uk/forums/engine-development/<slug>/?all"
```

## Reading list (newest first)

Issues: #3019 4.0 Beta checklist (2026-07) · #2316 build tools (2024-01→2026-08) · #3177
cross-platform editor (2026-10) · #2990 + PR #3025 AI policy (2026-06→11) · #2993 audio API
(2026-06) · #2983 SDL3 (2026-06) · #2786 proposal process (2025-07) · #2549 #2856 ranges/alpha
(2024-10→2026-10) · #1228 base classes (2021-03, 2025-05) · #2058 #1833 #1834 Common separation
(2022-11→2024-08) · #1409 delegates, #2533 overloading, #1387 script order (2021→2026) · #1349 one
game loop (2021-07, 2025-01) · #1092 AGS.Types vs WinForms (2020-06) · #1462 SDK csproj (2021-12,
2025-01) · #2211 code style (2023-11) · #1400 #1371 saves (2021→2024) · #2321 undo/redo (2024-01,
2026-10) · #2447 watch variables (2024-06)

Forum, engine board: "How should a proper audio system look like?" (2018→2026) · "Load older
game saves into updated game: Attempt 2" (2024→2025) · "[EXPERIMENT] AGS 4: Custom Shaders
support" (2025) · "Why isn't AGS script much, much faster?" (2021→2024; VM facts, 3.6.1 perf work) ·
"Planning base classes in AGS script", "Coroutines in AGS", "AGS 4: Touch input API" (2025) ·
"[4.0] Allow unlimited nested script callbacks" (2025-12) · "Suggestions to 3.6" (translations,
2026) · "Running old games with 3.6.2 engine" (2024→2025) · "AGS High-Priority Improvement Needs"
(2021) · "AGS Project Goal" (2020) · "Porting engine to ScummVM" (2020→21)

Forum, editor board: "AGS 4.+ Roadmap discussion" (2024) · "GUI Controls update proposal"
(2024→2026) · "AGS 4: dialogues as files" (2026-06) · "QUESTION: Dropping Linux 32-bit binaries"
(2026-04/05; tools and Wine state) · "Asking for advice about Name/ScriptName/Description"
(2025→2026) · "I ported the AGS editor to macOS", "AGS Editor on macOS" (2023→2026) · "Feature
Request: Inspect variable while debugging" (2017→2024; Watch Variables design) · "[NOTIFICATION]
AGS version branch split" (2020→2023) · "Feature Request: Store sprites as separate files"
(2017→2022) · "Animation editor in AGS 4.0" (2025) · "My personal list of problems with AGS 4"
(2025-07) · "Room masks import is not usable" (2024)

Wiki: AGS-Coding-Conventions-(Cpp) · A-quick-guide-to-Script-API · Extending-game-and-room-data-
format · Engine-data-structure-restrictions · Script-execution, Script-memory,
Script-objects-and-Dynamic-objects, Script-compilation · AGS-Game-Build-process-(3.5.*) ·
Public-release:-procedure · AGS-4-Draft:-major-engine-refactor, AGS-4-Draft:-Game-Data-and-Entities
(drafts, not plans) · Undo-Redo-for-Editor-panes:-plan (2026-10) · Features-of-the-new-Script-
Compiler, New-compiler's-end-user-cheat-sheet (new-compiler language reference)
