#Using the engine

##Installation

A jailbroken iDevice is required!

Cydia packages for the engine, midi patches and a test game can be obtained
by adding

    http://jjs.at/cydia/

as a repository to Cydia.


##Adding a game

Place the game data under

    /var/mobile/ags/game/

The main game file (the "exe") has to be renamed to "ac2game.dat".

You can change engine settings by placing a file "ios.cfg" into the
"ags" folder. A sample configuration file can be found in <SOURCE>/misc/.


##Controls

-   Finger movement: Moving the mouse cursor
-   Single finger tap: Perform a left click
-   Tap with two fingers: Perform a right click
-   Longclick: Hold down the left mouse button until tapping the screen again
-   Holding down one finger for over two seconds: Open/Close the onscreen keyboard


#Building the engine

The iOS app consists of two parts:

-   iOS app and engine
-   3rd party libraries


##iOS app

There is an xcode 3.2.6 project in the <SOURCE>/iOS/xcode directory. It requires the
iOS SDK 4.x. The project should build on Snow Leopard or Lion with either xcode 3.x
or 4.x.


##Native 3rd party libraries

You don't have to build these yourself unless you want to change the source code of
a library.

Change to the <SOURCE>/iOS/buildlibs directory and run

    ./buildall.sh

This will download, patch, build and properly install the required libraries.



##Links

Cydia repository: http://jjs.at/cydia/

iOS thread on the AGS forum: http://www.adventuregamestudio.co.uk/yabb/index.php?topic=46040.0