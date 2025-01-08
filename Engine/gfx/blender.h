//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// AGS specific color blending routines for transparency and tinting effects
//
//=============================================================================
#ifndef __AC_BLENDER_H
#define __AC_BLENDER_H

#include "core/types.h"

//
// Allegro's standard alpha blenders result in:
// - src and dst RGB are combined proportionally to src alpha
//   (src.rgb * src.alpha + dst.rgb * (1 - dst.alpha));
// - final alpha is zero.
// This blender is suggested for use with opaque destinations
// (ones without alpha channel).
//
/* Declared in Allegro's color.h:
void set_alpha_blender();
*/

uint32_t _myblender_color32(uint32_t x, uint32_t y, uint32_t n);
uint32_t _myblender_color32_light(uint32_t x, uint32_t y, uint32_t n);
// Customizable alpha blender that uses the supplied alpha value as src alpha,
// and preserves destination's alpha channel (if there was one);
void set_my_trans_blender(int r, int g, int b, int a);
// Argb2argb alpha blender combines RGBs proportionally to src alpha, but also
// applies dst alpha factor to the dst RGB used in the merge;
// The final alpha is calculated by multiplying two translucences (1 - .alpha).
// Custom alpha parameter, when not zero, is treated as fraction of source
// alpha that has to be used in color blending.
uint32_t _argb2argb_blender(uint32_t src_col, uint32_t dst_col, uint32_t src_alpha);
// Argb2rgb blender combines RGBs proportionally to src alpha, but discards alpha in the end.
// It is almost a clone of Allegro's _blender_alpha32, except it also applies optional overall alpha.
uint32_t _argb2rgb_blender(uint32_t src_col, uint32_t dst_col, uint32_t src_alpha);
// Rgb2argb blender treats all src pixels as if having opaque alpha.
uint32_t _rgb2argb_blender(uint32_t src_col, uint32_t dst_col, uint32_t src_alpha);
// Sets the alpha channel to opaque. Used when drawing a non-alpha sprite onto an alpha-sprite.
uint32_t _opaque_alpha_blender(uint32_t src_col, uint32_t dst_col, uint32_t src_alpha);

// Opaque alpha blender plain copies src over, applying opaque alpha value.
void set_opaque_alpha_blender();
// Sets argb2argb for 32-bit mode, and provides appropriate funcs for blending 32-bit onto 15/16/24-bit destination
void set_argb2any_blender();

// ===============================
// [AVD] Custom blenders for software BlendMode implementation
// If we ditch software rendering we can remove this whole section
// not very pretty but forces the original alpha of "x" to "blender_result"
uint32_t _blender_mask_alpha24(uint32_t blender_result, uint32_t x, uint32_t y, uint32_t n);
uint32_t _my_blender_dodge24(uint32_t x, uint32_t y, uint32_t n);
uint32_t _my_blender_burn24(uint32_t x, uint32_t y, uint32_t n);
uint32_t _my_blender_lighten24(uint32_t x, uint32_t y, uint32_t n);
uint32_t _my_blender_darken24(uint32_t x, uint32_t y, uint32_t n);
uint32_t _my_blender_exclusion24(uint32_t x, uint32_t y, uint32_t n);
uint32_t _my_blender_subtract24(uint32_t x, uint32_t y, uint32_t n);

uint32_t _blender_masked_add32(uint32_t x, uint32_t y, uint32_t n);
uint32_t _blender_masked_dodge32(uint32_t x, uint32_t y, uint32_t n);
uint32_t _blender_masked_burn32(uint32_t x, uint32_t y, uint32_t n);
uint32_t _blender_masked_lighten32(uint32_t x, uint32_t y, uint32_t n);
uint32_t _blender_masked_darken32(uint32_t x, uint32_t y, uint32_t n);
uint32_t _blender_masked_exclusion32(uint32_t x, uint32_t y, uint32_t n);
uint32_t _blender_masked_subtract32(uint32_t x, uint32_t y, uint32_t n);
uint32_t _blender_masked_screen32(uint32_t x, uint32_t y, uint32_t n);
uint32_t _blender_masked_multiply32(uint32_t x, uint32_t y, uint32_t n);
// ===============================

#endif // __AC_BLENDER_H
