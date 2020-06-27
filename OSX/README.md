# AGS for OSX

Uses a modified Allegro 4 library which sets up an offscreen buffer that the game renders to. And then blits that to texture memory in the OpenGL graphics hardware and finally renders that texture to screen.

Supports Mac OS X 10.7+ (tested through 10.11).

This port was initially done by Edward Rudd for providing Gemini Rue in a Humble Bundle.


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


## Adding resources for the game

All game resources are taken from `OSX/Resources` . Please feel free to replace the stub files that
exist there with your own game files, as long as they have the same names.

The default accepted name for the game file to be placed there is `game.ags`, which is generated in the `Compiled/Data/`
directory of your AGS game project. This name is defined in `Engine/platform/osx/alplmac.mm` file.


## Building your game with CMake and Xcode

At the ags project root, use CMake to generate the needed Xcode project.
```
mkdir buildXcode
cd buildXcode
cmake -G Xcode ..
```

Using the flags `-DCMAKE_BUILD_TYPE=Release` or `-DCMAKE_BUILD_TYPE=Debug` to specify the build type don't affect the 
generated Xcode project. The project will build for debug by default, and to change to release, in Xcode you can go to 
`Product` -> `Scheme` -> `Edit Scheme...` to change the build type from Debug to Release.

The produced binary will be in the `ags/buildXcode/Debug/` directory or `ags/buildXcode/Release/`, depending of the 
target used. By default the name is `AGS.app`.


## Building your game with CMake and make

You can also use CMake and make to build ags. 

```
export BUILD_TYPE=Release
mkdir build_$BUILD_TYPE
cd build_$BUILD_TYPE
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE ..
make
```

This will build the executable `AGS.app` in `the build_Release/` directory. The `AGS.app` has at it's root the directory `Contents`, and inside of it the following:

- `Info.plist` , the file that holds the `AGS.app` details and points to `AGS` as the target executable.

- `MacOS/`, contains the `AGS` engine binary used to run your game.

- `Resources/`, the directory where your game and it's resources, including the icon are placed.




