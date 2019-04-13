/* Allegro MP3 - a wrapper for mpglib from mpg123 */
/* to play MP3 files with Allegro */

/* MP3 decoder part of mpglib from mpg123 (www.mpg123.com) */
/* Allegro MP3 is copyright (c) 2001, 2002 Javier Gonz lez */

/* See COPYING.txt (GNU Lesser General Public License 2.1) for license */


#ifndef ALMP3_DLL_DECLSPEC
#  ifdef ALMP3_DLL
#    ifdef ALMP3_DLL_EXPORTS
#      define ALMP3_DLL_DECLSPEC __declspec(dllexport)
#    else
#      define ALMP3_DLL_DECLSPEC __declspec(dllimport)
#    endif
#  else
#    define ALMP3_DLL_DECLSPEC
#  endif
#endif
