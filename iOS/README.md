# Using the engine

## Installation

A jailbroken iDevice is required!

Cydia packages for the engine, midi patches and a test game can be obtained
by adding

    http://jjs.at/cydia/

as a repository to Cydia.


## Adding a game

Place the game data under

    /var/mobile/ags/game/

The main game file (the "exe") has to be renamed to "ac2game.dat".

You can change engine settings by placing a file "ios.cfg" into the
"ags" folder. A sample configuration file can be found in <SOURCE>/misc/.


## Controls

-   Finger movement: Moving the mouse cursor
-   Single finger tap: Perform a left click
-   Tap with two fingers: Perform a right click
-   Longclick: Hold down the left mouse button until tapping the screen again
-   Holding down one finger for over two seconds: Open/Close the onscreen keyboard


# Building the engine

The iOS app consists of two parts:

-   iOS app and engine
-   3rd party libraries


## iOS app

There is an Xcode project in the <SOURCE>/iOS/xcode directory. It requires a
minimum of the iOS SDK 8.0. The project should build on OS X Mavericks (version
10.9) or above with Xcode 6.0.1 or above.

To build from command line, change to <SOURCE>/iOS/xcode/ags and run

    $ xcodebuild clean build CODE_SIGN_IDENTITY="" CODE_SIGNING_REQUIRED=NO


## Native 3rd party libraries

You don't have to build these yourself unless you want to change the source code of
a library.

Change to the <SOURCE>/iOS/buildlibs directory and run

    ./buildall.sh
    ./makefatlibs.sh

This will patch, build and properly install the required libraries for
armv7, armv7s, arm64, i386 and x86_64 architectures.



## Links

Cydia repository: http://jjs.at/cydia/

iOS thread on the AGS forum: http://www.adventuregamestudio.co.uk/yabb/index.php?topic=46040.0
