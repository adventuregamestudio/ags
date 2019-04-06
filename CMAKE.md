# CMake

CMake is a build system generator. You describe your components along with their requirements and it will generate a build system for your project.


## Downloading

The latest version of CMake is recommended and available as source and pre-built binaries here:

https://cmake.org/download/


## Example Building on macOS

CMake can create Xcode projects, which allow multi-configs (like Debug, Release, RelWithDebInfo)

 - Install XCode and ensure command line tools are installed.
 - Install CMake 3.14 (via homebrew or from the above site)
 - make -C OSX/buildlibs install 
 - mkdir build
 - cd build
 - cmake -GXcode ..
 - cmake --build . --config Debug


## Example Building on Linux

CMake can create Makefile and Ninja projects. These are single config systems so they need to be manually specified.

 - Install recommended dev packages
 - Install CMake 3.14 (via above site)
 - mkdir build-debug
 - cmake .. -DCMAKE_BUILD_TYPE=Debug ..
 - cmake --build . --parallel


## Example Building on Windows

CMake can create Visual Studio projects. Different versions of Visual Studio are supported. It also supports multi-config.

 - Install Visual Studio (2015, 2017, 2019 should work)
 - Install DirectX SDK (August 2017)
 - Install CMake (via above site)
 - Build ogg, vorbis, theora, allegro using /MD. Use directory names of libogg, libvorbis, libtheora
 - Copy include files and libs in Windows/Include and Solutions/.lib (allegro and ogg may have an extra include file in the build dir)
 - cmake -G "Visual Studio 16 2019" -A Win32 ..
 - cmake --build . --parallel --config Debug
