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
 *      System level: initialization, cleanup, etc.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_SYSTEM_H
#define ALLEGRO_SYSTEM_H

#include "base.h"
#include "unicode.h"

#ifdef __cplusplus
   extern "C" {
#endif

struct RGB;
struct BITMAP;
struct GFX_VTABLE;

#define ALLEGRO_ERROR_SIZE 256

AL_ARRAY(char, allegro_id);
AL_ARRAY(char, allegro_error);


#define SYSTEM_AUTODETECT  0
#define SYSTEM_NONE        AL_ID('N','O','N','E')

#define MAKE_VERSION(a, b, c) (((a)<<16)|((b)<<8)|(c))

AL_FUNC(int, _install_allegro_version_check, (int system_id, int *errno_ptr,
   AL_METHOD(int, atexit_ptr, (AL_METHOD(void, func, (void)))), int version));

AL_INLINE(int, install_allegro, (int system_id, int *errno_ptr,
   AL_METHOD(int, atexit_ptr, (AL_METHOD(void, func, (void))))),
{
   return _install_allegro_version_check(system_id, errno_ptr, atexit_ptr, \
      MAKE_VERSION(ALLEGRO_VERSION, ALLEGRO_SUB_VERSION, ALLEGRO_WIP_VERSION));
})

#define allegro_init()  _install_allegro_version_check(SYSTEM_AUTODETECT, &errno, \
   (int (*)(void (*)(void)))atexit, \
   MAKE_VERSION(ALLEGRO_VERSION, ALLEGRO_SUB_VERSION, ALLEGRO_WIP_VERSION))

AL_FUNC(void, allegro_exit, (void));

AL_PRINTFUNC(void, allegro_message, (AL_CONST char *msg, ...), 1, 2);

#ifdef __cplusplus
   }
#endif

#endif          /* ifndef ALLEGRO_SYSTEM_H */


