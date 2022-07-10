# CMake

CMake is a build system generator. You describe your components along with their requirements and it will generate a build system for your project.


## Using a CMake native IDE

Some IDEs can use a CMakeLists.txt file directly as project. JetBrains CLion and Visual Studio with "C++ CMake tools for Windows" component installed both support this. 

To interact with AGS source code like this, use the "Open a local folder" option in Visual Studio or open Project in CLion, 
and select the local directory where the ags repository was cloned.


## Downloading CMake to use through Command Line

The latest version of CMake is recommended and available as source and pre-built binaries here:

https://cmake.org/download/

Kitware now provide Ubuntu packges:

https://blog.kitware.com/ubuntu-cmake-repository-now-available/


## Example Building on macOS

CMake can create Xcode projects, which allow multi-configs (like Debug, Release, RelWithDebInfo)

Configuring:

 - Install XCode and ensure command line tools are installed.
 - Install CMake 3.14 (via homebrew or from the above site)
 - mkdir build
 - cd build
 - cmake -GXcode ..

Building:

There are multiple options from command line or IDE:

   - cmake --build . --config Debug --parallel
   - cmake --build . --config Release --parallel
   - use XCode to open AGS.xcodeproj


## Example Building on Linux

CMake can create Makefile and Ninja projects. These are single config systems so they need to be manually specified.

 - Install recommended dev packages
 - Install CMake 3.14 (via above site)
 - mkdir build-debug
 - cd build-debug  # otherwise your build artifacts will be in the source directory!
 - cmake .. -DCMAKE_BUILD_TYPE=Debug
 - cmake --build . --parallel


## Example Building on Windows

CMake can create Visual Studio projects. Different versions of Visual Studio are supported. It also supports multi-config.

Configuring:

 - Install Visual Studio (2015, 2017, 2019 should work)
 - Install CMake (via above site)
 - cmake -A Win32 ..

Building:

There are multiple options from command line or IDE:

 - cmake --build . --config Debug
 - cmake --build . --config Release
 - use Visual Studio to open AGS.sln


## Configuration

Cmake can be configured with `ccmake` or `cmake-gui` commands or provide options on the command line:

    cmake -DCMAKE_BUILD_TYPE=Debug

Standard CMake options are available, for `CMAKE_BUILD_TYPE`, you can set: Debug, Release, RelWithDebInfo and MinSizeRel.


### Using locally installed libraries

AGS can fetch its dependencies by default when building, and it should build like that in any platform like so. 
But sometimes, it may be desirable to use a locally installed dependency, either to speedup build times, because you 
are fixing a bug in such dependency or any other reason really.

As an example, if you are building on Windows and want to link to your local SDL2.dll, you can do so by using:

    cmake -DCMAKE_BUILD_TYPE=Debug -DSDL2_DIR="C:\\Lib\\SDL2" -DAGS_USE_LOCAL_SDL=1

The available flags are:

- `AGS_USE_LOCAL_SDL2` : Find SDL2 locally
- `AGS_USE_LOCAL_SDL2_SOUND` : Find SDL sound locally
- `AGS_USE_LOCAL_OGG` : Find OGG locally
- `AGS_USE_LOCAL_THEORA` : Find Theora locally
- `AGS_USE_LOCAL_VORBIS` : Find Vorbis locally
- `AGS_USE_LOCAL_ALL_LIBRARIES` : Force all the above to be local libraries

While default fetching scripts static link AGS to required libraries, when using local libraries AGS should dynamic link
when possible.


### AGS specific configuration options

The relevant options include

- `AGS_TESTS` : Build tests
- `AGS_BUILD_ENGINE` : Ensure the AGS Engine target is included, it's ON by default, but when working in other parts of 
  the code, like the tools, you may turn this off to speed up things in your IDE.
- `AGS_BUILD_TOOLS` : Ensure the Tools target is included, which contains the packing utility and others.  
  Turning this ON by default also builds the standalone compiler.
- `AGS_BUILD_COMPILER` : Build the standalone AGS Script Compiler.
- `AGS_NO_VIDEO_PLAYER` : optionally turns off the video player in the engine, useful sometimes when porting to new 
  lower performance platforms
- `AGS_BUILTIN_PLUGINS` : Build and include plugins in the engine.
- `AGS_DEBUG_MANAGED_OBJECTS` : Enables including Managed Objects information when logging. 
  These are very verbose and should not be used in final builds.
- `AGS_DEBUG_SPRITECACHE` : Enables including Sprite Cache  information when logging. 
  These are very verbose and should not be used in final builds.
