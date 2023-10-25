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


## Building with CMake

Building with CMake is detailed in [`../CMAKE.md`](../CMAKE.md). Follow below for macOS specifics to create your own `.app` bundle with your own game.

### Building your game with CMake and Xcode

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


### Building your game with CMake and make

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


## Using the Xcode project

**NOTE:** Prefer the CMake generated project.

The Xcode project depends on the prebuilt SDL2 Framework and library sources.

- Download the [SDL2 Framework](https://github.com/libsdl-org/SDL/releases/download/release-2.24.1/SDL2-2.24.1.dmg) from the SDL2 releases, and extract and place it in `ags/libsrc` directory

The project also requires sources of library dependencies, either using the `libsrc/download.sh` script or manually downloading them and extracting them as below.

- Download [SDL_sound](https://github.com/icculus/SDL_sound/archive/1507be95c3605e4fd6a48ea4c527e4aa711a1566.zip) source code and place it in `ags/libsrc/SDL_sound`
- Download xiph [theora](https://github.com/xiph/theora/archive/7180717276af1ebc7da15c83162d6c5d6203aabf.tar.gz) source code, and place it in `ags/libsrc/theora`
- Download xiph [ogg](https://github.com/xiph/ogg/archive/refs/tags/v1.3.5.tar.gz) source code, and place in `ags/libsrc/ogg`
- Download xiph [vorbis](https://github.com/xiph/vorbis/archive/84c023699cdf023a32fa4ded32019f194afcdad0.tar.gz) source code, and place in `ags/libsrc/vorbis`

After this, load the `OSX/xcode/ags.xcworkspace` file in Xcode, it will enable building AGSKit, which is a self-contained framework like build of AGS Engine, and the `ags` app target, which you can customize with your own game, using the `Resources/` directory, in the same way the CMake build works.


## Using the Makefile

You should prefer using the CMake or Xcode project, as the Makefile build is not intended for porting your own game, but for running AGS locally. 

You will need to install the necessary xiph libraries (theora, ogg and vorbis):

    brew install theora
    brew install libogg
    brew install libvorbis
    
Additionally, you also need SDL2

    brew install sdl2
    
Make sure you have at least SDL 2.24.1 installed, if you have problems updating sdl2 with brew from a previous version, you can force it using `brew link --overwrite sdl2`.

The SDL_sound available in brew is not really well updated and it's a bit different from what's in the repository, using a different timidity and libmodplug. It's best to build it and install it from source. You will need CMake for this.

    SDL2_SOUND_VERSION=8d96d4cc0e1df35835a222ee51a7c32f273ec63e
    cd /tmp
    curl -fLsS "https://github.com/icculus/SDL_sound/archive/$SDL2_SOUND_VERSION.tar.gz" --output SDL_sound.tar.gz
    tar -xvzf SDL_sound.tar.gz
    mv SDL_sound-$SDL2_SOUND_VERSION SDL_sound
    cd /tmp/SDL_sound
    mkdir /tmp/SDL_sound/build
    cd /tmp/SDL_sound/build
    cmake -DSDL2_DIR=/usr/local/lib/cmake/SDL2 -DSDL_SOUND  -DSDLSOUND_DECODER_MIDI=1 ..  
    make && make install

Now you can go to the `Engine/` directory the cloned ags repository, and build it.

    make
