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
 *      Main header file for the entire Allegro library.
 *      (separate modules can be included from the allegro/ directory)
 *
 *      By Shawn Hargreaves.
 *
 *      Vincent Penquerc'h split the original allegro.h into separate headers.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_H
#define ALLEGRO_H

#include "allegro/base.h"

#include "allegro/system.h"
#include "allegro/debug.h"

#include "allegro/unicode.h"

#include "allegro/palette.h"
#include "allegro/gfx.h"
#include "allegro/color.h"
#include "allegro/draw.h"

#include "allegro/fli.h"

#include "allegro/file.h"
#include "allegro/datafile.h"

#include "allegro/fixed.h"
#include "allegro/fmaths.h"

#ifdef ALLEGRO_EXTRA_HEADER
   #include ALLEGRO_EXTRA_HEADER
#endif

#endif          /* ifndef ALLEGRO_H */


