#Using the engine


##Adding games to the game list

By default games have to be placed into the external storage device. This is
usually the SD-card, but this can vary.

Place the game into the directory

    <EXTERN>/ags/<GAME NAME>/

<GAME NAME> is what will be displayed in the game list.



##Game options

Global options can be configured by pressing the MENU button on the game list
and choosing the "Preferences" item. These settings apply to all games unless
they have their own custom preferences set.

By performing a longclick on a game entry in the list, a menu opens that lets
you choose custom preferences specifically for that game.

In the same menu you can also choose "Continue" to resume the game from
your last savegame.



##Controls

-   Finger movement: Moving the mouse cursor
-   Single finger tap: Perform a left click
-   Tap with two fingers: Perform a right click
-   Longclick: Hold down the left mouse button until tapping the screen again
-   MENU button: Opens a menu for key input and quitting the game
-   MENU button longpress: Opens and closes the onscreen keyboard
-   BACK button: Sends ESC key command to the game
-   BACK button longpress: "Quit game" dialog




##Downloading prebuilt engine packages

Go to http://jjs.at/daily to download the installation package for the current
source snapshot.



#Building the engine

The Android app consists of three parts, each with different requirements:

-   Java app: only needs the Android SDK for Windows, Linux or Mac
-   Native engine library: needs the Android NDK for Windows, Linux or Mac
-   Native 3rd party libraries: needs the Android NDK for Linux


##Java app

There are two parts to the Java app, one is the engine library in <SOURCE>/Android/library
and the other one is the launcher app. The default launcher which displays a list
of games from the SD-card is in <SOURCE>/Android/launcher_list.

The easiest way to build the app is to create an Android project in Eclipse. Choose the
"create from existing source" option and point Eclipse to the launcher directory.



##Native engine library

You don't have to build this yourself unless you want to change the source code of the engine.

This is the main AGS engine code. It must be compiled using the Android NDK. This can
simply be done by running

    ndk-build

inside the <SOURCE>/Android/library directory.
The native code is built for all current Android architecture, that is armv6, armv7-a,
x86 and mips.


##Native 3rd party libraries

You don't have to build these yourself unless you want to change the source code of
a library.

Change to the <SOURCE>/Android/buildlibs directory and run

    ./buildall.sh

This will download, patch, build and properly install the required libraries.

The scripts require standalone toolchains for all Android platforms and they have to
be available on the PATH. See docs/STANDALONE-TOOLCHAIN.html in the NDK directory
for instructions on creating the standalone toolchains.


##Links

Android SDK: http://developer.android.com/sdk/

Android NDK: http://developer.android.com/tools/sdk/ndk/

Daily builds: http://jjs.at/daily/

Android thread on the AGS forum: http://www.adventuregamestudio.co.uk/yabb/index.php?topic=44768.0