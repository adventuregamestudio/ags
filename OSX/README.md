# AGS for OSX

Uses a modified Allegro 4 library which sets up an offscreen buffer that the game renders to. And then blits that to texture memory in the OpenGL graphics hardware and finally renders that texture to screen.

Supports Mac OS X 10.7+ (tested through 10.11).  

This port was initially done by Edward Rudd for providing Gemini Rue in a Humble Bundle.

## Building

The recommended build system is CMake. Please refer to CMAKE.md in the project root directory for details.

In summary, to build, you will need to start in the project root directory.

With Xcode:

    cmake -G Xcode -S . -B build
    cd build
    open AGS.xcodeproj

NOTE: normally you can create the build directory outside of the source directory, but XCode has issues with debugging if you do that.


With Makefiles:

    cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug -S . -B build
    cd build
    ccmake . # optional: use if you wish to adjust build options
    make -j $(sysctl -n hw.ncpu)  # adjust for number of cpus

NOTE: you don't need to specify `-G "Unix Makefiles"` typically as it is the default.
NOTE2: build type options include Debug, Release, RelWithDebugInfo


Cmake only needs to be run once. Afterwards you can use `make` or `Xcode` and only call `ccmake .` if options need changing.


## Building (with provided XCode project)

There is also an XCode project which may also be helpful. However you will need to build the dependencies manually:

    cd OSX/buildlibs
    make install
    open ../OSX/xcode/ags.xcworkspace

