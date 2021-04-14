/*         ______   ___    ___ 
 *        /\  _  \ /\_ \  /\_ \ 
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___ 
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      Configuration defines for use on Unix platforms.
 *
 *      By Michael Bukin.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALUCFG_H
#define ALUCFG_H

#include <fcntl.h>
#include <unistd.h>

/* Describe this platform.  */
#define ALLEGRO_PLATFORM_STR  "Unix"



/* Define to 1 if you have the corresponding header file. */
#define ALLEGRO_HAVE_DIRENT_H
#define ALLEGRO_HAVE_INTTYPES_H
#define ALLEGRO_HAVE_STDINT_H

#define ALLEGRO_HAVE_SYS_STAT_H
#define ALLEGRO_HAVE_SYS_TIME_H

/* Define to 1 if the corresponding functions are available. */
#define ALLEGRO_HAVE_MEMCMP
/* #undef ALLEGRO_HAVE_STRICMP */
/* #undef ALLEGRO_HAVE_STRLWR */
/* #undef ALLEGRO_HAVE_STRUPR */

/*---------------------------------------------------------------------------*/

/* Define if target machine is little endian or big endian. */
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    #define ALLEGRO_LITTLE_ENDIAN
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    #define ALLEGRO_BIG_ENDIAN
#else
    #error platform not supported
#endif

/* Define for Unix platforms, to use C convention for bank switching. */
#define ALLEGRO_NO_ASM

/* Define if constructor attribute is supported. */
#define ALLEGRO_USE_CONSTRUCTOR



/* Provide implementations of missing functions.  */
#ifndef ALLEGRO_HAVE_STRICMP
#define ALLEGRO_NO_STRICMP
#endif

#ifndef ALLEGRO_HAVE_STRLWR
#define ALLEGRO_NO_STRLWR
#endif

#ifndef ALLEGRO_HAVE_STRUPR
#define ALLEGRO_NO_STRUPR
#endif

#ifndef ALLEGRO_HAVE_MEMCMP
#define ALLEGRO_NO_MEMCMP
#endif

#endif //ALUCFG_H