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
 *      Android specific header defines.
 *
 *      By JJS for the Adventure Game Studio runtime port.
 *      Based on the Allegro PSP port.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALIOSCFG_H
#define ALIOSCFG_H

#ifndef SCAN_DEPEND
   #include <fcntl.h>
   #include <unistd.h>
#endif


#include <stdio.h>

#define ALLEGRO_HAVE_DIRENT_H   1
#define ALLEGRO_HAVE_INTTYPES_H 1
#define ALLEGRO_HAVE_STDINT_H   1
#define ALLEGRO_HAVE_SYS_TIME_H 1
#define ALLEGRO_HAVE_SYS_STAT_H 1

#define ALLEGRO_HAVE_LIBPTHREAD 1

/* Describe this platform */
#define ALLEGRO_PLATFORM_STR  "iOS"
#define ALLEGRO_USE_CONSTRUCTOR
#define ALLEGRO_MULTITHREADED
#define ALLEGRO_LITTLE_ENDIAN

/* Provide implementations of missing definitions */
#ifndef O_BINARY
   #define O_BINARY     0
#endif
#define dup(X)	(fcntl(X, F_DUPFD, 0))

/* Exclude ASM */
#ifndef ALLEGRO_NO_ASM
   #define ALLEGRO_NO_ASM
#endif

// used by readbmp
#define stricmp strcasecmp

#endif //ALIOSCFG_H
