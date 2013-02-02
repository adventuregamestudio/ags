
                           AllegroOGG v1.0.3
                          ===================

                   AllegroOGG (c) 2002 Javier Gonzalez

        OGG Decoder part of Ogg Vorbis (Xiph.org Foundation)


AllegroOGG is an Allegro wrapper for the Ogg Vorbis (Xiph.org Foundation)
AllegroOGG is distributed under the Xiph.Org Foundation BDS alike license
(see file COPYING.txt).

The full source code (very slightly modified to make it compatible
with Allegro and DOS) of this one is available in the decoder directory.


AllegroOGG uses code from:
  - Ogg Vorbis (Xiph.org Foundation)


IMPORTANT NOTE FOR ALL VERSIONS: If you want to use the DLL please link
  to the import library (aloggdll.lib in MSVC or libaloggdll.a for the
  other systems (except DJGPP of course)) *and* define ALOGG_DLL
  *before* including alogg.h, this way for example:
    #define ALOGG_DLL
    #include <alogg.h>
  If you wish to use the STATIC library just don't define ALOGG_DLL

  Also please note the examples use the static link library, so to build
  them you will need to build the static link library first.


How to compile the library and example code:

  If you are running under a un*x system first run fixunix.sh
  If you want to use the DOS/Windows the pack is ready to be used
  as is right now, no conversion needed. fixdos.bat is only provided
  to be used if you used fixunix.sh to change file formats to unix one
  and you want them to be DOS format back again. Note that to use fixdos.bat
  you will need utod.exe.


  In case you are using GCC (DJGPP, MINGW32):

  Then edit the makefile and uncomment the target you want to compile to then run in the command
  line "make" and that's all.

  It will generate a static link library (libalogg.a) ready to use in your
  programs, or a dynamic link library (alogg.dll and libaloggdll.a import
  lib in the Mingw32 case).

  After that, if you want to compile the examples, get inside the directory
  examples, edit the makefile uncommenting the target you want to compile to
  and type "make".


  In case you are using Microsoft Visual C++:

  Open the project file and build whatever you choose. There are two options:
    1) To build the library as a static link library (alogg_static), that will
       generate the alogg.lib file.
    2) To build the library as a dynamic link libray (alogg_dll) that will
       generate the aloggdll.lib and alogg.dll files.


How to contact me:

  xaviergonz@hotmail.com


