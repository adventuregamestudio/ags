#Adventure Game Studio

Licensed under the Artistic License 2.0, see License.txt.


##Engine instructions

To get started building the AGS engine, see the platform specific instructions or forum threads:

-    [Linux](debian/README.md) ([Forum thread](http://www.adventuregamestudio.co.uk/forums/index.php?topic=46152.0))
-    [Windows](Windows/README.md) ([Forum thread](http://www.adventuregamestudio.co.uk/forums/index.php?topic=46847.0)) ([wiki](http://www.adventuregamestudio.co.uk/wiki/Compiling_AGS))
-    [Android](Android/README.md) ([Forum thread](http://www.adventuregamestudio.co.uk/forums/index.php?topic=44768.0))
-    [iOS](iOS/README.md) ([Forum thread](http://www.adventuregamestudio.co.uk/forums/index.php?topic=46040.0))
-    [PSP](PSP/README.md) ([Forum thread](http://www.adventuregamestudio.co.uk/forums/index.php?topic=43998.0))
-    [Mac OS X](OSX/README.md) ([Forum thread](http://www.adventuregamestudio.co.uk/forums/index.php?topic=47264.0))

##Issue tracker

Please report bugs and feature requests at the [AGS Issue Tracker](http://www.adventuregamestudio.co.uk/forums/index.php?action=projects)!

## Contributing

We are happy about every contributor, so if you want to help out, you can get commit access to the [adventuregamestudio/ags](https://github.com/adventuregamestudio/ags) repository.

There is also always the possibility of pushing changes to a personal fork and then creating a pull request.
This is also a good idea if you have commit access, but feel that changes you made should be reviewed before inclusion.

The [master](https://github.com/adventuregamestudio/ags/tree/master) branch should be kept in a working state and always compilable on all targeted platforms.
Larger changes that potentially break things temporarily should first be made in other branches or in personal forks.

##AGS game compatibility:

This runtime engine port is not compatible with all AGS games. There are the
following restrictions:

-   Supported AGS versions right now are all starting from 2.50 even though
    running games below version 3.2x is experimental.
    You can check the version of AGS a game was made with by examining the properties
    of the game executable.
    If you try to run a game made with a newer or older version of AGS, you will
    receive an error when trying to launch the game.
-   Savegames are compatible between the different platforms if they are created
    with the same engine version.
-   Games that depend on plugins for which there is no platform-independent
    replacement will not load.

##Changes from Chris Jones' version of AGS

This version of AGS contains changes from the version published by Chris Jones.
The run-time engine was ported to Android, iOS, Linux, Mac OS X and PSP and a refactoring effort is under way.
A detailed documentation of the changes is provided in the form of the git log of this git repository
(https://github.com/adventuregamestudio/ags).

##Credits:

-   Adventure Game Studio by Chris Jones (http://www.adventuregamestudio.co.uk/)
-   Linux port by berolinux (http://gitorious.org/ags)
-   Additional code by Bernhard Rosenkraenzer and Christian Morales Vega
-   PSP Eboot artwork by Paul Wilkinson (subspark)
-   AGSBlend plugin by Steven Poulton (see License_AGSBlend.txt)
-   PSP, Android, iOS ports and continued development by JJS
