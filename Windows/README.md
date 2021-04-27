# Adventure Game Studio - Windows

## Build Requirements

* In common:
  * Microsoft Visual Studio 2015 or higher - currently the only supported IDE for making Engine and Editor. The free Community edition is sufficient and is available for download at the Microsoft's site:
    * https://visualstudio.microsoft.com/downloads/
    * https://visualstudio.microsoft.com/vs/older-downloads/
  * If you are using MSVS 2019 and higher you might need to manually download [Windows 8.1 SDK](https://go.microsoft.com/fwlink/p/?LinkId=323507) from the [SDK Archive](https://developer.microsoft.com/en-us/windows/downloads/sdk-archive/).
* To work with Engine code and Editor's full solution (see elaboration in related section):
  * Allegro 4.4.3 library's *patched sources*: clone [our own Allegro repository](https://github.com/adventuregamestudio/lib-allegro.git) and checkout allegro-4.4.3.1-agspatch branch which already has necessary patch applied and MSVC projects created.
* Specifically for the Engine:
  * DirectX SDK August 2007 ([Download](https://www.microsoft.com/en-us/download/details.aspx?id=13287))
  * libogg-1.1.3 or higher ([Download](https://www.xiph.org/downloads/))
  * libtheora-1.0 or higher ([Download](https://www.xiph.org/downloads/))
  * libvorbis-1.2.0 or higher ([Download](https://www.xiph.org/downloads/))
* Specifically for the Editor:
  * irrKlang 1.6 (32-bit) assembly pack for .NET 4.5 ([Download](https://www.ambiera.com/irrklang/downloads.html)).
* To build Windows installer:
  * Inno Setup 6.0.2 or higher ([Download](http://www.jrsoftware.org/isdl.php))
  * (optional) PowerShell ([Download](https://aka.ms/powershell-release?tag=stable))


**NOTE:** You may skip building libraries from the source completely by using prebuilt libs from the following archive:
  * https://www.dropbox.com/s/3vdq7qw01tdtfux/ags-prebuilt-libs-3.5.x.zip?dl=0

You still have to download library sources though, because you'd need header files.
If you go this way skip **"Building the libraries"** sections altogether.


## Installing SDKs

You need to have DirectX SDK. ["August 2007" version](https://www.microsoft.com/en-us/download/details.aspx?id=13287) is recommended, that seems to be the last version that still has required libraries and headers compatible with DirectDraw renderer in Allegro 4.

## Building the libraries

If you want to build these libraries yourself, all of them should either have MSVC project(s) in their sources, CMake configuration, or at least a Makefile which you could use to create MSVC solution.

It is important to make sure static libraries have four build configuration set up: a pair with runtime C library linked dynamically (/MD and /MDd compilation flags) and another pair with runtime library linked statically (/MT and /MTd compilation flags). These options are be found at the compiler's "Code Generation" property page in the MSVC project settings.
/MD option is usually default one, so you may need to create second one by hand.

The reason for having both configurations is explained in the latter section below.

### Allegro

Our [patched Allegro library branch](https://github.com/adventuregamestudio/lib-allegro/tree/allegro-4.4.3.1-agspatch) has ready MSVC solution: you will find it in "build/VS2015" subdirectory.

If you want to go all the way on your own, following is a brief information on creating one.

Allegro 4.4.* source provides CMake script for generating MSVC project files. We do not cover CMake tool here, please refer to official documentation: https://cmake.org/documentation/ .

Before using CMake you need to create two enviroment variables (for your OS, not Visual Studio) called "INCLUDE" and "LIB", unless these already exist. Add path to DirectX SDK header files into "INCLUDE" variable and path to DirectX *.lib files into "LIB" variable.

When configuring CMake, you may uncheck all Allegro add-ons and examples, because AGS does not need them.
Also make sure to uncheck SHARED option, for AGS is linking Allegro statically.

In our Allegro 4 fork the MSVC_SHARED_CRT option will also be present. You need that option checked when building library with /MD flag for the Editor, and unchecked when building library with /MT option for the Engine (also see explanation in related section below). If you are using e.g. official Allegro source, then you'll have to modify generated projects by hand to setup this flag properly.

Static library built with /MD is expected to be named alleg-static.lib, and one with /MT named alleg-static-mt.lib. Debug versions are to be named alleg-debug-static.lib with /MDd flag, and alleg-debug-static-mt.lib for /MTd.

### OGG, Theora and Vorbis

All of these come with MSVC projects. You may need to make sure there are distinct build configurations with /MT(d) compilation flags (to link with the Engine only), but other than that just build static libraries, and you are all set.


## Building AGS Engine

Engine requires following libraries:
* Allegro 4.4.3.1 (patched with our changes)
* libogg
* libtheora
* libvorbis and libvorbisfile

You may download the prebuilt libraries [here](https://www.dropbox.com/s/3vdq7qw01tdtfux/ags-prebuilt-libs-3.5.x.zip?dl=0), although you'd still have to get library sources from their homepages because you need their headers for the engine compilation.

If you prefer to build everything yourself: you need to build *static* libraries compiled with **/MT** or **/MTd** option (*statically* linked runtime C library). This is important or you may get linking errors.

Download [DirectX SDK 2007](https://www.microsoft.com/en-us/download/details.aspx?id=13287).

Engine MSVS solution is Solutions\Engine.sln. It contains two projects, the "Engine" project is the one creating the executable.

In order to direct Studio to necessary libraries and their headers setup following enviroment variables in your system by [creating user macros in the Property Pages](https://docs.microsoft.com/en-us/cpp/build/working-with-project-properties?view=msvc-160#user-defined-macros):
 
* AGS_ALLEGRO_INCLUDE - pointing to the location of allegro 4 headers;
* AGS_ALLEGRO_LIB - pointing to the location of allegro 4 library files;
* AGS_DIRECTX_LIB - pointing to the location of DirectX SDK libraries;
* AGS_LIBOGG_LIB - pointing to the location of libogg library files;
* AGS_LIBVORBIS_LIB - pointing to the location of libvorbis library files;
* AGS_LIBTHEORA_LIB - pointing to the location of libtheora library files;

### Known problems

When running the engine from MSVS and breaking execution or doing step-by-step execution you may encounter a significant keyboard and mouse input lag. This is somehow caused by Allegro 4 library.<br>
One known solution to this problem is to adjust the system registry entry named "LowLevelHooksTimeout", found by the full path "HKEY_CURRENT_USER\Control Panel\Desktop\LowLevelHooksTimeout". This setting tells how long to wait for the input device response, in milliseconds. Setting it to a rather low value (e.g. 10) may improve the situation. Note that you must re-login into your Windows profile (or restart the system) for this to take effect.


## Building AGS Editor

Editor has two related solutions: Solutions\AGS.Editor.Full.sln and Solutions\AGS.Editor.NoNative.sln. The latter is useful if your version of MSVS cannot build mixed (C#/C++) assemblies, or if you specifically do not want to work with and/or compile C++ code.
Within the solution there are several library projects, and "AGSEditor" project is the one creating the executable.

Editor demands several third-party .NET libraries, most of them will be downloaded automatically as NuGet packages, but you will have to manually get [irrKlang 1.6 (32-bit) for .NET 4.5](https://www.ambiera.com/irrklang/downloads.html) and put irrKlang.NET4.dll and ikpMP3.dll into Editor/References subdirectory.

The "Full" solution addtionally requires Allegro 4.4.3.1 library. As noted before, you may download the prebuilt libraries [here](https://www.dropbox.com/s/3vdq7qw01tdtfux/ags-prebuilt-libs-3.5.x.zip?dl=0), although for Allegro 4 you will need to get headers too (you may acquire these by [checking out our repository](https://github.com/adventuregamestudio/lib-allegro/tree/allegro-4.4.3.1-agspatch)).

If you are building Allegro 4 yourself, then for the Editor you need to build *static* libraries compiled with **/MD** or **/MDd** option (*dynamically* linked runtime C library). This is important or you may get linking errors.

In order to direct Studio to necessary libraries and their headers setup following enviroment variables in your system by [creating user macros in the Property Pages](https://docs.microsoft.com/en-us/cpp/build/working-with-project-properties?view=msvc-160#user-defined-macros):
 
* AGS_ALLEGRO_INCLUDE - pointing to the location of allegro 4 headers;
* AGS_ALLEGRO_LIB - pointing to the location of allegro 4 library files;

If you are working with AGS.Editor.NoNative solution then you do not have to make any of C++ libraries at all, but you will have to get compatible compiled AGS.Native.dll somewhere. For starters we suggest taking one from the latest release of AGS.

### Known problems

AGS Editor currently cannot create a new game without a template, even if it's just a blank project. Game templates are not embedded into the Editor itself, but have to be placed along with Editor's exe, in Templates subdirectory. You may get latest game templates from [our repository](https://github.com/adventuregamestudio/ags-templates), or copy them from any existing public release.


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
