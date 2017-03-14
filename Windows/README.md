# Adventure Game Studio - Windows

## Build Requirements

* In common:
  * Microsoft Visual Studio 2008 SP1 or higher - currently the only supported IDE for making Engine and Editor, but in theory you may try other tools, using MSVS project for the reference.
* To work with Engine code and Editor's full solution (see elaboration in related section):
  * Allegro 4.4.2 library *patched sources*: clone [our own Allegro repository](https://github.com/adventuregamestudio/lib-allegro.git) and checkout allegro-4.4.2-agspatch branch which already has necessary patch applied.
    * **OR**, alternatively, you may get original sources and patch yourself (look for instructions below): clone [official git repository](https://github.com/liballeg/allegro5)) and checkout 4.4.2 tag, ([download from sourceforge.net](https://sourceforge.net/projects/alleg/files/allegro/4.4.2/allegro-4.4.2.zip/download), or [download from gna.org](http://download.gna.org/allegro/allegro/4.4.2/allegro-4.4.2.zip).
  * Alfont 1.9.1 library *patched sources*: clone [our own Alfont repository](https://github.com/adventuregamestudio/lib-alfont) and checkout alfont-1.9.1-agspatch branch which already has necessary patch applied.
    * **OR**, alternatively, you may get original sources and patch yourself: ([checkout or download from SVN repository](https://sourceforge.net/p/alfont/code/HEAD/tree/trunk/)).
* Specifically for the Engine:
  * DirectX SDK August 2007 ([Download](https://www.microsoft.com/en-us/download/details.aspx?id=13287))
  * OpenGL API and Extension Header Files ([Download](https://www.opengl.org/registry/#headers))
  * libogg-1.1.3 or higher ([Download](https://www.xiph.org/downloads/))
  * libtheora-1.0 or higher ([Download](https://www.xiph.org/downloads/))
  * libvorbis-1.2.0 or higher ([Download](https://www.xiph.org/downloads/))
* To make manual and/or Windows installer:
  * Python 2.7 with PyWin32 extension ([Download](http://www.activestate.com/activepython/downloads))
  * InnoSetup 5.5 ([Download](http://www.jrsoftware.org/isdl.php))

## Installing SDKs

You need to have DirectX SDK. "August 2007" version is recommended, that seems to be the last version that still has required libraries and headers compatible with software renderer in AGS.
OpenGL API and Extension headers should be placed wherever Visual Studio can see them (or setup include paths to them in MSVS).

## Patching libraries

Historically AGS was built with slightly modified libraries. To keep backwards compatibility at current point you need to patch two libraries which source you download by the links above.

Diff files can be found in "Windows\patches" directory of AGS repository.

* Allegro 4.4.2 source should be patched with **allegro-4.4.2.patch** file.
* Alfont 1.9.1 source should be patched with **alfont-1.9.1.patch** file.

If you got these libraries from our own repositories, you do not need to patch them yourself, just checkout allegro-4.4.2-agspatch and alfont-1.9.1-agspatch branches respectively and build from them.


## Building the libraries

All of the mentioned libraries should either have MSVC project(s) in their sources, CMake configuration, or at least a Makefile which you could use to create MSVC solution.

It is important to make sure static libraries have two build configuration set up: one with runtime C library linked dynamically (/MD compilation flag) and another with runtime library linked statically (/MT compilation flag). These options are be found at the compiler's "Code Generation" property page in the MSVC project settings.
/MD option is usually default one, so you may need to create second one by hand.

The reason for having both configurations is explained in the latter section below.

### DirectX

DirectX is linked dynamically. You should not be building DirectX libraries, but using the reference libs and header files from DirectX SDK.

### Allegro

Allegro 4.4 source provides CMake script for generating MSVC project files. We do not cover CMake tool here, please refer to official documentation: https://cmake.org/documentation/ .

When configuring CMake, you may uncheck all Allegro add-ons and examples, because AGS does not need them.
Also make sure to uncheck SHARED option, for AGS is linking Allegro statically.

If you are using patched sources from our Allegro fork, the MSVC_SHARED_CRT option will also be present. You need that option checked when building library with /MD flag for the Editor, and unchecked when building library with /MT option for the Engine (also see explanation in related section below). If you are using official Allegro source, then you'll have to modify generated projects by hand to setup this flag properly.

Static library built with /MD is expected to be named alleg-static.lib, and one with /MT named alleg-static-mt.lib.

### Alfont

Alfont 1.9.1 sources already come with MSVC projects. The ones in our own fork are already set up to have distinct configuration with /MD and /MT compilation flags. You need to build only static library project for AGS.

Static library built with /MD is expected to be named alfont-md.lib, and one with /MT named alfont-mt.lib.

### OGG, Theora and Vorbis

All of these come with MSVC projects. You may need to make sure there are distinct build configurations with /MD and /MT compilation flags, but other than that just build static libraries, and you are all set.


## Building AGS

### For the Engine:

You need to build *static* libraries, compiled with **/MT** or **/MTd** option (*statically* linked runtime C library). Latter is important or you may get linking issues.

Build following libraries:
* Allegro 4.4.2 (patched)
* Alfont 1.9.1 (patched)
* libogg
* libtheora
* libvorbis and libvorbisfile

Engine MSVS solution is Solutions\Engine.sln.
Depending on the version of MSVS you are using you need to setup paths to compiled libraries and their headers either in IDE options (older) or project property pages (newer). Then build the solution.

### For the Editor:

Editor has two related solutions: Solutions\AGS.Editor.Full.sln and Solutions\AGS.Editor.NoNative.sln. The latter is useful if your version of MSVS cannot build mixed (C#/C++) assemblies, or if you specifically do not want to work with and/or compile C++ code.

If you are working with full solution, you need to first build *static* libraries, compiled with **/MD** or **/MDd** option (*dynamically* linked runtime C library). Latter is important or you may get linking issues.

Build following libraries:
* Allegro 4.4.2 (patched)
* Alfont 1.9.1 (patched)

Depending on the version of MSVS you are using you need to setup paths to compiled libraries and their headers either in IDE options (older) or project property pages (newer). Then build the solution.

If you are working with AGS.Editor.NoNative solution, then you do not have to make any of C++ libraries at all, but you will have to get compatible compiled AGS.Native.dll somewhere. For starters we suggest taking one from the latest release of AGS.
