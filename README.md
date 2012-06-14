#Adventure Game Studio

Licensed under the Artistic License 2.0, see License.txt in the Engine folder.


##Engine instructions

To get started with the AGS engine, see the platform specific instructions or forum threads:

-    Android ([Forum thread](http://www.adventuregamestudio.co.uk/yabb/index.php?topic=44768.0))
-    iOS ([Forum thread](http://www.adventuregamestudio.co.uk/yabb/index.php?topic=46040.0))
-    [Linux](https://github.com/adventuregamestudio/ags/blob/main/debian/README.md) ([Forum thread](http://www.adventuregamestudio.co.uk/yabb/index.php?topic=46152.0))
-    Mac OS X
-    [PSP](https://github.com/adventuregamestudio/ags/blob/main/PSP/README.md) ([Forum thread](http://www.adventuregamestudio.co.uk/yabb/index.php?topic=43998.0))
-    Windows


##Bug tracker

The bug tracker is at [jjs.at/tracker](http://jjs.at/tracker). Please report any bugs you find!


##AGS game compatibility:

This runtime engine port is not compatible with all AGS games. There are the
following restrictions:

-   Supported AGS versions right now are all starting from 2.50 even though
    running games below version 3.2x is experimental at the moment.
    You can check the version of AGS a game was made with by examining the properties 
    of the game executable.
    If you try to run a game made with a newer or older version of AGS, you will
    receive an error when trying to launch the game.
-   Generally savegames are not compatible between the PC and PSP version of
    the engine. FIXME: What about other platforms?

###Glitches affecting games made with AGS version prior to 3.2x:

-   Objects on screen might overlap in the wrong order.


##Future plans:

-   Running 3.x and 2.x AGS games without glitches. When this will happen I cannot say as 
    it depends on if and when these engine versions become open source.
-   Ingame menu to change settings/controls.
-   Graphical menus.
-   Completely fixing the sound stuttering on slow devices.


##Credits:

-   Adventure Game Studio by Chris Jones (http://www.bigbluecup.com/)
-   Linux port by berolinux (http://gitorious.org/ags)
-   Additional code by Bernhard Rosenkraenzer and Christian Morales Vega
-   Eboot artwork by Paul Wilkinson (subspark)
-   AGSBlend plugin by Steven Poulton (see License_AGSBlend.txt)
-   PSP, Android, iOS ports and continued development by JJS

-   Thanks to thebudds for testing.
