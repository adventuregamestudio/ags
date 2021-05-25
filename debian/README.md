# Building the engine on any Linux
Engine code demands at least partial C++11 support (uses std::shared_ptr, std::unique_ptr, etc).
Known minimal versions of compilers that should work with AGS:

-   GCC 4.4

The following packages are required to build AGS. The versions in
parentheses are known to work, but other versions will also
probably work.

-   Allegro 4 (>= 4.4.3, but 4.4.2 will also work with minimal differences)
-   libogg (1.2.2-1.3.0)
-   libtheora (1.1.1-1.2.0)
-   libvorbis (1.3.2)
-   libXext (1.3.3)
-   libXxf86vm (1.1.4)

There are two general ways to proceed: either use [CMake scripts](../CMAKE.md) or install and build everything yourself using Makefiles provided in the Engine's directory.

Fedora package installation
---------------------------
    yum -y install git allegro-devel libogg-devel libtheora-devel libvorbis-devel libXext-devel libXxf86vm-devel

Debian/Ubuntu package installation
----------------------------------
    sudo apt-get install git debhelper build-essential pkg-config liballegro4-dev libogg-dev libtheora-dev libvorbis-dev

Other Linux systems use their respective package managers.

Download and build
------------------
Download the source with git and change into the **ags** directory:

    git clone git://github.com/adventuregamestudio/ags.git
    cd ags

Compile the engine:

    make --directory=Engine

The **ags** executable can now be found in the **Engine** folder and
can be installed with

    sudo make --directory=Engine install

# Building a Debian/Ubuntu package of AGS
Building a package is the preferred way to install software on
Debian/Ubuntu. This is how it's done.

Download the sources with git and change into the **ags** directory:

    git clone git://github.com/adventuregamestudio/ags.git
    cd ags

Build the package and install it with gdebi:

    fakeroot debian/rules binary
    sudo gdebi ../ags_3~git-1_*.deb

# Using the engine
To start an AGS game, just run ags with the game directory or the game
file as parameter, e.g.

    ags /path/to/game/

or

    ags game.exe

To view available command line options, use

    ags --help

The configuration file **acsetup.cfg** in the game directory will be used
if present. For more information on configuration and command line arguments
see [OPTIONS.md](../OPTIONS.md).

## SDL2-based digital sound driver

Standard Allegro 4's sound drivers sometimes do not work well on Linux. For that reason there's an alternate sound driver written by [Edward Rudd](https://github.com/urkle) which uses SDL2 for the audio output. This driver's source code may be found in our [allegro fork](https://github.com/adventuregamestudio/lib-allegro/blob/allegro-4.4.3.1-agspatch/src/unix/sdl2digi.c).

Naturally this driver requires installed SDL2 library to work (libsdl2), and dev library (e.g. libsdl2-dev) to compile.

If you are using AGS CMake script this driver will be statically linked in the engine so long as you have got above dev library installed. It is also built as a dynamically linked module by the script that prepares engine and libraries for default game release (see [section below](#building-ags-for-a-game-release) for further information on this).

If you are building Allegro 4 from our repository yourself, its CMake script will build it as a module so long as ALLEGRO_WITH_MODULES option is enabled (and you have SDL2 library installed). Without CMake you'll have to deal with this yourself the way you see fit.

AGS will use SDL2 digital driver if all the following is true:
* it was either embedded in program or present as a module;
* in game config `digiid` option is either set to `sdl2` or it's set to `auto` while SDL2 driver has priority among the modules;
* SDL2 runtime library is installed in the system.

Note that Allegro searches for modules in the path defined by `ALLEGRO_MODULES` enviroment variable. There has to be a `modules.lst` file found at that path, the default example of such file may be found in [allegro repository](https://github.com/adventuregamestudio/lib-allegro/blob/allegro-4.4.3.1-agspatch/modules.lst). Adding module's \*.so name (e.g. `alleg-sdl2digi.so`) to the *end* of the list will give this driver a priority.

## MIDI music support

For midi music playback, you have to download GUS patches. We recommend
"Richard Sanders's GUS patches" from this address:

http://alleg.sourceforge.net/digmid.html

A direct link is here:

http://www.eglebbk.dds.nl/program/download/digmid.dat

This 'digmid.dat' is, in fact, a **bzip2** archive, containing actual data file,
which should be about 25 MB large. Extract that file and rename it to **patches.dat**.
You can now place it:

-   in the directory pointed to by the ALLEGRO environment variable; or
-   if $ALLEGRO is not defined, in $HOME; or
-   in the same directory of the AGS executable; or
-   in the game's directory.

# Debugging
When using the Debian/Ubuntu package, the package ags-dbg_*.deb containing debugging
symbols is created alongside ags_*.deb. The build date and the name of the
last git commit at the time of building are stored in the package description,
which can be viewed with

    apt-cache show ags

This information should be included in bug reports.

# Building AGS for a game release
If you want to build AGS for inclusion in a game release, you want an
engine that runs on most 32 and 64 bit Linux systems regardless of the library
versions that are installed on that system. You can get such a built by using
the script **debian/make_ags+libraries.sh**. The script itself can be used
on Debian or Ubuntu. See the comments in the script for instructions.

# Workaround: 32 bit AGS on 64 bit Debian/Ubuntu
In the past AGS worked only on 32 bit architectures, so it was necessary to compile
a 32 bit version on 64 bit systems. This is not necessary anymore, but these
instructions are kept for reference and may be helpful for debugging etc.

The development versions of Debian and Ubuntu support parallel
installation of both 32 and 64 bit versions of all required libraries
(multiarch), so you can build a 32 bit AGS to use on your 64 bit system.
This part works only on Debian sid and wheezy and Ubuntu quantal.

Download the sources with git and change into the **ags** directory:

    git clone git://github.com/adventuregamestudio/ags.git
    cd ags

## Matching working directory and orig tarball
To build the package, it is required that there is an "orig tarball"
that has the same content as the working directory. This tarball is generated
from the git content with

    debian/rules get-orig-source

The working directory must have the same content as git, i.e. be "clean".
To ensure this, check if the working directory is clean with

    git status

If there are changes, run 

    debian/rules clean 

and/or

    git reset --hard HEAD

If there are still untracked files, delete them manually.

Run `debian/rules get-orig-source` every time the sources change. If
you want to change the sources yourself, you have to commit the
changes to git, then run `debian/rules get-orig-source`, then
build the package.


## Building the package

Enable multiarch:

    sudo dpkg --add-architecture i386
    sudo apt-get update

Install and prepare pbuilder (use the same distribution you are using,
i.e. `sid`, `wheezy` or `quantal`):

    sudo apt-get install pbuilder
    sudo pbuilder create --distribution sid --architecture i386

This creates an i386 chroot which will be used to build the i386 package
on an amd64 system. pbuilder automatically manages the build dependencies.
The pbuilder base can later be updated with

    sudo pbuilder update

Build the package with pbuilder and install it and its dependencies with gdebi:

    cd ags
    pdebuild
    sudo gdebi /var/cache/pbuilder/result/ags_3~git-1_i386.deb
