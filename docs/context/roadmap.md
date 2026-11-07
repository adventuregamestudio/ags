# Roadmap and active work

Snapshot 2026-11-02. Numbers = adventuregamestudio/ags issues/PRs. Status: **done**, **in code**,
**PR**, **proposed** (maintainer design, no commitment), **open**. Refresh:
`gh issue list -R adventuregamestudio/ags --milestone "4.0 Beta"` (see `sources.md`).

## State

| Line | Version | State |
|---|---|---|
| AGS 3 | 3.6.3 RC (`release-3.6.3`); `master` = 3.6.4 dev | maintenance: milestones `3.6.2 maintenance` (9 open), `3.6.3` (8 open), old-game compat reports (#3106 #3125 #3133 #3134 #3138); 3.6.4 maybe only to ship the tools (#2316) |
| AGS 4 | 4.0.0.33 Beta (`ags4`) | finishing: `4.0 Beta` 19 open, `4.0 final` 25 open; 294 open issues total, 93 `ags 4` |

Last 12 months: 115 PRs merged (70 `master`, 25 `ags4`, 20 release branches; 72 ivan-mogilko,
22 ericoporto, 9 a-riddle-fan); 123 issues closed.

## 4.0 Beta checklist (#3019, ivan-mogilko 2026-07)

| Item | Status |
|---|---|
| dialog scripts as files (#1008, PR #2978) | in code (4.0.0.33) |
| directory-based "import existing room" (#2561); 4.0 room templates (#2562) | open |
| review Joystick and Touch API | open |
| audio API (#2993: `AudioPlayer`, relative volumes, `AudioType` objects, `Audio.AudioChannels[]`) | proposed, assigned ivan-mogilko |
| `SCR_NO_VALUE` (#1247); `caseSensitive` default true (#2502) | open |
| 8-bit alpha / `.Alpha` (#2549); 0-255 ranges (#2856) | proposed; PR #3119 (AlanDrake) open |
| SDL_ttf fonts (#1528, PR #2711) | done; wrapping perf #3049/#3050 → 4.0 final |
| deprecate ASCII mode (#2553) | open |
| built-in mechanics → script: #472, #1283, #3053, #3054, #3055 | open |
| execution order: #1144, #1155, #2821 | open |
| inventory item IDs 0-based | open |
| remove old compiler (#3172); non-parallel compile option (#3178) | proposed 2026-10 |
| colour remapping via attributes (#3168) | proposed |

## 4.0 final (selection)

open sprite storage #1281 (persn branch `feature/open-sprites`) · PO completion #1780 ·
standalone new-compiler builds #2328 · custom properties as extender attributes #2605 (collision →
hard error leaning) · AutoComplete via compiler parser #3155 · screen-res render targets #2750 ·
multidim dynamic array creation #2758 · new-compiler bugs #2809 #2981 #3004 · local coords #2867 ·
room rebuild on array-size change #2915 · save room without recompile #3060 · font reload #3062 ·
D3D11 testing #3115 · colour conversion tests #3167

## Build tools (#2316, milestone "AGS 3: Minimal standalone tool set", 9 open)

| Item | Status |
|---|---|
| `agf2dta` `agfexport` `agf2dlgasc` `agscc` `agspak` `crmpak` `spriteimport` `spritepak` `trac` `agsbuild` | done; `spritepak` fixes #3083 |
| plugin script-header extraction (#2771) | open: tool vs `<plugin>.ash` files |
| 4.0 room compiler (`Rooms/<n>/` → `.crm`) | "ticket TBD"; implemented in the AGS Draconian fork (`crmbuild`), offered in PR #3182 |
| PO translation compiler | "ticket TBD"; also in PR #3182 |
| lipsync compiler #3065 · `--version` #3068 · `agscc -I` #3070 · tool logging #2530 · AGFReader tests #3147 · packaging (TBD) | proposed |
| prerequisites: Common without external impls #1833 (proposed), without globals #1834 (mostly done), data/runtime split #2058 (proposed) | |

## Open PRs

| PR | Base | What |
|---|---|---|
| #3182 AlanDrake | ags4 | AGS 4 support in the tools (`crmbuild`, table-driven `agfreader`, PO `trac`, `agsbuild --json`), 24 commits |
| #3176 #3175 #3119 AlanDrake | ags4 | void-returns-void compiler; Editor antialiased fonts; 8-bit alpha + ranges |
| #3162 a-riddle-fan | master | engine debugger on non-Windows (`--debugger-comm pipe\|file`) |
| #3183 #3184 PacketLauncher | master | TTF outline preview; 4-bit WFN fonts |
| #3173 ivan-mogilko | release-3.6.3 | CMake: system miniz |
| #2782 ivan-mogilko | ags4 | `ShaderProgram` from string |
| #2770 ericoporto | master | SKETCH: build generator for the tools |
| #2256 ivan-mogilko | master | DRAFT: `IAGSAudioPlayer` plugin API |

## Proposed, unscheduled (maintainer designs)

- engine: audio API #2993 · one game loop / state stack #1349 ("a rewrite"; blocked by blocking script calls) · `Drawable` system #2628 · HW-accelerated `DynamicSprite` #2059 · configurable script-error behaviour #1488 (partly #3086) · automatic savegame tests #2814 · run from unpacked assets #3044 · timers as objects #1958 · dynamic audio clips (forum 2025-01) · `Character.Following` (forum 2024-11) · immediate nested callbacks PR #2914 (2025-12) · `WaitAsync` coroutines (forum 2025-04; saves with suspended scripts unsolved) · base classes `Entity`→`GameEntity`→… #1228 + data/runtime split #2058 ("far too late for first 4.0", one batch later)
- language (fernewelten's compiler): overloading #2533 (import-only rule discussed 2026-01) · delegates #1409 (WIP commit) · type order independence #1387 · initializer lists #2152 · user variadics (forum 2022-12, type-safety dispute)
- Editor: undo/redo #2321 #2535 (wiki "Undo-Redo for Editor panes: plan", 2026-10) · clipboard via own serialization #2537 · `AGS.Types` without WinForms #1092 · SDK csproj + .NET 4.8 #1462 (CI container blocks) · Windows x64 Editor #2282 · "Overview of things" (forum 2025-08) · Room Explorer tree (forum 2023-24) · GUI controls update (OnClick on all controls, 9-slice; only `BorderWidth`/`Padding` landed in 3.6.3; forum 2024-26) · Name/ScriptName/Description terms PR #3014 · event-handler module per object PR #2534
- platform: SDL3 #2983 (not before 2027) · newer Linux build distro #3026 · iOS has no CI

## Parked / rejected (maintainer reason, date)

- old-saves prototype (2016): "approach not promising", ivan-mogilko 2021-05-27
- Editor Git integration: plugin at most, ivan-mogilko/ericoporto 2026-08-13
- in-Editor module manager: maintenance burden, ericoporto 2024-02-20
- a new cross-platform editor submitted whole: declined, ivan-mogilko 2026-05-21 (decisions P6)
- engine-level accessibility: "no manpower", AlanDrake 2024-05-28; plugin/module route
- parallax as engine feature: not in any milestone, ivan-mogilko 2025-01-06
- vector walk-boxes: needs another pathfinder; script-side via `WalkPath`, ivan-mogilko 2024-06-03
- start/stop/turn animation sections: postponed past 4.0, ivan-mogilko 2025-10-29
- `f` prefix for flag enums: breaks scripts/habits, 2024-12-16
- forward-compatible save format on 3.x: deferred to 4.0, ivan-mogilko #1400
