# Adventure Game Studio - Windows

## Build Requirements

* In common:
  * Microsoft Visual Studio 2015 or higher - currently the only supported IDE for making Engine and Editor. The free Community edition is sufficient and is available for download at the Microsoft's site:
    * https://visualstudio.microsoft.com/downloads/
    * https://visualstudio.microsoft.com/vs/older-downloads/
  * In theory you may try other tools, using MSVS project for the reference.
* To work with Engine code and Editor's full solution (see elaboration in related section):
  * Allegro 4.4.3 library's *patched sources*: clone [our own Allegro repository](https://github.com/adventuregamestudio/lib-allegro.git) and checkout allegro-4.4.3.1-agspatch branch which already has necessary patch applied and MSVC projects created.
* Specifically for the Engine:
  * DirectX SDK August 2007 ([Download](https://www.microsoft.com/en-us/download/details.aspx?id=13287))
  * OpenGL API and Extension Header Files ([Download](https://www.opengl.org/registry/#headers))
  * libogg-1.1.3 or higher ([Download](https://www.xiph.org/downloads/))
  * libtheora-1.0 or higher ([Download](https://www.xiph.org/downloads/))
  * libvorbis-1.2.0 or higher ([Download](https://www.xiph.org/downloads/))
* Specifically for the Editor:
  * irrKlang 1.6 (32-bit) assembly pack for .NET 4.5 ([Download](https://www.ambiera.com/irrklang/downloads.html)).
* To run certain tools and build Windows installer:
  * Python 2.7 with PyWin32 extension ([Download](http://www.activestate.com/activepython/downloads))
  * InnoSetup 5.5 or higher ([Download](http://www.jrsoftware.org/isdl.php))


**NOTE:** You may skip building libraries from the source completely by using prebuilt libs from the following archive:
  * https://www.dropbox.com/s/4p6nw6waqwat6co/ags-prebuilt-libs.zip?dl=0

You still have to download library sources though, because you'd need header files.
If you go this way skip **"Building libraries"** sections altogether.


## Installing SDKs

You need to have DirectX SDK. "August 2007" version is recommended, that seems to be the last version that still has required libraries and headers compatible with DirectDraw renderer in Allegro 4.
OpenGL API and Extension headers should be placed wherever Visual Studio can see them (or setup include paths to them in MSVS).

## Building the libraries

All of the mentioned libraries should either have MSVC project(s) in their sources, CMake configuration, or at least a Makefile which you could use to create MSVC solution.

It is important to make sure static libraries have four build configuration set up: a pair with runtime C library linked dynamically (/MD and /MDd compilation flags) and another pair with runtime library linked statically (/MT and /MTd compilation flags). These options are be found at the compiler's "Code Generation" property page in the MSVC project settings.
/MD option is usually default one, so you may need to create second one by hand.

The reason for having both configurations is explained in the latter section below.

### DirectX

DirectX is linked dynamically. You should not be building DirectX libraries, but using the reference libs from DirectX SDK.

### Allegro

Our patched Allegro library branch has ready MSVC solution: you will find it in "build/VS2015" subdirectory.

If you want to go all the way on your own, following is a brief information on creating one.

Allegro 4.4.* source provides CMake script for generating MSVC project files. We do not cover CMake tool here, please refer to official documentation: https://cmake.org/documentation/ .

Before using CMake you need to create two enviroment variables (for your OS, not Visual Studio) called "INCLUDE" and "LIB", unless these already exist. Add path to DirectX SDK header files into "INCLUDE" variable and path to DirectX *.lib files into "LIB" variable.

When configuring CMake, you may uncheck all Allegro add-ons and examples, because AGS does not need them.
Also make sure to uncheck SHARED option, for AGS is linking Allegro statically.

If you are using patched sources from our Allegro fork, the MSVC_SHARED_CRT option will also be present. You need that option checked when building library with /MD flag for the Editor, and unchecked when building library with /MT option for the Engine (also see explanation in related section below). If you are using e.g. official Allegro source, then you'll have to modify generated projects by hand to setup this flag properly.

Static library built with /MD is expected to be named alleg-static.lib, and one with /MT named alleg-static-mt.lib. Debug versions are to be named alleg-debug-static.lib with /MDd flag, and alleg-debug-static-mt.lib for /MTd.

### OGG, Theora and Vorbis

All of these come with MSVC projects. You may need to make sure there are distinct build configurations with /MT(d) compilation flags (to link with the Engine only), but other than that just build static libraries, and you are all set.


## Building AGS

### For the Engine:

You need to build *static* libraries, compiled with **/MT** or **/MTd** option (*statically* linked runtime C library). Latter is important or you may get linking issues.

Build following libraries:
* Allegro 4.4.3.1 (patched with our changes)
* libogg
* libtheora
* libvorbis and libvorbisfile

Engine MSVS solution is Solutions\Engine.sln.
Depending on the version of MSVS you are using you need to setup paths to compiled libraries and their headers either in IDE options (older) or project property pages (newer). Then build the solution.

If you are building from MSVS 2015 or higher it is important to make sure that DirectX SDK headers path is mentioned **after** Windows Kits headers path in the "Include directories", otherwise you will get obscure compilation errors. It is possible to omit DirectX SDK headers path from the "Include directories" completely and let compiler use related headers from Windows Kits. You still need to link against DirectX SDK libs though.

### For the Editor:

Editor has two related solutions: Solutions\AGS.Editor.Full.sln and Solutions\AGS.Editor.NoNative.sln. The latter is useful if your version of MSVS cannot build mixed (C#/C++) assemblies, or if you specifically do not want to work with and/or compile C++ code.

If you are working with full solution, you need to first build *static* libraries, compiled with **/MD** or **/MDd** option (*dynamically* linked runtime C library). Latter is important or you may get linking issues.

Build following libraries:
* Allegro 4.4.3.1 (patched with our changes)

Depending on the version of MSVS you are using you need to setup paths to compiled libraries and their headers either in IDE options (older) or project property pages (newer). Then build the solution.

If you are working with AGS.Editor.NoNative solution, then you do not have to make any of C++ libraries at all, but you will have to get compatible compiled AGS.Native.dll somewhere. For starters we suggest taking one from the latest release of AGS.

In either case you also need to download [irrKlang assembly pack](https://www.ambiera.com/irrklang/downloads.html) and put irrKlang.NET4.dll and ikpMP3.dll into Editor/References subdirectory.
