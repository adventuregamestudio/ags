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
* To build Windows installer:
  * Inno Setup 6.0.2 or higher ([Download](http://www.jrsoftware.org/isdl.php))
  * (optional) PowerShell ([Download](https://aka.ms/powershell-release?tag=stable))


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


## Building AGS installer

There's an installer script Windows/Installer/ags.iss, you have to have [Inno Setup](http://www.jrsoftware.org/isdl.php) installed in order to build it.<br>
Installer script requires several macroses to be defined:

- AgsAppId - a GUID identifying installed software
- AgsFullVersion - a 4-digit version number
- AgsFriendlyVersion - a 3-digit 'user-friendly' version number
- AgsSpVersion - a special version tag (optional can be empty)

The manual way is to run compiler from command-line and pass these as arguments, for example:

`ISCC.exe Windows\Installer\ags.iss /DAgsAppId="baec604c-933c-426e-a11f-dec55953c4c3" /DAgsFullVersion="3.5.1.3" /DAgsFriendlyVersion="3.5.1" /DAgsSpVersion="Beta4"`

Alternatively, there's a PowerShell script that reads these values automatically from the file "version.json" (in the project's root). You run the script from the project's root like this:

`powershell Windows\Installer\build.ps1 -IsccPath 'C:\Program Files (x86)\Inno Setup 6\ISCC.exe'`

There are two things to note here:
* `IsccPath` argument should be followed by a path to Inno Setup program location, if it's different from the above example, then adjust the path accordingly.
* PowerShell may require administrative rights to run the script, in which case you have to first start up PowerShell as an administrator, and then issue above command in its console. In this case you also have to adjust the command by excluding `powershell`, like this:
    `Windows\Installer\build.ps1 -IsccPath 'C:\Program Files (x86)\Inno Setup 6\ISCC.exe'`

In any case, there is a number of files that have to be prepared for installer to actually build. These files have to be placed in Windows\Installer\Source and subdirectories:

- Redist\vc_redist.x86.exe - a [Visual C++ Redistributable for Visual Studio 2015](https://download.microsoft.com/download/6/A/A/6AA4EDFF-645B-48C5-81CC-ED5963AEAD48/vc_redist.x86.exe) (or C++ Redist corresponding to the MSVS you were building AGS with).
- Editor\
  - AGSEditor.exe
  - AGS.Controls.dll
  - AGS.CScript.Compiler.dll
  - AGS.Native.dll
  - AGS.Types.dll
  - ikpMP3.dll
  - irrKlang.NET4.dll
  - Magick.NET-Q8-x86.dll
  - Magick.NET-Q8-x86.Native.dll
  - Newtonsoft.Json.dll
  - WeifenLuo.WinFormsUI.Docking.dll
  - AGSEditor.exe.config
  - acsprset.spr
  - ags-help.chm
- Engine\
  - acwin.exe
- Linux\
  - full contents of prebuilt debian binaries (see [debian/README.md](debian/README.md#building-ags-for-a-game-release))
- Docs\ags-help.chm
- Templates\
  - at least one game template file (*.agt)

Note that if you do not want to build some of these components yourself, you may copy them from existing public release. Of course there won't be any guarantee that they match latest changes.

On success the resulting installer will be present in Windows\Installer\Output.
