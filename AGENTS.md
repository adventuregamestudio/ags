# AGENTS.md

Adventure Game Studio (AGS): the IDE and runtime for making and playing adventure games.

This file states the project's rules for AI coding tools. [CONTRIBUTING.md](CONTRIBUTING.md) is the authoritative document and applies in full; this is a summary written to be read by a tool. If the two disagree, CONTRIBUTING.md wins.

## Do not

- **Do not open pull requests or issues on this repository.** Contributions are submitted by people, not by agents.
- **Do not post comments or review replies.** Discussion with maintainers is between people. Do not draft replies to reviewer questions on the contributor's behalf.
- **Do not commit or push** unless the person you are working with asks you to, each time.
- **Do not generate creative content of any kind.** No artwork, sprites, icons, fonts, sound, music, user interface design, user-facing text, or documentation prose. This content is human-authored in AGS without exception. If asked for it, decline and say why.
- **Do not make sweeping changes.** No cross-tree refactors, no reformatting of code you were not asked to touch, no architectural redesign. Large changes require a maintainer-agreed issue first; that agreement is not yours to assume.
- **Do not add tool attribution trailers to commits** (`Co-Authored-By`, `Generated-with`, and the like). The human contributor is the author of record. Disclosure happens in the pull request description instead.

## The repository

Four programs share one tree, over a shared C++ library in `Common/`:

- `Engine/` - C++11 runtime. Cross-platform: Windows, Linux, macOS, Android, iOS, Emscripten.
- `Editor/` - C# / .NET Framework WinForms IDE. **Windows only.**
- `Compiler/` - C++ AGS Script compiler. Also used inside the Editor.
- `Tools/` - C++ command line utilities.

Branches: `master` is the AGS 3.x line and stays backward compatible with games made in every version since AGS 2.50 - changes to data formats and to the script API are kept to a strict minimum there. `ags4` is the 4.0 line, where breaking changes are made.

Two build systems, and they are not equivalent. CMake builds the Engine, Compiler, Tools and the C++ tests on all platforms. The MSBuild solutions under `Solutions/` are the only way to build the Editor. Tests are GoogleTest via CTest, plus NUnit for the Editor.

## When writing code

- Match the style of the file you are editing. Much of this codebase predates the current [coding conventions](https://github.com/adventuregamestudio/ags/wiki/AGS-Coding-Conventions-(Cpp)); consistency with the surrounding code comes first.
- Write the minimum that does the job. No speculative edge-case handling, no defensive layers for conditions that cannot occur, no abstraction introduced for a single caller.
- Serialization is version-gated. To change a stored format, bump the constant in `Common/ac/game_version.h` or `Common/game/room_version.h` and read conditionally. Never alter an existing layout - old games and old saves must keep loading.
- Changes crossing the Editor's native boundary usually travel in threes: `Common/`, `Editor/AGS.Native` marshalling, and `Editor/AGS.Types`.
- A new script API function means registering the import in `Engine/script/`, implementing it in the matching `Engine/ac/`, and declaring it in `Editor/AGS.Editor/Resources/agsdefns.sh`.
- AGS is under the Artistic License 2.0. Do not reproduce source from other projects; GPL and LGPL code cannot be absorbed into this codebase.

## Commits

- Prefix the title with the program affected: `Engine:`, `Editor:`, `Compiler:`, `Script API:`, `Tool:`, `Plugin:`, `ci:`, `CMake:`. Keep it to 72 characters.
- One concern per commit. Separate bug fixes from refactors, and Editor changes from Engine changes.
- Every commit must leave the program it modifies building.

## The human is the contributor

Everything you produce is reviewed by the person you are working with before it goes anywhere, and that person is accountable for it. Work in a way that makes review possible: small steps, clear reasoning, and an honest account of what you did not verify. If you are unsure whether a change is wanted, ask rather than proceed.
