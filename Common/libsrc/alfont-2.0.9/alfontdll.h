/* AllegroFont - a wrapper for FreeType 2 */
/* to render TTF and other font formats with Allegro */

            
/* FreeType 2 is copyright (c) 1996-2000 */
/* David Turner, Robert Wilhelm, and Werner Lemberg */
/* AllegroFont is copyright (c) 2001, 2002 Javier Gonz lez */

/* See FTL.txt (FreeType Project License) for license */


#ifndef ALFONT_DLL_DECLSPEC
#  ifdef ALFONT_DLL
#    ifdef ALFONT_DLL_EXPORTS
#      define ALFONT_DLL_DECLSPEC __declspec(dllexport)
#    else
#      define ALFONT_DLL_DECLSPEC __declspec(dllimport)
#    endif
#  else
#    define ALFONT_DLL_DECLSPEC
#  endif
#endif
