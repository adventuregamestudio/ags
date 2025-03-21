<p align=center>
  <img src="https://avatars.githubusercontent.com/u/1833326" width=96></br>
  <a target="_blank" href="https://cirrus-ci.com/github/adventuregamestudio/ags" title="Build Status"><img src="https://api.cirrus-ci.com/github/adventuregamestudio/ags.svg"></a>
</p>
  
# Adventure Game Studio

Adventure Game Studio (AGS) - is the IDE and the engine meant for creating and running videogames of adventure (aka "quest") genre. Created by Chris Jones back in 1999, AGS was open-sourced in 2011. Since then continues to be developed by contributors. Both Editor and Engine are licensed under the Artistic License 2.0; for more details see [License.txt](License.txt). 

For community forums, games and more, go to the website: [www.adventuregamestudio.co.uk](https://www.adventuregamestudio.co.uk)

The latest stable version of AGS Editor may be found here: [https://www.adventuregamestudio.co.uk/site/ags/](https://www.adventuregamestudio.co.uk/site/ags/)

For the full list of releases, both stable, betas and others, see our repository's [Releases page](https://github.com/adventuregamestudio/ags/releases).

Please be aware that, unlike the runtime engine, AGS Editor is only supported on MS Windows (and Windows emulators such as WINE).


## Branches and releases

The [`master`][master-br] branch is where the next planned version is being developed. It may temporarily contain unstable or untested code.

Currently, `master` corresponds to 3.\* generation of the engine/IDE and maintains backward compatibility with previous releases - see also [Compatibility](#ags-game-compatibility). According to current plans, this branch should only receive improvements to the backend, system support, and performance. Changes to data formats and game script should be kept to a strict minimum necessary to fill in the critical gaps in the engine's functionality.

There's an [`ags4`][ags4-br] branch also active where we develop a future version AGS 4.0. There we introduce greater changes and cut much of the old version support.

According to our plans, in the future `master` branch will be merged with `ags4`, while the backward compatible generation will remain as the `ags3` branch and only receive fixes and minor enhancements. But there's still some work to do in AGS 3.\*, so the exact moment that happens is unknown.

For "official" releases we create `release-X.X.X` branches, that is to prepare the code for the final release and continue making patches to that release if a need arises. 

Because of the low number of active developers we tend to only update the one latest release branch. If bugs are found in one of the older versions, then we advise you to update to the latest version first.

Please note that while the `master` branch may contain changes to game data format and new script functions, we cannot guarantee that these will remain unchanged until the actual release. We only support data formats and script APIs that are in published releases. For that reason, it's best to use one of the actual releases if you'd like to make your own game with this tool.

There may be other temporary development branches meant for preparing and testing large changes, but these are situational.

## Building and running

To get started building the AGS engine, see the platform-specific instructions or forum threads:

- [CMAKE](CMAKE.md) - this readme covers the centralized build system using CMake
- [Android](Android/README.md) ([Forum thread](https://www.adventuregamestudio.co.uk/forums/index.php?topic=59751.0))
- [iOS](iOS/README.md) ([Forum thread](https://www.adventuregamestudio.co.uk/forums/index.php?topic=46040.0))
- [Linux](debian/README.md) ([Forum thread](https://www.adventuregamestudio.co.uk/forums/index.php?topic=59750.0))
- [Mac OS X](OSX/README.md) ([Forum thread](https://www.adventuregamestudio.co.uk/forums/index.php?topic=47264.0))
- [Windows](Windows/README.md)
- [Emscripten Web port](Emscripten/README.md) ([Forum thread](https://www.adventuregamestudio.co.uk/forums/index.php?topic=59164.0))

No longer actively supported, but may work with older code revisions:
-    [PSP](PSP/README.md) ([Forum thread](https://www.adventuregamestudio.co.uk/forums/index.php?topic=43998.0))

On desktop systems launching a game is done by either placing an engine executable in the same directory where the game data is and starting the engine up or passing game location as a command-line argument.

For Android, we have a game launcher app that may run any game, but it's also possible to make a signed APK for your own game (see instructions linked above).

Game configuration is usually found in the `acsetup.cfg` file. On Windows you may also invoke a setup dialog by running the engine with `--setup` argument.

For the list of available config options and command-line arguments, please refer to [OPTIONS.md](OPTIONS.md).

AGS Editor is currently only supported on Windows, although it may be run using Wine on Linux and OSX.


## AGS game compatibility

This repository now holds two generations of AGS, referred to as "AGS 3" and "AGS 4".

**The 3rd generation** of AGS may be currently found in the `master` branch (also `ags3`). Its specifics are:

- Supports (imports into the editor and runs by the engine) all versions of AGS games made with AGS 2.50 and up. Note that there may still be compatibility issues with very old games that were not uncovered and fixed yet.

- If you try to run an unsupported game, you will receive an error message reporting the original version of AGS it was made in and data format index, which may be used for reference e.g. when reporting the problem.

- Game saves are compatible between the different platforms if they are created with the same version of the engine. Latest 3.\* engine should normally read saves made by engine 3.2.0 and above, but that has not been tested for a while.

- Certain games may require engine plugins. If there's no plugin version for a particular platform or a platform-independent replacement, then the game will not load on that platform.

- As of AGS 3.6.0 the Windows engine no longer supports playing **AVI/MPG videos**. The decision to drop this feature was made while moving to a new SDL2 backend, primarily because its implementation was based on using DirectShow interface (Windows-only) and relied on codecs installed on player's system. Other ports were not supporting this ever. If the game tries to play such video, it will simply be skipped. The existing workaround is to convert the avi/mpg video file to OGV, while keeping the full original filename (*including the extension!*): then this file will be instead opened by the integrated OGV player.

**The 4th generation** is found in [`ags4`][ags4-br] branch. It formally supports importing games made with AGS 3.4.1 and above, but only if these do not use any legacy features. The 4.x engine should be able to read saves made with the 3.5.0 engine and above.


## Other repositories

There are other repositories which contain additional resources and may be of interest to you:

- [ags-manual](https://github.com/adventuregamestudio/ags-manual) - holds AGS documentation.

- [ags-templates](https://github.com/adventuregamestudio/ags-templates) - this is where compiled game templates are uploaded in preparation for the Editor's release.

- [ags-template-source](https://github.com/adventuregamestudio/ags-template-source) - this is where template sources are developed - these are essentially AGS game projects.


## Contributing

If you would like to contribute to the project, please read the [CONTRIBUTING.md](CONTRIBUTING.md). Below is a brief summary of the process.

* For the bug fixes or minor code improvements, you need only create a fork of the repository, commit your changes to a branch, and submit a pull request. We will review your commits and may ask you to make alterations to your code if needed before merging it into our repository.

* For new features or other larger contributions, please first open an issue in the [Issue Tracker](https://github.com/adventuregamestudio/ags/issues). There we can discuss the proposed changes with the development team to ensure that they will be consistent with existing program behavior and future development plans.

We may also accept patch files if you send them to one of the project maintainers.

## Changes from Chris Jones' version of AGS

This version of AGS contains changes from the version published by Chris Jones. The run-time engine was ported to Android, iOS, Linux, Mac OS X, and PSP and a refactoring effort is underway. Detailed documentation of the changes is provided in the form of the git log of this git repository ([github.com/adventuregamestudio/ags](https://github.com/adventuregamestudio/ags)).


## Credits

[Link](Copyright.txt)

[master-br]: https://github.com/adventuregamestudio/ags/tree/master
[ags4-br]: https://github.com/adventuregamestudio/ags/tree/ags4

