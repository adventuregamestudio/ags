# CMake

CMake is a build system generator. You describe your components along with their requirements and it will generate a build system for your project.


## Downloading

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
 - Install DirectX SDK (August 2007)
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

The relevant options include

 AGS_BUILD_STR - optionally append information to version string
 AGS_BUILTIN_PLUGINS - build and include plugins. Works on linux and macos.
 AGS_NO_MP3_PLAYER - disable mp3 playing for license reasons.
 CMAKE_BUILD_TYPE - Debug, Release, RelWithDebInfo and MinSizeRel

