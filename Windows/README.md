# Adventure Game Studio - Windows

## Build Requirements

* In common:
  * Microsoft Visual Studio 2008 SP1 or higher - currently the only supported IDE for making Engine and Editor, but in theory you may try other tools, using MSVS project for the reference.
* To work with Engine code and Editor's full solution (see elaboration in related section):
  * Allegro 4.2.2 library *sources*, which you need to patch (look for instructions below) ([Download from sourceforge.net](https://sourceforge.net/projects/alleg/files/allegro/4.2.2/all422.zip/download), [Download from gna.org](http://download.gna.org/allegro/allegro/4.2.2/all422.zip) or clone [official git repository](https://github.com/liballeg/allegro5)).
  * Alfont 1.9.1 library *sources*, which you also need to patch ([Checkout or download from SVN repository](https://sourceforge.net/p/alfont/code/HEAD/tree/trunk/)).
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

* Allegro 4.2.2 source should be patched with **allegro-4.2.2.patch** file.
* Alfont 1.9.1 source should be patched with **alfont-1.9.1.patch** file.

## Building

All of the mentioned libraries should normally have MSVS project(s) in their source packages, or at least makefiles which you could use.

### For the Engine:

You need to build *static* libraries, compiled with **/MT** or **/MTd** option (*statically* linked runtime C library). Latter is important or you may get linking issues.

Build following libraries:
* Allegro 4.2.2 (patched)
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
* Allegro 4.2.2 (patched)
* Alfont 1.9.1 (patched)

Depending on the version of MSVS you are using you need to setup paths to compiled libraries and their headers either in IDE options (older) or project property pages (newer). Then build the solution.

If you are working with AGS.Editor.NoNative solution, then you do not have to make any of C++ libraries at all, but you will have to get compatible compiled AGS.Native.dll somewhere. For starters we suggest taking one from the latest release of AGS.
