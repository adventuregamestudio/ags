# AGENTS.md

Adventure Game Studio: engine (C++), Editor (C#, Windows only), script compiler and build tools
in one repo. Policy: `CONTRIBUTING.md` (wins on conflict). Knowledge: `docs/context/` (load per
task, never all of it).

## Load by task

- any change: this file, `git log -5`, `docs/context/repository.md` "Where a change goes"
- C++ (`Engine/`, `Common/`, `Compiler/`, `Tools/`): `docs/context/repository.md`, `docs/context/workflows.md`
- Editor C#: same, plus `docs/context/decisions.md` P6
- script API, data formats, saves: `repository.md` "Game data and compatibility"
- design or planning: `decisions.md`, then `roadmap.md`; search issues first (`sources.md` "Fetching")
- issue/PR text for the human to post: `workflows.md` "Submit", `.github/pull_request_template.md`

## Do not

- open PRs, issues or comments, or draft replies to reviewers; commit or push unless asked,
  each time
- add `Co-Authored-By` or tool trailers (AI use is disclosed in the PR description, by the human)
- generate art, audio, fonts, UI design, user-facing text, templates or manual prose
- refactor, reformat or restructure beyond the asked change; cross-tree changes need an issue first
- add dependencies, abstractions or defensive code nobody asked for
- paste code from GPL/LGPL projects (AGS is Artistic License 2.0)

## Facts

- `master` = AGS 3.x, must run every game since 2.50. `ags4` = 4.0 Beta; breaking changes only
  per issue #3019. Check `version.json`. 3.x fixes go to `master` (or `release-3.6.3`) and are
  merged forward; 4.0-only code to `ags4`.
- Three build systems kept in sync by hand: CMake (engine, compiler, tools, tests), MSBuild
  `Solutions/*.sln` (the only way to build the Editor), Makefiles (`Engine/`, `Compiler/`,
  `Tools/*`). A new source file goes into all three.
- Formats are append-only: new game/room data = extension block; save changes = new
  `kSvgVersion`; never alter an existing layout.
- The script API is an ABI: registered names (`"Character::Walk^4"`) and the offset switches in
  `Engine/ac/dynobj/`. New function = `agsdefns.sh` under the current `SCRIPT_API_v*` +
  `Engine/ac/<type>.cpp` implementation + `Sc_*` wrapper + `Register<Type>API()` + plugin export
  + manual page (`ags-manual-source`) + `Changes.txt` line.
- `Engine/ac/` = game logic, `Engine/script/` = VM, `Common/` = shared with `AGS.Native` and
  the tools: no engine globals in `Common/`.
- Style: match the surrounding file; new code follows wiki "AGS Coding Conventions (Cpp)".
- Commits: `Engine:` `Editor:` `Script API:` `Compiler:` `Tool:` `Plugin:` `CMake` `ci:`;
  72-char title; one program and one concern per commit; every commit builds.
- Tests: `ctest` (common, engine, compiler, tools), NUnit (Editor), auto-test game from
  `ags-test-games` (engine run with `--gfxdriver Software --no-message-box`, TAP output).
  Engine coverage is mostly that game.

## Before reporting done

Build what you touched (CMake; `Tests.sln` too when `Common/` changed); `ctest`; auto-test game
for engine or compiler changes; load a pre-change game and save for format changes; state what
you did not verify.
