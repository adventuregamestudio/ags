# AGS for OSX

Uses a modified Allegro 4 library which sets up an offscreen buffer that the game renders to. And then blits that to texture memory in the OpenGL graphics hardware and finally renders that texture to screen.

Supports Mac OS X 10.7+ (tested through 10.15).

This port was initially done by Edward Rudd for providing Gemini Rue in a Humble Bundle.

## Prerequisites

You need a current XCode and the XCode command line tools.

Install them by running

    xcode-select --install

## Compiling

Install and compile the used libraries by running

    cd OSX/buildlibs
    make
    cd -

Afterwards compile the engine by running

    make --directory Engine

## Packaging

Copy the template app `OSX/AGS.app` to a path and name it like your game. Copy your game data files (ac2game.dat, acsetup.cfg, audio.vox, speech.vox) to <your game>.app/Contents/Resources. 

Finally, copy the engine binary into the app container:

    cp Engine/ags <your game>.app/Contents/MacOS/AGS

## Creating Icon Sets

The unprocessed icons are available here:

    $ ls OSX/ags.iconset/
    icon_128x128.png	icon_16x16.png		icon_256x256.png	icon_32x32.png		icon_512x512.png
    icon_128x128@2x.png	icon_16x16@2x.png	icon_256x256@2x.png	icon_32x32@2x.png	icon_512x512@2x.png

To generate the icon set needed by app bundles, Run, in the project workspace root:

    iconutil -c icns -o OSX/Resources/ags.icns OSX/ags.iconset

This will create a file called `ags.incs` placed in the `Resources/` directory.
When you build within the cmake build directory, it should pick these files up.

TIP: Use `touch AGS.app` to force macos to reload the application icon.

