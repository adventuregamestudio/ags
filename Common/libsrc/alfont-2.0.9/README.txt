
                          AllegroFont v2.0.9
                         ====================

               AllegroFont (c) 2001, 2002 Javier Gonzalez

              FreeType project Copyright (c) 1996-2000 by
            David Turner, Robert Wilhelm, and Werner Lemberg
              Enhanced by Chernsha since 2004 year


AllegroFont is an Allegro wrapper for the wonderful FreeType2 library
that makes Allegro (amongst other things) able to load and render TTF
and many other famous font formats. Other feature is that it is able
to render the fonts antialiased.

AllegroFont is distributed under the FreeType Project License
(see file FTL.txt).

Note I am NOT responsable of the contents of the freetype directory.
This library is able to use FreeType code thanks to the FreeType license
(freetype/docs/ftl.txt) and *NOT* thanks to the GPL license
(freetype/docs/gpl.txt), so from the two license choices FreeTypes gives
the former is selected.

AllegroFont uses code from:
  - The FreeType project (www.freetype.org)
  - The Libiconv project for DOS version (www.gnu.org/software/libiconv/)


IMPORTANT NOTE FOR ALL VERSIONS: If you want to use the DLL please link
  to the import library (alfontdll.lib in MSVC or libalfontdll.a for the
  other systems (except DJGPP of course)) *and* define ALFONT_DLL
  *before* including alfont.h, this way for example:
    #define ALFONT_DLL
    #include <alfont.h>
  If you wish to use the STATIC library just don't define ALFONT_DLL

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

  Then edit the makefile and the header "alfont.h" in the include directory and
  uncomment the target you want to compile to then run in the command line
   "make" and that's all.

  It will generate a static link library (libalfont.a) ready to use in your
  programs, or a dynamic link library (alfont.dll and libalfontdll.a import
  lib in the Mingw32 case).

  After that, if you want to compile the examples, get inside the directory
  examples, edit the makefile uncommenting the target you want to compile to
  and type "make".


  In case you are using Microsoft Visual C++:
  
  Then edit the header "alfont.h" in the include directory and uncomment the
  target you want to compile to.
   
  Open the project file and build whatever you choose. There are two options:
    1) To build the library as a static link library (alfont_static), that will
       generate the alfont.lib file.
    2) To build the library as a dynamic link libray (alfont_dll) that will
       generate the alfontdll.lib and alfont.dll files.


How to contact me:

  xaviergonz@hotmail.com
  
  since v2.0.0
  chernsha@ms25.url.com.tw

