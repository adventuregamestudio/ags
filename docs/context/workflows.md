# Workflows

Commands are CI's (`.github/workflows/build.yml`), checked 2026-11-02 on `ags4`; `master` same
unless noted.

## Build

CMake (engine, compiler, tools, tests; all platforms; deps fetched unless `-DAGS_USE_LOCAL_<LIB>=ON`;
Linux needs the SDL2 dev packages from the `apt-get` step in `build.yml` or `debian/README.md`):

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DAGS_BUILD_TOOLS=ON -DAGS_TESTS=ON
cmake --build build --config Release --parallel 8
# tools only, no SDL: -DAGS_BUILD_ENGINE=OFF -DAGS_BUILD_TOOLS=ON
```

MSBuild (Windows; the only way to build the Editor; v142 toolset on VS 2022; x64 exists for
engine and tools; Editor needs NuGet restore + irrKlang DLLs in `Editor/References/`
(`Windows/README.md`); output `Solutions/.build/<platform>/<config>/`):

```bat
msbuild Solutions\Engine.sln          /m /p:PlatformToolset=v142 /p:Configuration=Release /p:Platform=x86
msbuild Solutions\Tools.sln           /m /p:PlatformToolset=v142 /p:Configuration=Release /p:Platform=x86
msbuild Solutions\Compiler.App.sln    /m /p:PlatformToolset=v142 /p:Configuration=Release /p:Platform=x86
msbuild Solutions\Tests.sln           /m /p:PlatformToolset=v142 /p:Configuration=Release /p:Platform=x86
msbuild Solutions\AGS.Editor.Full.sln /m /p:PlatformToolset=v142 /p:Configuration=Release /p:Platform=x86
```

Makefiles (Linux/macOS, no CMake): `make --directory=Engine`, `make --directory=Compiler`,
`make --directory=Tools/<tool>`; source lists are hand-written: a file added only to CMake breaks
them silently in CI.

Other: `Emscripten/README.md` (emsdk 3.1.74 in CI), `Android/README.md` (Gradle, NDK),
`OSX/README.md`, `iOS/README.md` (Xcode; no CI).

## Test

```sh
cd build && ctest -C Release --output-on-failure     # common_test engine_test compiler_test tools_test
Solutions\packages\NUnit.ConsoleRunner.3.*\tools\nunit3-console.exe Solutions\.test\x86\Release\AGS.Editor.Tests.dll
```

Auto-test game (the only end-to-end engine test). Source: `adventuregamestudio/ags-test-games`,
`ags4/auto-test` and `ags3/auto-test`, `auto-test-3.6.1`, `auto-test-3.6.2`; scripts
`run-tests.asc`, `test-*.asc`, `tap.asc` (TAP), `gfx-compare.asc` (render compare). CI downloads
that repo's latest release zip (`ags4-auto-test.zip` / `ags3-auto-test.zip`) and runs:

```sh
./ags --gfxdriver Software --no-message-box --log-stdout=script:info,main:debug,game:warn --user-data-dir . ags4-auto-test.ags
mv agstest.tap agstest_software.tap && tappy agstest_software.tap     # pip install tap.py; non-zero on any "not ok"
./ags --gfxdriver OGL ...                                              # D3D9 on Windows
```

The game is compiled in that repo's CI with the released Editor (`atbx`), so a script API change is
covered only after the test game is updated and re-released; engine-only changes are covered at
once. New feature → add `test-<feature>.asc/.ash` there and register it in `run-tests.asc`.

Engine options for testing: `--no-message-box`, `--log-stdout=<group>:<level>,...`,
`--user-data-dir`, `--startr <room>`, `--gfxdriver Software|OGL|D3D9|D3D11`, `--setup`
(`OPTIONS.md`; `--help`).

## Verify before a PR

1. every commit builds the program it touches; for `Common/` build CMake **and** `Tests.sln`
   (GCC/libstdc++ rejects missing includes MSVC accepts)
2. `ctest` green; NUnit green if `Editor/` or `AGS.Types` changed
3. auto-test game on a Software build when engine, `Common/` or compiler changed
4. format or save changes: load a game and a save produced before the change
5. tools or `agfreader` changes: build the auto-test project with `agsbuild`, compare with the
   Editor's `game28.dta` (the Editor is the reference until the tools replace it)
6. Makefile source lists updated

## Documentation duties

- script API change ⇒ manual page: `adventuregamestudio/ags-manual-source` (`master` 3.x,
  `ags4` 4.0; one `.md` per type, e.g. `Character.md`; `StandardEnums.md`, `Globalfunctions_*.md`,
  `UpgradeTo40.md`); build = `ags-manual` (pandoc + Lua filters → CHM for F1 help, HTML for site)
- `Changes.txt`: one line per user-visible change under the version's section and heading
  (Common / Editor / Compiler / Scripting / Script API / Engine / ports); internals not listed
- `OPTIONS.md` for config keys and switches; `CMAKE.md` for build options; platform READMEs
- release: `version.json` + `Script/push_version.py`, `release-X.X.X` branches (wiki "Public release: procedure")

## Submit

- branch from `master` (3.x; `release-3.6.3` for RC fixes) or `ags4` (4.0); fixes and small
  improvements straight to a PR; features and larger changes: issue first (CONTRIBUTING.md
  "Issues"; `.github/pull_request_template.md`: Summary / Related issues / Testing / Notes / AI tooling)
- commit titles `Engine:` `Editor:` `Script API:` `Compiler:` `Tool:` `Plugin:` `CMake` `Makefile`
  `ci:` or library name; 72 chars; one concern, one program per commit
- no `Co-Authored-By`/tool trailers; AI use disclosed in the PR description (CONTRIBUTING.md "Use of
  AI Tooling")
- Artistic License 2.0; no GPL/LGPL code copied in; linking a separate library is fine (#2990)
