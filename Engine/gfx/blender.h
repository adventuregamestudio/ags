//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================
//
// AGS specific color blending routines for transparency and tinting effects
//
//=============================================================================

#ifndef __AC_BLENDER_H
#define __AC_BLENDER_H

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

unsigned long _myblender_color15(unsigned long x, unsigned long y, unsigned long n);
unsigned long _myblender_color16(unsigned long x, unsigned long y, unsigned long n);
unsigned long _myblender_color32(unsigned long x, unsigned long y, unsigned long n);
unsigned long _myblender_color15_light(unsigned long x, unsigned long y, unsigned long n);
unsigned long _myblender_color16_light(unsigned long x, unsigned long y, unsigned long n);
unsigned long _myblender_color32_light(unsigned long x, unsigned long y, unsigned long n);
// Customizable alpha blender that uses the supplied alpha value as src alpha,
// and preserves destination's alpha channel (if there was one);
void set_my_trans_blender(int r, int g, int b, int a);
// Argb2argb alpha blender combines RGBs proportionally to src alpha, but also
// applies dst alpha factor to the dst RGB used in the merge;
// The final alpha is calculated by multiplying two translucences (1 - .alpha).
// Custom alpha parameter, when not zero, is treated as fraction of source
// alpha that has to be used in color blending.
unsigned long _argb2argb_blender(unsigned long src_col, unsigned long dst_col, unsigned long src_alpha);
// Argb2rgb blender combines RGBs proportionally to src alpha, but discards alpha in the end.
// It is almost a clone of Allegro's _blender_alpha32, except it also applies optional overall alpha.
unsigned long _argb2rgb_blender(unsigned long src_col, unsigned long dst_col, unsigned long src_alpha);
// Rgb2argb blender treats all src pixels as if having opaque alpha.
unsigned long _rgb2argb_blender(unsigned long src_col, unsigned long dst_col, unsigned long src_alpha);
// Sets the alpha channel to opaque. Used when drawing a non-alpha sprite onto an alpha-sprite.
unsigned long _opaque_alpha_blender(unsigned long src_col, unsigned long dst_col, unsigned long src_alpha);

// Additive alpha blender plain copies src over, applying a summ of src and
// dst alpha values.
void set_additive_alpha_blender();
// Opaque alpha blender plain copies src over, applying opaque alpha value.
void set_opaque_alpha_blender();
// Sets argb2argb for 32-bit mode, and provides appropriate funcs for blending 32-bit onto 15/16/24-bit destination
void set_argb2any_blender();

// ===============================
// [AVD] Custom blenders for software BlendMode implementation
// If we ditch software rendering we can remove this whole section
#include <allegro.h>
#include <allegro/internal/aintern.h>

#define BLEND(bpp, r, g, b)   _blender_trans##bpp(makecol##bpp(r, g, b), y, n)
// some formulas from AGSBlend
#define _BLENDOP_BURN(B,L) ((B + L < 255) ? 0:(B + L - 255)) // note: burn was called subtract
#define _BLENDOP_LIGHTEN(B,L) ((L > B) ? L:B)
#define _BLENDOP_DARKEN(B,L) ((L > B) ? B:L)
#define _BLENDOP_EXCLUSION(B,L) (B + L - 2 * B * L / 255)
#define _BLENDOP_SUBTRACT(B,L) (L - B < 0) ? 0:(L - B)
// not very pretty but forces the original alpha of "x" to "blender_result"
inline unsigned long _blender_mask_alpha24(unsigned long blender_result, unsigned long x, unsigned long y, unsigned long n)
{
    const unsigned long src_alpha = geta32(x);
    const unsigned long alphainv = 255 - src_alpha;
    const unsigned long r = (getr24(blender_result)*src_alpha + alphainv*getr24(y)) / 255;
    const unsigned long g = (getg24(blender_result)*src_alpha + alphainv*getg24(y)) / 255;
    const unsigned long b = (getb24(blender_result)*src_alpha + alphainv*getb24(y)) / 255;
    return r | g << 8 | b << 16;
}
// alegro's dodge blend was wrong, mine is not perfect either
inline unsigned long _my_blender_dodge24(unsigned long x, unsigned long y, unsigned long n)
{
    const unsigned long h = (getr24(y)*n) / (256 - getr24(x));
    const unsigned long j = (getg24(y)*n) / (256 - getg24(x));
    const unsigned long k = (getb24(y)*n) / (256 - getb24(x));
    return BLEND(24, h>255 ? 255 : h, j>255 ? 255 : j, k>255 ? 255 : k);
}
inline unsigned long _my_blender_burn24(unsigned long x, unsigned long y, unsigned long n)
{
    const unsigned long h = _BLENDOP_BURN(getr24(x), getr24(y));
    const unsigned long j = _BLENDOP_BURN(getg24(x), getg24(y));
    const unsigned long k = _BLENDOP_BURN(getb24(x), getb24(y));
    return BLEND(24, h, j, k);
}
inline unsigned long _my_blender_lighten24(unsigned long x, unsigned long y, unsigned long n)
{
    const unsigned long h = _BLENDOP_LIGHTEN(getr24(x), getr24(y));
    const unsigned long j = _BLENDOP_LIGHTEN(getg24(x), getg24(y));
    const unsigned long k = _BLENDOP_LIGHTEN(getb24(x), getb24(y));
    return BLEND(24, h, j, k);
}
inline unsigned long _my_blender_darken24(unsigned long x, unsigned long y, unsigned long n)
{
    const unsigned long h = _BLENDOP_DARKEN(getr24(x), getr24(y));
    const unsigned long j = _BLENDOP_DARKEN(getg24(x), getg24(y));
    const unsigned long k = _BLENDOP_DARKEN(getb24(x), getb24(y));
    return BLEND(24, h, j, k);
}
inline unsigned long _my_blender_exclusion24(unsigned long x, unsigned long y, unsigned long n)
{
    const unsigned long h = _BLENDOP_EXCLUSION(getr24(x), getr24(y));
    const unsigned long j = _BLENDOP_EXCLUSION(getg24(x), getg24(y));
    const unsigned long k = _BLENDOP_EXCLUSION(getb24(x), getb24(y));
    return BLEND(24, h, j, k);
}
inline unsigned long _my_blender_subtract24(unsigned long x, unsigned long y, unsigned long n)
{
    const unsigned long h = _BLENDOP_SUBTRACT(getr24(x), getr24(y));
    const unsigned long j = _BLENDOP_SUBTRACT(getg24(x), getg24(y));
    const unsigned long k = _BLENDOP_SUBTRACT(getb24(x), getb24(y));
    return BLEND(24, h, j, k);
}

inline unsigned long _blender_masked_add32(unsigned long x, unsigned long y, unsigned long n)
{
    return _blender_mask_alpha24(_blender_add24(x, y, n), x, y, n);
}
inline unsigned long _blender_masked_dodge32(unsigned long x, unsigned long y, unsigned long n)
{
    return _blender_mask_alpha24(_my_blender_dodge24(x, y, n), x, y, n);
}
inline unsigned long _blender_masked_burn32(unsigned long x, unsigned long y, unsigned long n)
{
    return _blender_mask_alpha24(_my_blender_burn24(x, y, n), x, y, n);
}
inline unsigned long _blender_masked_lighten32(unsigned long x, unsigned long y, unsigned long n)
{
    return _blender_mask_alpha24(_my_blender_lighten24(x, y, n), x, y, n);
}
inline unsigned long _blender_masked_darken32(unsigned long x, unsigned long y, unsigned long n)
{
    return _blender_mask_alpha24(_my_blender_darken24(x, y, n), x, y, n);
}
inline unsigned long _blender_masked_exclusion32(unsigned long x, unsigned long y, unsigned long n)
{
    return _blender_mask_alpha24(_my_blender_exclusion24(x, y, n), x, y, n);
}
inline unsigned long _blender_masked_subtract32(unsigned long x, unsigned long y, unsigned long n)
{
    return _blender_mask_alpha24(_my_blender_subtract24(x, y, n), x, y, n);
}
inline unsigned long _blender_masked_screen32(unsigned long x, unsigned long y, unsigned long n)
{
    return _blender_mask_alpha24(_blender_screen24(x, y, n), x, y, n);
}
inline unsigned long _blender_masked_multiply32(unsigned long x, unsigned long y, unsigned long n)
{
    return _blender_mask_alpha24(_blender_multiply24(x, y, n), x, y, n);
}
// ===============================

#endif // __AC_BLENDER_H
