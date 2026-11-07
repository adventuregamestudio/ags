# Decisions and positions

**decided** = agreed or in code; **position** = a maintainer's stated intent, not contradicted;
proposals live in `roadmap.md`. Compiled 2026-11-02; re-check sources older than a year.
Names: `sources.md`.

## Project

| # | Rule | Status, source |
|---|---|---|
| P1 | Two generations in one repo: `master` = 3.x (runs every game since 2.50; fixes, performance, backward-compat, tooling), `ags4` = 4.0 (breaking) | decided; forum "AGS version branch split" ivan-mogilko 2020-11-26; CONTRIBUTING.md |
| P2 | 4.0 compat: Editor imports projects saved by 3.6.0+ (older: open in 3.6.x first); running pre-4.0 compiled games not guaranteed; reads 3.5.0+ saves. Code: `kGameVersion_LowSupported = kGameVersion_360_21`, `kRoomVersion_LowSupport = kRoomVersion_3508` (README still says 3.4.1) | decided; ivan-mogilko forum 2023-05-27; `Changes.txt` 4.0.0; `game_version.h` |
| P3 | 4.0 Beta = no more major breaking changes (feature removals, forced manual project upgrades, update-order changes) except the #3019 list; script control over new built-in features | position; ivan-mogilko #3019 2026-07, forum 2025-10-29 |
| P4 | 3.x after 3.6.3: fixes, cleanup, old-game compatibility; a 3.6.4 may exist only to ship the build tools | position; #2316 comments 2026 |
| P5 | Issue first for features, refactors, cross-tree edits; proposal guidelines stay informal ("align with existing code, don't break existing stuff, common sense") | decided; CONTRIBUTING.md; #2786 ivan-mogilko 2025-07-28 |
| P6 | Cross-platform Editor: not planned; a whole new editor will not be adopted (C++/ImGui AI-made one was asked not to be submitted). Welcome in small PRs: `AGS.Types` without WinForms/`System.Drawing` (#1092), `AGS.Native` platform-neutral or removed, build via the tools. Editor must keep Windows 8. UI framework undecided | position; ivan-mogilko forum 2026-05-21; #3177 2026-10-30/11-01 |
| P7 | AI tooling: code accepted if a person authors, understands and discloses it (PR description, no trailers); no generated art/audio/UI design/user-facing text/manual prose; no bot PRs/issues/comments; fork experiments rewritten before submission | decided; CONTRIBUTING.md "Use of AI Tooling" (PR #3025, merged 2026-11-03); #2990 |
| P8 | Artistic License 2.0; no GPL/LGPL code absorbed; linking separately is fine | decided; License.txt; #2990 |

## Formats

| # | Rule | Status, source |
|---|---|---|
| F1 | Extend game/room formats by appending self-described blocks; never insert fields mid-layout. Not enforced for saves on 3.x (#1400, deferred to 4.0) | decided; wiki "Extending game and room data format"; #828; PR #1183 |
| F2 | Serialize field by field through `Stream`; never export raw engine variables/arrays to script (properties/functions with offset readers instead) | decided; wiki "Engine data structure restrictions" |
| F3 | Fixed project folder names (`Compiled`, `_Debug`; 4.0: `Rooms`, `Fonts`, `Dialogs`); none configurable; pre-existing folder → `Backup-<name>` on upgrade | decided; forum "dialogues as files" 2026-06-06 |
| F4 | `ags4`: dialog scripts are `Dialogs/<name>.dasc` (format 4.0.0.33); rooms are `Rooms/<n>/` sources; translation sources are PO (`#:` without line numbers, `msgctxt` for parser words) | in code; #1008, PR #2978, #1780 |
| F5 | Compiled scripts may be separate package assets (engine since 3.6.1); suggested for the room compiler (`roomN.o`) | position; ivan-mogilko #2316 |
| F6 | Script API versioning: `#ifdef SCRIPT_API_vXXX` for new, `SCRIPT_COMPAT_vXXX` for deprecated, both branches for overriding changes; never `#ifver` in `agsdefns.sh`; names carry arg count `^N` | decided; wiki "A quick guide to Script API" |
| F7 | 4.0 moves 0-100 parameters (opacity, tint, light) to 0-255; list unfinished; old-function handling (API switch vs compat plugin vs doc shims) unsettled | position; #2856, #2549 (AlanDrake vs ivan-mogilko 2026-10-03) |

## Tooling

| # | Rule | Status, source |
|---|---|---|
| T1 | Game building = standalone C++ tools (`Tools/`), reusing engine code; the Editor's switch is a separate task | decided goal; #872 → #2316 |
| T2 | Never embed the script API header in a tool binary; ship it as a file | position; ivan-mogilko #2316 |
| T3 | Tools log through the Common log system (`debug::out`); `-v` = log config | position; ivan-mogilko #2530 (ericoporto implementing) |
| T4 | Plugin script headers: `plugin2ash` tool (#2771) vs `<plugin>.ash` files next to plugins; ivan-mogilko prefers files (2026-09-03) | open |
| T5 | Windows x64: "x64-only in ags4 fine, nothing radical in 3.x"; plugin bitness blocks the Editor | position; #2282 |
| T6 | Linux: keep the i386 Editor-shipped runtime (2026-04-23); shipped runtime built on Debian Jessie, Bullseye proposed (#3026); CI = GitHub Actions since 2026 | position; ericoporto |
| T7 | SDL3: not before it settles ("at least a year", 2026-06); then a feature branch for both 3.x and 4.0 | position; #2983 |
| T8 | New compiler default for new 4.0 projects; old compiler removal is a 4.0 Beta item (#3172); add a non-parallel compile option first (#3178, agreed 2026-10-31) | in code / proposed |
| T9 | Style: wiki conventions for new code; in edits, consistency with surrounding code first; repo-wide `.clang-format` proposed (#2211, 2023) and not adopted | decided / not adopted |
| T10 | Third-party code: link external libraries; vendor only if unavailable on Linux, tiny, or pinned/modified | decided; wiki "Adding third-party libraries" |

## Engine (4.0)

| # | Rule | Status, source |
|---|---|---|
| E1 | 2016 old-saves prototype rejected; shipped instead (3.6.2, 4.0): load saves with less data, `validate_restored_save(RestoredSaveInfo*)`, `OPT_SAVECOMPONENTSIGNORE`, `Game.ScanSaveSlots`; object identity in game data still unsolved | decided; ivan-mogilko 2021-05-27; forum "Attempt 2" 2024-2025; #1371 |
| E2 | 4.0 VM = `ScriptExecutor` with saveable script threads (#2621); GC postponed while scripts run (hotfix `364edb820`, #2955) | in code |
| E3 | Post-script actions must run after all modules' callbacks (#1144); a guaranteed init hook alongside `game_start` (#1155): accepted 4.0 Beta changes | position; #3019 |
| E4 | Room previews/thumbnails: separate assets, not inside `.crm`; 4.0 room templates = room folders | position; ivan-mogilko forum 2023-09-28 |
