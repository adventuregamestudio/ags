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
 *      Math routines, etc.
 *
 *      By Michael Bukin.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"



#ifdef ALLEGRO_NO_ASM
/* Empty bank switch routines.  Should be used with C calling convention. */

uintptr_t _stub_bank_switch(BITMAP *bmp, int y)
{
   return (uintptr_t)bmp->line[y];
}

void _stub_unbank_switch(BITMAP *bmp)
{
}

void _stub_bank_switch_end(void)
{
}

#else

/*
 * For ASM calling convention:
 *   Unix version uses bank switch routines from src/i386/imisc.s.
 *   DOS version uses bank switch routines from -#- or src/c/cmiscs.s.
 */

#endif


