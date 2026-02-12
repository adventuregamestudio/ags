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
 *      Color blending routines.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */

#ifndef ALLEGRO_COLORBLEND_H
#define ALLEGRO_COLORBLEND_H

#include "base.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef AL_METHOD(uint32_t, BLENDER_FUNC, (uint32_t x, uint32_t y, uint32_t n));

AL_FUNC(void, set_blender_mode, (BLENDER_FUNC b15, BLENDER_FUNC b16, BLENDER_FUNC b24, int r, int g, int b, int a));
AL_FUNC(void, set_blender_mode_ex, (BLENDER_FUNC b15, BLENDER_FUNC b16, BLENDER_FUNC b24, BLENDER_FUNC b32, BLENDER_FUNC b15x, BLENDER_FUNC b16x, BLENDER_FUNC b24x, int r, int g, int b, int a));

AL_FUNC(void, set_alpha_blender, (void));
AL_FUNC(void, set_write_alpha_blender, (void));
AL_FUNC(void, set_trans_blender, (int r, int g, int b, int a));
AL_FUNC(void, set_add_blender, (int r, int g, int b, int a));
AL_FUNC(void, set_burn_blender, (int r, int g, int b, int a));
AL_FUNC(void, set_color_blender, (int r, int g, int b, int a));
AL_FUNC(void, set_difference_blender, (int r, int g, int b, int a));
AL_FUNC(void, set_dissolve_blender, (int r, int g, int b, int a));
AL_FUNC(void, set_dodge_blender, (int r, int g, int b, int a));
AL_FUNC(void, set_hue_blender, (int r, int g, int b, int a));
AL_FUNC(void, set_invert_blender, (int r, int g, int b, int a));
AL_FUNC(void, set_luminance_blender, (int r, int g, int b, int a));
AL_FUNC(void, set_multiply_blender, (int r, int g, int b, int a));
AL_FUNC(void, set_saturation_blender, (int r, int g, int b, int a));
AL_FUNC(void, set_screen_blender, (int r, int g, int b, int a));

AL_FUNC(void, create_blender_table, (COLOR_MAP *table, AL_CONST PALETTE pal, AL_METHOD(void, callback, (int pos))));

#ifdef __cplusplus
   }
#endif

#endif /* ifndef ALLEGRO_COLORBLEND_H */
