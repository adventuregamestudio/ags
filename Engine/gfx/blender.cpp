
#include "gfx/blender.h"
#include "wgt2allg.h"
#include "gfx/gfxfilterdefines.h"

extern "C" {
    unsigned long _blender_trans16(unsigned long x, unsigned long y, unsigned long n);
    unsigned long _blender_trans15(unsigned long x, unsigned long y, unsigned long n);
}

// the allegro "inline" ones are not actually inline, so #define
// over them to speed it up
#define getr32(xx) ((xx >> _rgb_r_shift_32) & 0xFF)
#define getg32(xx) ((xx >> _rgb_g_shift_32) & 0xFF)
#define getb32(xx) ((xx >> _rgb_b_shift_32) & 0xFF)
#define geta32(xx) ((xx >> _rgb_a_shift_32) & 0xFF)
#define makeacol32(r,g,b,a) ((r << _rgb_r_shift_32) | (g << _rgb_g_shift_32) | (b << _rgb_b_shift_32) | (a << _rgb_a_shift_32))

// Take hue and saturation of blend colour, luminance of image
unsigned long _myblender_color15_light(unsigned long x, unsigned long y, unsigned long n)
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
unsigned long _myblender_color16_light(unsigned long x, unsigned long y, unsigned long n)
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
unsigned long _myblender_color32_light(unsigned long x, unsigned long y, unsigned long n)
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
unsigned long _myblender_color15(unsigned long x, unsigned long y, unsigned long n)
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
unsigned long _myblender_color16(unsigned long x, unsigned long y, unsigned long n)
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
unsigned long _myblender_color32(unsigned long x, unsigned long y, unsigned long n)
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
unsigned long _myblender_alpha_trans24(unsigned long x, unsigned long y, unsigned long n)
{
    unsigned long res, g, alph;

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

void set_my_trans_blender(int r, int g, int b, int a) {
    // use standard allegro 15 and 16 bit blenders, but customize
    // the 32-bit one to preserve the alpha channel
    set_blender_mode(_blender_trans15, _blender_trans16, _myblender_alpha_trans24, r, g, b, a);
}



// add the alpha values together, used for compositing alpha images
unsigned long _additive_alpha_blender(unsigned long x, unsigned long y, unsigned long n)
{
    unsigned long newAlpha = ((x & 0xff000000) >> 24) + ((y & 0xff000000) >> 24);

    if (newAlpha > 0xff) newAlpha = 0xff;

    return (newAlpha << 24) | (x & 0x00ffffff);
}

void set_additive_alpha_blender() {
    // add the alpha channels together
    set_blender_mode(NULL, NULL, _additive_alpha_blender, 0, 0, 0, 0);
}

// sets the alpha channel to opaque. used when drawing a non-alpha sprite onto an alpha-sprite
unsigned long _opaque_alpha_blender(unsigned long x, unsigned long y, unsigned long n)
{
    return x | 0xff000000;
}

void set_opaque_alpha_blender() {
    set_blender_mode(NULL, NULL, _opaque_alpha_blender, 0, 0, 0, 0);
}
