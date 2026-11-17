# Repository map

Checked on `ags4` `a83208d33` and `master` `10994b4d6` (2026-11-02). Paths repo-relative.

## Branches

| Branch | Is | `version.json` |
|---|---|---|
| `master` | AGS 3.x; runs every game since 2.50; fixes, performance, backend, tooling only; format and script-API changes "kept to a strict minimum" | 3.6.4.0 dev |
| `release-3.6.3` | current 3.x release line (RC); patches here, merged to `master` | 3.6.3 |
| `ags4` | AGS 4.0 Beta; breaking changes; imports projects saved by 3.6.0+; pre-4.0 compiled games not guaranteed | 4.00.00.33 |

`master` is merged into `ags4` by the maintainers (33 `master` commits not yet in `ags4` on
2026-11-02). Plan since 2020: `ags4` becomes `master`, 3.x continues as `ags3`; date undecided.

## Map

```
Common/        C++11 library shared by Engine, Editor (AGS.Native) and Tools
  ac/            data structs: gamesetupstruct.h, game_version.h, gamestructdefines.h (kScriptAPI_*)
  game/          main_game_file.cpp (game28.dta), room_file.cpp, room_version.h
  gfx/ font/ gui/ script/ util/ platform/   Bitmap over trimmed Allegro 4; fonts; GUI data+draw; cc_script.h; Stream/String/Path
  test/          common_test (22 files); libsrc/: aastr, googletest (submodule)
Engine/        C++11 runtime: Windows, Linux, macOS, Android, iOS, Web (Emscripten), FreeBSD; PSP dead
  main/          main.cpp, engine.cpp (startup), game_run.cpp (loop), update.cpp, config.cpp
  ac/            233 files: game logic + script API, one file per type; dynobj/ (87): managed script objects
  script/        exports.cpp (setup_script_exports), scriptexecutor.cpp (VM), script_api.h (Sc_* macros)
  game/          game_init.cpp, savegame*.cpp (kSvgVersion_* in savegame.h)
  gfx/           ali3dsw.cpp (software), ali3dogl.cpp; D3D9/D3D11 in platform/windows/gfx/
  media/ platform/ plugin/   audio (OpenAL via mojoAL), video; per-OS; agsplugin.h (IAGSEngine), plugin_stubs.cpp
  test/          engine_test: 3 files only
Compiler/      agscc. compiler.cpp, main.cpp; preproc/ (C++ preprocessor; the Editor has a C# one)
               ags4: script/ = old compiler, script2/ = new (default for new projects); test/, test2/
               master: script/ only
Tools/         agf2dta agfexport agf2dlgasc agsbuild agspak crmpak spriteimport spritepak trac
               data/: agfreader (Game.agf), data_file_writer (game28.dta), scriptgen, tra_utils; test/: tools_test
Editor/        C# 6, .NET Framework 4.7, WinForms, Windows only (runs under Wine)
  AGS.Types/     project model, Game.agf (de)serialization; Enums/ScriptAPIVersion.cs
  AGS.Editor/    Components/ (one per tree node), Panes/, BuildTargets/, Tasks.cs, DataFileWriter.cs,
                 UpgradeGameManager.cs, Debugger/, Resources/agsdefns.sh (THE script API header)
  AGS.Native/    C++/CLI over Common: room/sprite I/O, font+GUI previews, compile (agsnative.cpp)
  AGS.CScript.Compiler/  C# preprocessor used by the Editor build; unfinished C# compiler
  AGS.Controls/ scintilla/ AGS.Editor.Tests/ (NUnit, 26 files)
Plugins/       first-party engine plugins kept for old games (AGSSpriteFont, snowrain, flashlight, agslua, ...)
libsrc/        allegro (trimmed 4.x), freetype, glm, miniz, mojoAL, sdl_ttf, stb, tinyxml2
Solutions/     MSBuild: Engine.sln Tests.sln Tools.sln Compiler.App.sln Compiler.Lib.sln Compiler2.Lib.sln (ags4)
               AGS.Editor.Full.sln AGS.Editor.NoNative.sln
CMake/ ci/ Script/      CMake modules; CI Dockerfiles; push_version.py, build_packages.py
Android/ iOS/ OSX/ Emscripten/ debian/ Windows/   platform builds + READMEs
```

"ac" = Adventure Creator (1997 name). `Engine/ac/` is flat; new engine code goes there unless it
is a clear subsystem.

## Build systems (all three maintained by hand)

| System | Builds | Used by |
|---|---|---|
| CMake | Engine, Common, Compiler, Tools, tests. Options: `AGS_BUILD_ENGINE` ON, `AGS_BUILD_TOOLS` OFF, `AGS_BUILD_COMPILER` (=tools), `AGS_TESTS` OFF, `AGS_BUILD_LEGACY_COMPILER`, `AGS_USE_LOCAL_*`, `AGS_NO_VIDEO_PLAYER`, `AGS_BUILTIN_PLUGINS`; deps fetched by `CMake/Fetch*.cmake` | `build.yml` matrix: Windows MSVC, MinGW x86_64/arm64, Ubuntu x86_64/arm64, macOS; Emscripten, Android, Linux packaging |
| MSBuild `Solutions/` | everything; **only** way to build the Editor | `windows-msvc-*` jobs, Inno Setup packaging |
| Makefiles `Engine/`, `Compiler/`, `Tools/*/`, plugins | engine, compiler, tools on Linux/macOS | `makefile-build.yml`, `linux-make-tools`, `macos-tools-builds`, `debian/`; source lists hand-written; a broken tool Makefile does not fail CI (`find ... -execdir make`) |
| Gradle `Android/`, Xcode `iOS/xcode` `OSX/xcode` | bundles | Android CI; iOS has no CI |

CI = GitHub Actions since 2026 (`.disabled.cirrus.yml` is the old Cirrus config).

## Tests

| Suite | Target | Run |
|---|---|---|
| GoogleTest | `common_test` `engine_test` `compiler_test` `tools_test` (CMake `-DAGS_TESTS=ON`; MSBuild `Tests.sln`) | `ctest` in the build dir |
| NUnit | `Editor/AGS.Editor.Tests` | `nunit3-console` after `AGS.Editor.Full.sln` |
| auto-test game | `ags-test-games`: `ags4/auto-test`, `ags3/auto-test*`; scripts emit TAP | engine `--gfxdriver Software --no-message-box ...`, then `tappy agstest.tap` (workflows.md) |

Engine coverage ≈ the auto-test game; `engine_test` is 3 files.

## Game data and compatibility: must not break

| Thing | Where | Rule |
|---|---|---|
| game data `game28.dta` (in `game.ags`) | read `Common/game/main_game_file.cpp`; written by Editor `DataFileWriter.cs` **and** `Tools/data/data_file_writer.cpp` (keep equal) | versions `Common/ac/game_version.h`: `ags4` reads `kGameVersion_360_21`+, `master` 2.50+; new data = **extension block** appended after main data (1-byte or 16-char id + 8-byte length), never inserted (wiki "Extending game and room data format", #828, PR #1183) |
| rooms `room<n>.crm` | `Common/game/room_file.cpp`, `room_version.h` | same block scheme; `ags4` reads `kRoomVersion_3508`+; versions not monotonic across branches; `ags4` Editor keeps rooms as sources `Rooms/<n>/` (`data.xml`, PNGs, `room<n>.asc`) and compiles them |
| saves | `Engine/game/savegame.h` (`kSvgVersion_*`), `savegame_components.cpp` | full state dump; readable back to `kSvgVersion_Components`; `ags4` reads 3.5.0+ saves; component content change = new version |
| script API | decl `Editor/AGS.Editor/Resources/agsdefns.sh`; impl `Engine/ac/<type>.cpp` with `Register<Type>API()` (40 files) collected by `setup_script_exports()` in `Engine/script/exports.cpp` | compiled scripts link by **name** (`"Character::Walk^4"`, `^N` = arg count, `^10N` variadic) and read fields by **offset** (switches in `Engine/ac/dynobj/`); new API under `#ifdef SCRIPT_API_vXXX`, deprecations under `SCRIPT_COMPAT_vXXX`, versions in `kScriptAPI_*` + `ScriptAPIVersion.cs`; each function also exported to plugins via `ccAddExternalFunctionForPlugin` (wiki "A quick guide to Script API") |
| plugin API | `Engine/plugin/agsplugin.h` | `IAGSEngine` vtable order and `AGS_Engine*` exports frozen (binary plugins nobody can rebuild) |
| engine config / command line | `OPTIONS.md`, `Engine/main/config.cpp`, `main.cpp` | keys live in players' `acsetup.cfg` and launchers |
| project files `Game.agf`, `Rooms/`, `Dialogs/*.dasc` (ags4) | `Editor/AGS.Types`; `UpgradeGameManager.cs` upgrades on load | tools (`Tools/data/agfreader.cpp`) read the current format: teach them every new property |

Serialize field by field through `Stream` (`Common/util/stream.h`); `AlignedStream` exists only to
read old raw-memory formats (wiki "Engine data structure restrictions").

## Where a change goes

- **script function/property:** `agsdefns.sh` (current `SCRIPT_API_v*` guard) → `Type_Member()` in `Engine/ac/<type>.cpp` → `Sc_*` wrapper via `API_*` macro → `Register<Type>API()` → manual page in `ags-manual-source` → `Changes.txt` "Script API"
- **project property:** `AGS.Types` class + serialization → property grid if needed → `DataFileWriter.cs` if the engine needs it → `Tools/data/agfreader.cpp` and `data_file_writer.cpp` → engine reader (+ extension block/version if format changes)
- **config option:** `Engine/main/config.cpp` → `OPTIONS.md` → Editor Default Setup pane if the Editor writes it
- **tool or tool option:** `Tools/<tool>/` → `Tools/CMakeLists.txt` → the tool's `Makefile` → `Solutions/Tools.App` → `agsbuild` if part of the build
- **dependency:** link external; vendor only if unavailable on Linux, tiny, or pinned/modified (wiki "Adding third-party libraries to the project")
