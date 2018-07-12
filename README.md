# Adventure Game Studio

Adventure Game Studio (AGS) - is the IDE and the engine meant for creating and running videogames of adventure (aka "quest") genre. It has potential, although limited, support for other genres as well.

Originally created by Chris Jones back in 1999, AGS was opensourced in 2011 and since continued to be developed by contributors.

An official homepage of AGS is: http://www.adventuregamestudio.co.uk

Both Editor and Engine are licensed under the Artistic License 2.0; for more details see [License.txt](License.txt).


## Engine instructions

To get started building the AGS engine, see the platform specific instructions or forum threads:

-    [Linux](debian/README.md) ([Forum thread](http://www.adventuregamestudio.co.uk/forums/index.php?topic=46152.0))
-    [Windows](Windows/README.md)
-    [Android](Android/README.md) ([Forum thread](http://www.adventuregamestudio.co.uk/forums/index.php?topic=44768.0))
-    [iOS](iOS/README.md) ([Forum thread](http://www.adventuregamestudio.co.uk/forums/index.php?topic=46040.0))
-    [PSP](PSP/README.md) ([Forum thread](http://www.adventuregamestudio.co.uk/forums/index.php?topic=43998.0))
-    [Mac OS X](OSX/README.md) ([Forum thread](http://www.adventuregamestudio.co.uk/forums/index.php?topic=47264.0))

When running the engine on you may configure it by modifying acsetup.cfg or using several command line arguments.
On Windows you may also invoke a setup dialog by running executable with "--setup" argument, or executing winsetup.exe, if it is present.
For the list of available config options and command line arguments, please refer to [OPTIONS.md](OPTIONS.md).


## Issue tracker

Please report bugs and feature requests at the [AGS Issue Tracker](http://www.adventuregamestudio.co.uk/forums/index.php?action=projects)!


## Contributing

We welcome any contributor who wishes to help the project.

The usual workflow is this: you fork our repository (unless you already did that), create a feauture/fix branch, commit your changes to that branch, and then create a pull request. We will review your commits, and sometimes may ask to change something before merging into ours.

For bug fixing and general code improvements that may be enough, however, for significant changes, especially completely new features, it is advised to first open an issue in the tracker and discuss it with development team, to make sure it does not break anything, nor contradict to existing program behavior or concepts.

The [master](https://github.com/adventuregamestudio/ags/tree/master) branch should be kept in a working state and always compilable on all targeted platforms.
Larger changes that potentially break things temporarily should first be made in other branches or in personal forks.

We have a coding convention, please check it before writing the code: http://www.adventuregamestudio.co.uk/wiki/AGS_Engine_Coding_Conventions


## AGS game compatibility:

This runtime engine port is not compatible with all AGS games. There are the
following restrictions:

-   Supported AGS versions right now are all starting from 2.50; games between 2.5 and
    3.2 are supported in theory, but may have yet unknown compatibility issues.
    You can check the version of AGS a game was made with by examining the properties
    of the game executable.
    If you try to run a game made with a newer or older version of AGS, you will
    receive an error when trying to launch the game.
-   Savegames are compatible between the different platforms if they are created
    with the same engine version.
-   Games that depend on plugins for which there is no platform-independent
    replacement will not load.

	
## Changes from Chris Jones' version of AGS

This version of AGS contains changes from the version published by Chris Jones.
The run-time engine was ported to Android, iOS, Linux, Mac OS X and PSP and a refactoring effort is under way.
A detailed documentation of the changes is provided in the form of the git log of this git repository
(https://github.com/adventuregamestudio/ags).


## Credits

[Link](Credits.txt)
