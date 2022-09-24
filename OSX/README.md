# AGS for macOS (OSX)

This port was initially done by Edward Rudd for providing Gemini Rue in a Humble Bundle when the AGS backend was Allegro 4. Currently, it uses SDL2.

## Creating Icon Sets

The unprocessed icons are available here:

    $ ls OSX/ags.iconset/
    icon_128x128.png	icon_16x16.png		icon_256x256.png	icon_32x32.png		icon_512x512.png
    icon_128x128@2x.png	icon_16x16@2x.png	icon_256x256@2x.png	icon_32x32@2x.png	icon_512x512@2x.png

To generate the icon set needed by app bundles, Run, in the project workspace root:

    iconutil -c icns -o OSX/Resources/ags.icns OSX/ags.iconset

This will create a file called `ags.incs` placed in the `Resources/` directory.
When you build within the CMake build directory, it should pick these files up.

TIP: Use `touch AGS.app` to force macOS to reload the application icon.


## Adding resources for the game

After you build your game with the Editor, your own game `.ags` and additional files will be generated in the `Compiled/Data/` of your game project directory.

In this port, all game resources are taken from `OSX/Resources`, remove the stub files that exist there and replace them with your game files taken from `Compiled/Data/`.


## Building your game with CMake and Xcode

At the AGS project root, use CMake to generate the needed Xcode project.
```
mkdir buildXcode
cd buildXcode
cmake -G Xcode ..
```

Using the flags `-DCMAKE_BUILD_TYPE=Release` or `-DCMAKE_BUILD_TYPE=Debug` to specify the build type doesn't affect the generated Xcode project. 
The project will build for debugging by default, and to change to release, in Xcode you can go to `Product` -> `Scheme` -> `Edit Scheme...` to change the build type from Debug to Release.

The produced binary will be in the `ags/buildXcode/Debug/` directory or `ags/buildXcode/Release/`, depending on the used target. 
By default, the name is `AGS.app`.


## Building your game with CMake and make

You can also use CMake and make to build AGS. 

```
export BUILD_TYPE=Release
mkdir build_$BUILD_TYPE
cd build_$BUILD_TYPE
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE ..
make
```

This will build the executable `AGS.app` in `the build_Release/` directory. The `AGS.app` has at its root the directory `Contents`, and inside of it the following:

- `Info.plist` , the file that holds the `AGS.app` details and points to `AGS` as the target executable.

- `MacOS/`, contains the `AGS` engine binary used to run your game.

- `Resources/`, the directory where your game and its resources, including the icon, are placed.
