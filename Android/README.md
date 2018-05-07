# Using the engine


## Adding games to the game list

By default games have to be placed into the external storage device. This is
usually the SD-card, but this can vary.

Place the game into the directory

    <EXTERN>/ags/<GAME NAME>/

<GAME NAME> is what will be displayed in the game list.



## Game options

Global options can be configured by pressing the MENU button on the game list
and choosing the "Preferences" item. These settings apply to all games unless
they have their own custom preferences set.

By performing a longclick on a game entry in the list, a menu opens that lets
you choose custom preferences specifically for that game.

In the same menu you can also choose "Continue" to resume the game from
your last savegame.



## Controls

-   Finger movement: Moving the mouse cursor
-   Single finger tap: Perform a left click
-   Tap with two fingers: Perform a right click
-   Longclick: Hold down the left mouse button until tapping the screen again
-   MENU button: Opens a menu for key input and quitting the game
-   MENU button longpress: Opens and closes the onscreen keyboard
-   BACK button: Sends ESC key command to the game
-   BACK button longpress: "Quit game" dialog




## Downloading prebuilt engine packages

Unsigned APKs suitable for running any compatible AGS game may be found linked in the
corresponding release posts on [AGS forum board](http://www.adventuregamestudio.co.uk/forums/index.php?board=28.0).



## MIDI playback

For midi music playback, you have to download GUS patches. We recommend
"Richard Sanders's GUS patches" from this address:

http://alleg.sourceforge.net/digmid.html

A direct link is here:

http://www.eglebbk.dds.nl/program/download/digmid.dat

This 'digmid.dat' is, in fact, a **bzip2** archive, containing actual data file,
which should be about 25 MB large. Extract that file, rename it to **patches.dat**
and place it into the  **ags** directory alongside your games.



# Building the engine

The Android app consists of three parts, each with different requirements:

-   Java app: needs the Android SDK for Windows, Linux or Mac. Requires built native engine library.
-   Native engine library: needs the Android NDK for Windows, Linux or Mac. Requires built native 3rd party libraries.
-   Native 3rd party libraries: needs the Android NDK for Linux.

For example, if you have prebuilt native engine library, you may follow instructions for "Java app" straight away.
If you are building from scratch, then first follow "Native 3rd party libraries", then "Native engine library", and "Java app" only then.


## Native 3rd party libraries

These are backend and utility libraries necessary for the AGS engine.
Building them currently is only possible on Linux.

Before building you need to have Android NDK installed with prepared standalone toolchains
for all Android platforms. See https://developer.android.com/ndk/guides/standalone_toolchain.html
for instructions on creating the standalone toolchains.

Make sure your Linux system has 'curl' utility installed, it is required for the download script.

Set NDK_HOME variable, pointing to the location of Android NDK at your system.
Change to the <SOURCE>/Android/buildlibs directory and run,
e.g. assuming the ndk is installed in '/opt/android-ndk-r16b':

    $ export NDK_HOME=/opt/android-ndk-r16b
    $ cd <SOURCE>/Android/buildlibs
    $ ./buildall.sh

This will download, patch, build and properly install the required libraries.

Library sources must be available in <SOURCE>/libsrc. Compiled libraries will be put to <SOURCE>/Android/nativelibs.


## Native engine library

This is the main AGS engine code. Before building it you must have native 3rd party libraries ready.

**IMPORTANT:** Android port integrates number of plugins as a part of the engine. Some of the plugin sources
may be included as submodules, so make sure to initialize submodules before compiling it, e.g. from the
root <SOURCE> directory:

    $ git submodule update --init --recursive

It must be compiled using the Android NDK. This can
simply be done by running ndk-build within the <SOURCE>Android/library directory.

e.g. (assuming the ndk is installed in /opt)

    $ export PATH=$PATH:/opt/android-ndk-r16b
    $ cd <SOURCE>/Android/library
    $ ndk-build

The native code is built for all current Android architecture, that is armv6, armv7-a,
x86 and mips.


## Java app

There are two parts to the Java app, one is the engine library in <SOURCE>/Android/library
and the other one is the launcher app. The default launcher which displays a list
of games from the SD-card is in <SOURCE>/Android/launcher_list.

The easiest way to build the app is to create an Android project in Eclipse. Choose the
"create from existing source" option and point Eclipse to the launcher directory.

To build from the command line, you can use the tool 'ant'.

e.g. (assuming the SDK is installed in /opt)

    $ export ANDROID_HOME=/opt/android-sdk-linux
    $ cd <SOURCE>/Android/launcher_list
    $ ant debug
    $ ant release # for release build



## Links

Android SDK: http://developer.android.com/sdk/

Android NDK: http://developer.android.com/ndk/

Android thread on the AGS forum: http://www.adventuregamestudio.co.uk/yabb/index.php?topic=44768.0
