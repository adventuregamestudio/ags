# Building the engine on any Linux
Engine core code demands partial C++11 support (uses std::shared_ptr, std::unique_ptr).
Known minimal versions of compilers that should work with AGS:

-   GCC 4.4

The following packages are required to build AGS. The versions in
parentheses are known to work, but other versions will also
probably work.

-   Allegro 4 (>= 4.2.2)
-   libaldmb (0.9.3)
-   libdumb (0.9.3)
-   libfreetype (2.4.9)
-   libogg (1.2.2-1.3.0)
-   libtheora (1.1.1-1.2.0)
-   libvorbis (1.3.2)
-   libXext (1.3.3)
-   libXxf86vm (1.1.4)

Fedora package installation
---------------------------
    yum -y install git allegro-devel dumb-devel freetype-devel libogg-devel libtheora-devel libvorbis-devel libXext-devel libXxf86vm-devel

Debian/Ubuntu package installation
----------------------------------
    sudo apt-get install git debhelper build-essential pkg-config libaldmb1-dev libfreetype6-dev libtheora-dev libvorbis-dev libogg-dev

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

To view some possible command line options, use

    ags --help

The configuration file **acsetup.cfg** in the game directory will be used
if present. Sometimes a configuration file coming with a game can cause problems,
so if a game doesn't start, try deleting **acsetup.cfg** first.

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
-   in the same folder of the AGS executable.

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
the script **debian/make_ags+libraries,sh**. The script itself can be used
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
