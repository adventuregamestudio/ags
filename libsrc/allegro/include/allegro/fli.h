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
 *      FLI/FLC routines.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_FLI_H
#define ALLEGRO_FLI_H

#include "base.h"
#include "file.h"
#include "palette.h"

#ifdef __cplusplus
   extern "C" {
#endif

struct BITMAP;

#define FLI_OK          0              /* FLI player return values */
#define FLI_EOF         -1
#define FLI_ERROR       -2
#define FLI_NOT_OPEN    -3

AL_FUNC(int, open_fli_pf, (PACKFILE *pf));
AL_FUNC(int, open_memory_fli, (void *fli_data));
AL_FUNC(void, close_fli, (void));
AL_FUNC(int, next_fli_frame, (int loop));
AL_FUNC(int, skip_fli_frame, (int loop));
AL_FUNC(void, reset_fli_variables, (void));

AL_VAR(struct BITMAP *, fli_bitmap);   /* current frame of the FLI */
AL_VAR(PALETTE, fli_palette);          /* current FLI palette */

AL_VAR(int, fli_bmp_dirty_from);       /* what part of fli_bitmap is dirty */
AL_VAR(int, fli_bmp_dirty_to);
AL_VAR(int, fli_pal_dirty_from);       /* what part of fli_palette is dirty */
AL_VAR(int, fli_pal_dirty_to);

AL_VAR(int, fli_frame);                /* current frame number */
AL_VAR(int, fli_frame_count);          /* total number of frames */

AL_VAR(int, fli_speed);                /* FLI playback speed factor, in milliseconds */

#ifdef __cplusplus
   }
#endif

#endif          /* ifndef ALLEGRO_FLI_H */


