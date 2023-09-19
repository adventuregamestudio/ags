# Adventure Game Studio - Windows

## Build Requirements

* In common:
  * Microsoft Visual Studio 2015 or higher - currently the only supported IDE for making Engine and Editor. The free Community edition is sufficient and is available for download at the Microsoft's site:
    * https://visualstudio.microsoft.com/downloads/
    * https://visualstudio.microsoft.com/vs/older-downloads/
  * If you are using MSVS 2019 and higher you might need to manually download [Windows 10 SDK (10.0.10240)](https://go.microsoft.com/fwlink/p/?LinkId=619296) from the [SDK Archive](https://developer.microsoft.com/en-us/windows/downloads/sdk-archive/).
* Specifically for the Engine:
  * SDL 2.0.12 or higher (https://github.com/libsdl-org/SDL/tree/SDL2)
  * SDL_Sound 2.0.* (https://github.com/icculus/sdl_sound)
  * libogg-1.1.3 or higher ([Download](https://www.xiph.org/downloads/))
  * libtheora-1.0 or higher ([Download](https://www.xiph.org/downloads/))
  * libvorbis-1.2.0 or higher ([Download](https://www.xiph.org/downloads/))
* Specifically for the Editor:
  * irrKlang 1.6 (32-bit) assembly pack for .NET 4.5 ([Download](https://www.ambiera.com/irrklang/downloads.html)).
* To build Windows installer:
  * Inno Setup 6.0.2 or higher ([Download](http://www.jrsoftware.org/isdl.php))
  * (optional) PowerShell ([Download](https://aka.ms/powershell-release?tag=stable))
**IMPORTANT:** all libraries should match the Engine's architecture: e.g. if you are building engine using 32-bit (x86) configuration then link libraries for 32-bit (x86) too.

**NOTE:** You may skip building libraries from the source completely by using prebuilt libs from the archive called "WinDevDependenciesVS.zip", which is attached to any [latest release of AGS](https://github.com/adventuregamestudio/ags/releases). If you go this way, then skip **"Building the libraries"** sections altogether.


## Building the libraries

If you prefer to build necessary libraries yourself, all of them should either have MSVC project(s) in their sources, CMake configuration, or at least a Makefile which you could use to create MSVC solution.

It is important to make sure that static libraries meant for the engine have build configurations with runtime C library linked statically (/MT and /MTd compilation flags). These options are found on the compiler's "Code Generation" property page in the MSVC project settings.
/MD option is usually default one, in which case you would have to create second configuration by copying existing one and modifying it.

The reason for this is that Engine project itself link C runtime statically.

### SDL2

For Windows you may download ready lib and DLL files directly [from SDL2 homepage](https://www.libsdl.org/download-2.0.php). You need the one under "Development Libraries", archive is called SDL2-devel-2.x.x-VC.zip (where x.x is a latest version number).

If for some reason you'd like to build it yourself, take the one under "Source Code", or get it from [SDL2's Mercurial repository](https://www.libsdl.org/hg.php). Then follow their instructions to build a dynamic library.

AGS engine will need following files to link:

* SDL2.lib
* SDL2main.lib

and SDL2.dll to run.

### SDL_Sound

Official page for SDL_Sound library is https://www.icculus.org/SDL_sound/, but downloads are hosted on github: https://github.com/icculus/SDL_sound/releases.
Any latest 2.0.X release should be good.

For the reference, at the time of writing our build server is using revision 495e948b45: https://github.com/icculus/SDL_sound/archive/495e948b455af48eb45f75cccc060498f1e0e8a2.tar.gz

After you downloaded the source this way or another, you should use CMake to generate MSVS solution from their provided CMakeList.txt.
Note that when doing this you may have to direct CMake to the SDL2's cmake config files. First go to the SDL2's sources location and find "cmake" directory inside. It should contain the file called "sdl2-config.cmake". If the file is not present, this means something is wrong with the SDL2's package, or maybe you've downloaded a way too old version of SDL2.
Once you checked that the file is present, configure CMake for SDL_Sound project and set SDL2_DIR variable, pointing to the directory which contains "sdl2-config.cmake". This may be done using CMake's GUI frontend, for example.
If you are running CMake from command line, you may also pass this path as an argument, for example:

    cmake . -DSDL2_DIR="absolute-path-to-my-libs\SDL2-2.24.1\cmake"

Once CMake finished working, you should have "SDL_sound.sln" MSVS solution in the same dir. Open the generated solution in MSVS, build a SDL2_sound-static project using wanted configuration.

### OGG, Theora and Vorbis

All of these come with MSVC projects. You may need to make sure there are distinct build configurations with /MT(d) compilation flags (to link with the Engine only), but other than that just build static libraries, and you are all set.


## Building AGS Engine

Engine requires following libraries:
* SDL2
* SDL_Sound
* libogg
* libtheora
* libvorbis

You may download the prebuilt libraries [here](https://github.com/adventuregamestudio/ags/releases/download/v.3.6.0.15/WinDevDependenciesVS.zip), although you'd still have to get library sources from their homepages because you need their headers for the engine compilation.

If you prefer to build everything yourself: you need to build *static* libraries compiled with **/MT** or **/MTd** option (*statically* linked runtime C library). This is important or you may get linking errors, as engine itself is compiled with **/MT(d)**.

Engine MSVS solution is Solutions\Engine.sln. It contains two projects, the "Engine" project is the one creating the executable.

In order to direct Studio to necessary libraries and their headers setup following enviroment variables in your system by [creating user macros in the Property Pages](https://docs.microsoft.com/en-us/cpp/build/working-with-project-properties?view=msvc-160#user-defined-macros):
 
* AGS_SDL_INCLUDE - pointing to the location of SDL2 headers;
* AGS_SDL_LIB - pointing to the location of SDL2 library files;
* AGS_SDL_SOUND_INCLUDE - pointing to the location of SDL Sound headers;
* AGS_SDL_SOUND_LIB - pointing to the location of SDL Sound library files;
* AGS_LIBOGG_LIB - pointing to the location of libogg library files;
* AGS_LIBTHEORA_LIB - pointing to the location of libtheora library files;
* AGS_LIBVORBIS_LIB - pointing to the location of libvorbis library files;


## Building AGS Editor

Editor has two related solutions: Solutions\AGS.Editor.Full.sln and Solutions\AGS.Editor.NoNative.sln. The latter is useful if your version of MSVS cannot build mixed (C#/C++) assemblies, or if you specifically do not want to work with and/or compile C++ code.
Within the solution there are several library projects, and "AGSEditor" project is the one creating the executable.

Editor demands several third-party .NET libraries, most of them will be downloaded automatically as NuGet packages, but you will have to manually get [irrKlang 1.6 (32-bit) for .NET 4.5](https://www.ambiera.com/irrklang/downloads.html) and put irrKlang.NET4.dll and ikpMP3.dll into Editor/References subdirectory.

As noted before, you may download the missing libraries [here](https://www.dropbox.com/s/3vdq7qw01tdtfux/ags-prebuilt-libs-3.5.x.zip?dl=0).

If you are working with AGS.Editor.NoNative solution then you won't have to deal with the parts written in C++, but you will have to get compatible compiled AGS.Native.dll somewhere. For starters we suggest taking one from the latest release of AGS.

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
* PowerShell's script execution policy may prevent the script from being executed, in which case explicitly set the scope of the restrictions to allow the local script to run:

`powershell -ExecutionPolicy RemoteSigned Windows\Installer\build.ps1 -IsccPath 'C:\Program Files (x86)\Inno Setup 6\ISCC.exe'`

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
