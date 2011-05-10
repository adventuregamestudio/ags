/* Allegro OGG */
/* to play OGG files with Allegro */

/* OGG decoder part of Ogg Vorbis (Xiph.org Foundation) */
/* Allegro OGG is copyright (c) 2002 Javier Gonz lez */

/* See COPYING.txt for license */


#ifndef ALOGG_DLL_DECLSPEC
#  ifdef ALOGG_DLL
#    ifdef ALOGG_DLL_EXPORTS
#      define ALOGG_DLL_DECLSPEC __declspec(dllexport)
#    else
#      define ALOGG_DLL_DECLSPEC __declspec(dllimport)
#    endif
#  else
#    define ALOGG_DLL_DECLSPEC
#  endif
#endif
