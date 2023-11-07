//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2023 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "gfx/blender.h"
#include <allegro.h>
#include "core/types.h"

extern "C" {
    // Fallback routine for when we don't have anything better to do.
    uint32_t _blender_black(uint32_t x, uint32_t y, uint32_t n);
    // Standard Allegro 4 trans blenders for 16 and 15-bit color modes
    uint32_t _blender_trans15(uint32_t x, uint32_t y, uint32_t n);
    uint32_t _blender_trans16(uint32_t x, uint32_t y, uint32_t n);
    // Standard Allegro 4 alpha blenders for 16 and 15-bit color modes
    uint32_t _blender_alpha15(uint32_t x, uint32_t y, uint32_t n);
    uint32_t _blender_alpha16(uint32_t x, uint32_t y, uint32_t n);
    uint32_t _blender_alpha24(uint32_t x, uint32_t y, uint32_t n);
}



// Take hue and saturation of blend colour, luminance of image
uint32_t _myblender_color15_light(uint32_t x, uint32_t y, uint32_t n)
{
    float xh, xs, xv;
    float yh, ys, yv;
    int r, g, b;

    rgb_to_hsv(getr15(x), getg15(x), getb15(x), &xh, &xs, &xv);
    rgb_to_hsv(getr15(y), getg15(y), getb15(y), &yh, &ys, &yv);

    // adjust luminance
    yv -= (1.0 - ((float)n / 250.0));
    if (yv < 0.0) yv = 0.0;

    hsv_to_rgb(xh, xs, yv, &r, &g, &b);

    return makecol15(r, g, b);
}

// Take hue and saturation of blend colour, luminance of image
// n is the last parameter passed to draw_lit_sprite
uint32_t _myblender_color16_light(uint32_t x, uint32_t y, uint32_t n)
{
    float xh, xs, xv;
    float yh, ys, yv;
    int r, g, b;

    rgb_to_hsv(getr16(x), getg16(x), getb16(x), &xh, &xs, &xv);
    rgb_to_hsv(getr16(y), getg16(y), getb16(y), &yh, &ys, &yv);

    // adjust luminance
    yv -= (1.0 - ((float)n / 250.0));
    if (yv < 0.0) yv = 0.0;

    hsv_to_rgb(xh, xs, yv, &r, &g, &b);

    return makecol16(r, g, b);
}

// Take hue and saturation of blend colour, luminance of image
uint32_t _myblender_color32_light(uint32_t x, uint32_t y, uint32_t n)
{
    float xh, xs, xv;
    float yh, ys, yv;
    int r, g, b;

    rgb_to_hsv(getr32(x), getg32(x), getb32(x), &xh, &xs, &xv);
    rgb_to_hsv(getr32(y), getg32(y), getb32(y), &yh, &ys, &yv);

    // adjust luminance
    yv -= (1.0 - ((float)n / 250.0));
    if (yv < 0.0) yv = 0.0;

    hsv_to_rgb(xh, xs, yv, &r, &g, &b);

    return makeacol32(r, g, b, geta32(y));
}

// Take hue and saturation of blend colour, luminance of image
uint32_t _myblender_color15(uint32_t x, uint32_t y, uint32_t /*n*/)
{
    float xh, xs, xv;
    float yh, ys, yv;
    int r, g, b;

    rgb_to_hsv(getr15(x), getg15(x), getb15(x), &xh, &xs, &xv);
    rgb_to_hsv(getr15(y), getg15(y), getb15(y), &yh, &ys, &yv);

    hsv_to_rgb(xh, xs, yv, &r, &g, &b);

    return makecol15(r, g, b);
}

// Take hue and saturation of blend colour, luminance of image
uint32_t _myblender_color16(uint32_t x, uint32_t y, uint32_t /*n*/)
{
    float xh, xs, xv;
    float yh, ys, yv;
    int r, g, b;

    rgb_to_hsv(getr16(x), getg16(x), getb16(x), &xh, &xs, &xv);
    rgb_to_hsv(getr16(y), getg16(y), getb16(y), &yh, &ys, &yv);

    hsv_to_rgb(xh, xs, yv, &r, &g, &b);

    return makecol16(r, g, b);
}

// Take hue and saturation of blend colour, luminance of image
uint32_t _myblender_color32(uint32_t x, uint32_t y, uint32_t /*n*/)
{
    float xh, xs, xv;
    float yh, ys, yv;
    int r, g, b;

    rgb_to_hsv(getr32(x), getg32(x), getb32(x), &xh, &xs, &xv);
    rgb_to_hsv(getr32(y), getg32(y), getb32(y), &yh, &ys, &yv);

    hsv_to_rgb(xh, xs, yv, &r, &g, &b);

    return makeacol32(r, g, b, geta32(y));
}

// trans24 blender, but preserve alpha channel from image
uint32_t _myblender_alpha_trans24(uint32_t x, uint32_t y, uint32_t n)
{
    uint32_t res, g, alph;

    if (n)
        n++;

    alph = y & 0xff000000;
    y &= 0x00ffffff;

    res = ((x & 0xFF00FF) - (y & 0xFF00FF)) * n / 256 + y;
    y &= 0xFF00;
    x &= 0xFF00;
    g = (x - y) * n / 256 + y;

    res &= 0xFF00FF;
    g &= 0xFF00;

    return res | g | alph;
}

void set_my_trans_blender(int r, int g, int b, int a)
{
    // use standard allegro 15 and 16 bit blenders, but customize
    // the 32-bit one to preserve the alpha channel
    set_blender_mode(_blender_trans15, _blender_trans16, _myblender_alpha_trans24, r, g, b, a);
}

// plain copy source to destination
// assign new alpha value as a summ of alphas.
uint32_t _additive_alpha_copysrc_blender(uint32_t x, uint32_t y, uint32_t /*n*/)
{
    uint32_t newAlpha = ((x & 0xff000000) >> 24) + ((y & 0xff000000) >> 24);

    if (newAlpha > 0xff) newAlpha = 0xff;

    return (newAlpha << 24) | (x & 0x00ffffff);
}

FORCEINLINE uint32_t argb2argb_blend_core(uint32_t src_col, uint32_t dst_col, uint32_t src_alpha)
{
    uint32_t dst_g, dst_alpha;
    src_alpha++;
    dst_alpha = geta32(dst_col);
    if (dst_alpha)
        dst_alpha++;

    // dst_g now contains the green hue from destination color
    dst_g   = (dst_col & 0x00FF00) * dst_alpha / 256;
    // dst_col now contains the red & blue hues from destination color
    dst_col = (dst_col & 0xFF00FF) * dst_alpha / 256;

    // res_g now contains the green hue of the pre-final color
    dst_g   = (((src_col & 0x00FF00) - (dst_g   & 0x00FF00)) * src_alpha / 256 + dst_g)   & 0x00FF00;
    // res_rb now contains the red & blue hues of the pre-final color
    dst_col = (((src_col & 0xFF00FF) - (dst_col & 0xFF00FF)) * src_alpha / 256 + dst_col) & 0xFF00FF;

    // dst_alpha now contains the final alpha
    // we assume that final alpha will never be zero
    dst_alpha  = 256 - (256 - src_alpha) * (256 - dst_alpha) / 256;
    // src_alpha is now the final alpha factor made for being multiplied by,
    // instead of divided by: this makes it possible to use it in faster
    // calculation below
    src_alpha  = /* 256 * 256 == */ 0x10000 / dst_alpha;

    // setting up final color hues
    dst_g   = (dst_g   * src_alpha / 256) & 0x00FF00;
    dst_col = (dst_col * src_alpha / 256) & 0xFF00FF;
    return dst_col | dst_g | (--dst_alpha << 24);
}

// blend source to destination with respect to source and destination alphas;
// assign new alpha value as a multiplication of translucenses.
// combined_alpha = front.alpha + back.alpha * (1 - front.alpha);
// combined_rgb = (front.rgb * front.alpha + back.rgb * (1 - front.alpha) * back.alpha) / combined_alpha;
uint32_t _argb2argb_blender(uint32_t src_col, uint32_t dst_col, uint32_t src_alpha)
{
    if (src_alpha > 0)
        src_alpha = geta32(src_col) * ((src_alpha & 0xFF) + 1) / 256;
    else
        src_alpha = geta32(src_col);
    if (src_alpha == 0)
        return dst_col;
    return argb2argb_blend_core(src_col, dst_col, src_alpha);
}

uint32_t _rgb2argb_blender(uint32_t src_col, uint32_t dst_col, uint32_t src_alpha)
{
    if (src_alpha == 0 || src_alpha == 0xFF)
        return src_col | 0xFF000000;
    return argb2argb_blend_core(src_col | 0xFF000000, dst_col, src_alpha);
}

uint32_t _argb2rgb_blender(uint32_t src_col, uint32_t dst_col, uint32_t src_alpha)
{
   uint32_t res, g;

   if (src_alpha > 0)
        src_alpha = geta32(src_col) * ((src_alpha & 0xFF) + 1) / 256;
    else
        src_alpha = geta32(src_col);
   if (src_alpha)
      src_alpha++;

   res = ((src_col & 0xFF00FF) - (dst_col & 0xFF00FF)) * src_alpha / 256 + dst_col;
   dst_col &= 0xFF00;
   src_col &= 0xFF00;
   g = (src_col - dst_col) * src_alpha / 256 + dst_col;

   res &= 0xFF00FF;
   g &= 0xFF00;

   return res | g;
}

// Based on _blender_alpha16, but keep source pixel if dest is transparent
uint32_t skiptranspixels_blender_alpha16(uint32_t x, uint32_t y, uint32_t n)
{
    uint32_t result;
    if ((y & 0xFFFF) == 0xF81F)
        return x;
    n = geta32(x);
    if (n)
        n = (n + 1) / 8;
    x = makecol16(getr32(x), getg32(x), getb32(x));
    x = (x | (x << 16)) & 0x7E0F81F;
    y = ((y & 0xFFFF) | (y << 16)) & 0x7E0F81F;
    result = ((x - y) * n / 32 + y) & 0x7E0F81F;
    return ((result & 0xFFFF) | (result >> 16));
}

void set_additive_alpha_blender()
{
    set_blender_mode(nullptr, nullptr, _additive_alpha_copysrc_blender, 0, 0, 0, 0);
}

void set_argb2argb_blender(int alpha)
{
    set_blender_mode(nullptr, nullptr, _argb2argb_blender, 0, 0, 0, alpha);
}

// sets the alpha channel to opaque. used when drawing a non-alpha sprite onto an alpha-sprite
uint32_t _opaque_alpha_blender(uint32_t x, uint32_t /*y*/, uint32_t /*n*/)
{
    return x | 0xff000000;
}

void set_opaque_alpha_blender()
{
    set_blender_mode(nullptr, nullptr, _opaque_alpha_blender, 0, 0, 0, 0);
}

void set_argb2any_blender()
{
    set_blender_mode_ex(_blender_black, _blender_black, _blender_black, _argb2argb_blender,
        _blender_alpha15, skiptranspixels_blender_alpha16, _blender_alpha24,
        0, 0, 0, 0xff); // TODO: do we need to support proper 15- and 24-bit here?
}

// ===============================
// [AVD] Custom blenders for software BlendMode implementation
// If we ditch software rendering we can remove this whole section

#include <allegro/internal/aintern.h>

#define BLEND(bpp, r, g, b)   _blender_trans##bpp(makecol##bpp(r, g, b), y, n)
// some formulas from AGSBlend
#define _BLENDOP_BURN(B,L) ((B + L < 255) ? 0:(B + L - 255)) // note: burn was called subtract
#define _BLENDOP_LIGHTEN(B,L) ((L > B) ? L:B)
#define _BLENDOP_DARKEN(B,L) ((L > B) ? B:L)
#define _BLENDOP_EXCLUSION(B,L) (B + L - 2 * B * L / 255)
#define _BLENDOP_SUBTRACT(B,L) (L - B < 0) ? 0:(L - B)

uint32_t _blender_mask_alpha24(uint32_t blender_result, uint32_t x, uint32_t y, uint32_t n)
{
    const uint32_t src_alpha = geta32(x);
    const uint32_t alphainv = 255 - src_alpha;
    const uint32_t r = (getr24(blender_result)*src_alpha + alphainv*getr24(y)) / 255;
    const uint32_t g = (getg24(blender_result)*src_alpha + alphainv*getg24(y)) / 255;
    const uint32_t b = (getb24(blender_result)*src_alpha + alphainv*getb24(y)) / 255;
    return r | g << 8 | b << 16;
}
// alegro's dodge blend was wrong, mine is not perfect either
uint32_t _my_blender_dodge24(uint32_t x, uint32_t y, uint32_t n)
{
    const uint32_t h = (getr24(y)*n) / (256 - getr24(x));
    const uint32_t j = (getg24(y)*n) / (256 - getg24(x));
    const uint32_t k = (getb24(y)*n) / (256 - getb24(x));
    return BLEND(24, h>255 ? 255 : h, j>255 ? 255 : j, k>255 ? 255 : k);
}
uint32_t _my_blender_burn24(uint32_t x, uint32_t y, uint32_t n)
{
    const uint32_t h = _BLENDOP_BURN(getr24(x), getr24(y));
    const uint32_t j = _BLENDOP_BURN(getg24(x), getg24(y));
    const uint32_t k = _BLENDOP_BURN(getb24(x), getb24(y));
    return BLEND(24, h, j, k);
}
uint32_t _my_blender_lighten24(uint32_t x, uint32_t y, uint32_t n)
{
    const uint32_t h = _BLENDOP_LIGHTEN(getr24(x), getr24(y));
    const uint32_t j = _BLENDOP_LIGHTEN(getg24(x), getg24(y));
    const uint32_t k = _BLENDOP_LIGHTEN(getb24(x), getb24(y));
    return BLEND(24, h, j, k);
}
uint32_t _my_blender_darken24(uint32_t x, uint32_t y, uint32_t n)
{
    const uint32_t h = _BLENDOP_DARKEN(getr24(x), getr24(y));
    const uint32_t j = _BLENDOP_DARKEN(getg24(x), getg24(y));
    const uint32_t k = _BLENDOP_DARKEN(getb24(x), getb24(y));
    return BLEND(24, h, j, k);
}
uint32_t _my_blender_exclusion24(uint32_t x, uint32_t y, uint32_t n)
{
    const uint32_t h = _BLENDOP_EXCLUSION(getr24(x), getr24(y));
    const uint32_t j = _BLENDOP_EXCLUSION(getg24(x), getg24(y));
    const uint32_t k = _BLENDOP_EXCLUSION(getb24(x), getb24(y));
    return BLEND(24, h, j, k);
}
uint32_t _my_blender_subtract24(uint32_t x, uint32_t y, uint32_t n)
{
    const uint32_t h = _BLENDOP_SUBTRACT(getr24(x), getr24(y));
    const uint32_t j = _BLENDOP_SUBTRACT(getg24(x), getg24(y));
    const uint32_t k = _BLENDOP_SUBTRACT(getb24(x), getb24(y));
    return BLEND(24, h, j, k);
}

uint32_t _blender_masked_add32(uint32_t x, uint32_t y, uint32_t n)
{
    return _blender_mask_alpha24(_blender_add24(x, y, n), x, y, n);
}
uint32_t _blender_masked_dodge32(uint32_t x, uint32_t y, uint32_t n)
{
    return _blender_mask_alpha24(_my_blender_dodge24(x, y, n), x, y, n);
}
uint32_t _blender_masked_burn32(uint32_t x, uint32_t y, uint32_t n)
{
    return _blender_mask_alpha24(_my_blender_burn24(x, y, n), x, y, n);
}
uint32_t _blender_masked_lighten32(uint32_t x, uint32_t y, uint32_t n)
{
    return _blender_mask_alpha24(_my_blender_lighten24(x, y, n), x, y, n);
}
uint32_t _blender_masked_darken32(uint32_t x, uint32_t y, uint32_t n)
{
    return _blender_mask_alpha24(_my_blender_darken24(x, y, n), x, y, n);
}
uint32_t _blender_masked_exclusion32(uint32_t x, uint32_t y, uint32_t n)
{
    return _blender_mask_alpha24(_my_blender_exclusion24(x, y, n), x, y, n);
}
uint32_t _blender_masked_subtract32(uint32_t x, uint32_t y, uint32_t n)
{
    return _blender_mask_alpha24(_my_blender_subtract24(x, y, n), x, y, n);
}
uint32_t _blender_masked_screen32(uint32_t x, uint32_t y, uint32_t n)
{
    return _blender_mask_alpha24(_blender_screen24(x, y, n), x, y, n);
}
uint32_t _blender_masked_multiply32(uint32_t x, uint32_t y, uint32_t n)
{
    return _blender_mask_alpha24(_blender_multiply24(x, y, n), x, y, n);
}
// ===============================
