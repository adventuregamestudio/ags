# Building the engine on any Linux

There are two general ways to proceed: either use [**CMake**](../CMAKE.md) or install and build 
everything yourself using Makefiles provided in the Engine's directory.

Engine code demands at least partial C++11 support (uses std::shared_ptr, std::unique_ptr, etc).
Known minimal versions of compilers that should work with AGS:

-   GCC 4.4

## Dependencies

The following packages are required to build AGS. The versions in
parentheses are known to work, but other versions will also
probably work.

-   libsdl2 (2.0.12 or higher)
-   libsdl2_sound (2.0.x, revision 1507be95c3 or higher)
-   libogg (1.2.2-1.3.0)
-   libtheora (1.1.1-1.2.0)
-   libvorbis (1.3.2)

If you are building using CMake, you have the option to let the CMake build system fetch and build the dependencies,
or use locally installed libraries. This can be configured, and is [explained in CMake own readme](../CMAKE.md#using-locally-installed-libraries).

### Fedora package installation

    yum -y install git sdl2-devel libogg-devel libtheora-devel libvorbis-devel

### Debian/Ubuntu package installation

    sudo apt-get install git debhelper build-essential pkg-config libsdl2-dev libogg-dev libtheora-dev libvorbis-dev

Other Linux systems use their respective package managers.

## SDL_Sound library installation

At the time of writing SDL_Sound `2.*` has just been released, but almost no linux distro provides it.
Until that is resolved, we recommend to clone their repository from https://github.com/icculus/SDL_sound.
Or download particular revision archive using following url:

    https://github.com/icculus/SDL_sound/archive/1507be95c3605e4fd6a48ea4c527e4aa711a1566.tar.gz

then build and install using CMake (see instructions in the SDL_Sound's docs).

## Download

Download the source with git and change into the **ags** directory:

    git clone git://github.com/adventuregamestudio/ags.git
    cd ags
    
## Building AGS

Compile the engine:

    make --directory=Engine

The **ags** executable can now be found in the **Engine** folder and
can be installed with

    sudo make --directory=Engine install
    
In case you are using CMake there are different ways you can do, and it's better to read in 
[**CMake README**](../CMAKE.md), following is an example of the default cmake release build.

    mkdir build-release
    cd build-release
    cmake -S .. -B . -DCMAKE_BUILD_TYPE=Release
    cmake --build .
    
The CMake way to install would be

    cmake --install .


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

## MIDI music support

Since 3.6.0 engine is using SDL_Sound library to load MIDI files, which internally utilizes Timidity.

For MIDI playback you have to install GUS compatible patches, and have `timidity.cfg` present.

For more information on this please refer to a [dedicated article in the AGS manual](https://github.com/adventuregamestudio/ags-manual/wiki/MIDI-playback).

## Plugin support

For games that require plugins to run, one of the following requirements must be met for each plugin:
- Game should come with linux versions of this plugun.
- Game dir should contain a plugin "stub", that is - a dummy plugin that does nothing and only exports required functions.
- Engine should have this plugin built-in (statically linked).
- Engine should have this plugin's "stubs" embedded.

If none of the above is true, game will only run if the plugin is optional. This may be a case with plugins that do something behind the scenes but do not export any functions to the game script. Because if the game script uses plugin functions, and plugin is missing, then the script will fail to load (error on script linking stage).

Another thing to consider is plugin's dependencies. If a plugin depends on particular libraries, then these libraries should be installed on your system. This may be learnt if you run the engine with the verbose log (e.g. ```ags --log-stdout=main:all <game path>```) and inspect "dlopen" error messages.

If a plugin loads another custom library, present in the game dir, then normally the system may fail to find such dependency. The solution is to setup a `LD_LIBRARY_PATH` enviroment variable right before running the engine. This may be done either by writing a launch script for this game, or right in a command-line, for example:

    LD_LIBRARY_PATH=path_to_game_dir/ ags path_to_game_dir


# Debugging

When using the Debian/Ubuntu package, the package `ags-dbg_*.deb` containing debugging
symbols is created alongside `ags_*.deb`. The build date and the name of the
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
