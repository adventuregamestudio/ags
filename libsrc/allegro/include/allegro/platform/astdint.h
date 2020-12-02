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
 *      A header file to get definitions of uint*_t and int*_t.
 *
 *      By Peter Wang.
 * 
 *      See readme.txt for copyright information.
 */

#ifndef ASTDINT_H
#define ASTDINT_H

/* Please only include this file from include/allegro/internal/alconfig.h
 * and don't add more than inttypes.h/stdint.h emulation here.  Thanks.
 */



#if defined ALLEGRO_HAVE_INTTYPES_H
   #include <inttypes.h>
#elif defined ALLEGRO_HAVE_STDINT_H
   #include <stdint.h>
#else
   #error I dunno how to get the definitions of fixed-width integer types on your platform.  Please report this to your friendly Allegro developer.
#endif



#endif /* ifndef ASTDINT_H */
