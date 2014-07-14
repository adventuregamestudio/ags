/* display.c, Allegro interface */

/* the Xlib interface is closely modeled after
* mpeg_play 2.0 by the Berkeley Plateau Research Group
*/

/* the Allegro interface was created with the original Xlib
* interface but optimized/stripped most non-essential processing
* to emphasize speed
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <allegro.h>
#include "apeg.h"

#include "mpeg1dec.h"

/* private prototypes */
#define DECLARE_RENDERER(bpp)	\
    static void Dither_Frame420_##bpp(APEG_LAYER *layer, unsigned char **src); \
    static void Dither_Frame422_##bpp(APEG_LAYER *layer, unsigned char **src); \
    static void Dither_Frame444_##bpp(APEG_LAYER *layer, unsigned char **src)

DECLARE_RENDERER(8);
DECLARE_RENDERER(16);
DECLARE_RENDERER(24);
DECLARE_RENDERER(32);


static int (*Init_Display_Ex)(APEG_STREAM *stream, int coded_w, int coded_h,
                              void *arg);
static void (*Dither_Frame_Ex)(APEG_STREAM *stream, unsigned char **src,
                               void *arg);
static void *Display_Arg;

// Paletted converters
static int Y_Table8[256+16];	// Resizes and masks the Y component
static int Cb_Table8[256+16];	// Resizes and masks the U(Cb) component
static int Cr_Table8[256+16];	// Resizes and masks the V(Cr) component

// Helper Hi/True Color conversion tables
static int Y_Table[256];
static int Cr_Table1[256];
static int Cb_Table1[256];
static int Cr_Table2[256];
static int Cb_Table2[256];

// RGB conversion tables
static int R_Table[256+16];	// Resizes and masks the R component
static int G_Table[256+16];	// Resizes and masks the G component
static int B_Table[256+16];	// Resizes and masks the B component

// Local options
static int override_depth = -1;	// User specified color depth

// Global palette
PALETTE apeg_palette;


void apeg_set_display_depth(int depth)
{
    override_depth = depth;
}


/* 4x4 ordered dither
*
* dither pattern:
*  -8  0 -6  2
*   4 -4  6 -2
*  -5  3 -7  1
*   7 -1  5 -3
*/
void apeg_reset_colors(APEG_STREAM *stream)
{
    int depth;
    int i, v;

    if(!stream->bitmap)
        return;

    depth = bitmap_color_depth(stream->bitmap);
    if(depth == 8)
    {
        set_palette(apeg_palette);

        for(i = 0;i < 256+16;i++)
        {
            // Extract YUV components and adjust for dithering
            v = clamp_val(0, (i-8)>>4, 15);
            Y_Table8[i] = v<<4;

            v = clamp_val(0, ((i-8)-64)>>5, 3);
            Cb_Table8[i] = v<<2;
            Cr_Table8[i] = v;
        }
    }
    else if(depth < 24)
    {
        for(i = 0;i < 256+16;i++)
        {
            // Create RGB components and adjust for dithering
            v = clamp_val(0, i-8, 255);

            R_Table[i] = makeacol_depth(depth, v, 0, 0, 255);
            G_Table[i] = makeacol_depth(depth, 0, v, 0, 255);
            B_Table[i] = makeacol_depth(depth, 0, 0, v, 255);
        }
    }
}


void apeg_set_display_callbacks(int (*init)(APEG_STREAM *stream, int coded_w, int coded_h, void *arg),
                                void (*decode)(APEG_STREAM *stream, unsigned char **src, void *arg),
                                void *arg)
{
    Init_Display_Ex = init;
    Dither_Frame_Ex = decode;
    Display_Arg = arg;
}

/*float crv = 104597.0f/65536.0f;
float cbu = 132201.0f/65536.0f;
float cgu = 25675.0f/65536.0f;
float cgv = 53279.0f/65536.0f;*/
float colorspace_coeffs[3][4] = {
    { 1.596, 2.018, 0.391, 0.813 }, /* Approximate (Theora CS Unspecified)? */
    { 1.596, 2.018, 0.391, 0.813 }, /* SMPTE 170M (NTSC;MPEG-1) */
    // TODO: Get PAL coefficients!
    { 1.596, 2.018, 0.391, 0.813 }
};

/* setup the conversion tables and the new palette. Also create the
bitmap buffer
*/
void apeg_initialize_display(APEG_LAYER *layer, int cs)
{
    float crv, cbu, cgu, cgv;
    int depth, i;

    if(Init_Display_Ex && Dither_Frame_Ex)
    {
        int ret;

        ret = Init_Display_Ex((APEG_STREAM*)layer, layer->coded_width,
            layer->coded_height, Display_Arg);
        if(ret == 0)
        {
            layer->display_frame = Dither_Frame_Ex;
            layer->display_arg = Display_Arg;
            return;
        }
        if(ret < 0)
            apeg_error_jump(NULL);
        layer->display_frame = NULL;
    }

    // matrix coefficients
    crv = colorspace_coeffs[cs][0];
    cbu = colorspace_coeffs[cs][1];
    cgu = colorspace_coeffs[cs][2];
    cgv = colorspace_coeffs[cs][3];

    /* color allocation:
    *
    * translation to hi/true-color RGB is done by color look-up
    * tables, using the algorithm:
    *
    *  R = 1.164*(Y - 16)                 + crv*(V - 128)
    *  G = 1.164*(Y - 16) - cgu*(U - 128) - cgv*(V - 128)
    *  B = 1.164*(Y - 16) + cbu*(U - 128)
    */
#define GET_R(y, cr)		clamp_val(0, (Y_Table[(y)]+Cr_Table1[(cr)]) >> 16, 255)
#define GET_G(y, cb, cr)	clamp_val(0, (Y_Table[(y)]-Cb_Table1[(cb)]-Cr_Table2[(cr)]) >> 16, 255)
#define GET_B(y, cb)		clamp_val(0, (Y_Table[(y)]+Cb_Table2[(cb)]) >> 16, 255)
    for(i = 0;i < 256;i++)
    {
        Y_Table[i] = (((i-16) * 255.0 / 219.0) + 0.5) * 65536.0;

        Cr_Table1[i] = (i-128) * crv * 65536.0;
        Cb_Table2[i] = (i-128) * cbu * 65536.0;
        Cb_Table1[i] = (i-128) * cgu * 65536.0;
        Cr_Table2[i] = (i-128) * cgv * 65536.0;
    }

    /* palette allocation:
    * i is the (internal) 8 bit color number, it consists of separate
    * bit fields for Y, U and V: i = (yyyyuuvv)
    *
    * the colors correspond to the following Y, U and V values:
    * Y:   8, 24, 40, 56, 72, 88, 104, 120, 136, 152, 168, 184, 200,
    *      216, 232, 248
    * U,V: -48, -16, 16, 48
    *
    * U and V values span only about half the color space; this gives
    * usually much better quality, although highly saturated colors can
    * not be displayed properly
    *
    * translation to R,G,B is implicitly done by the palette
    */
    for(i = 0;i < 256;++i)
    {
        // color extraction
        // +8 to push Y into midrange
        // +80 to push Cr/Cb into midrange
        int Y  = 16*((i>>4)&15) + 8;
        int Cb = 32*((i>>2)&3) + 80;
        int Cr = 32*(i&3) + 80;

        apeg_palette[i].r = GET_R(Y, Cr) >> 2;
        apeg_palette[i].g = GET_G(Y, Cb, Cr) >> 2;
        apeg_palette[i].b = GET_B(Y, Cb) >> 2;
    }

    // Set the color depth, and make sure it's valid
    if(override_depth > 0)
        depth = override_depth;
    else if(screen != NULL)
        depth = bitmap_color_depth(screen);
    else
        depth = 32;

    switch(depth)
    {
    case 8:
    case 15:
    case 16:
    case 24:
    case 32:
        break;

    default:
        sprintf(apeg_error, "Unsupported color depth (%ibpp)", depth);
        apeg_error_jump(NULL);
    }

    if(layer->stream.bitmap)
        destroy_bitmap(layer->stream.bitmap);

    // Build the internal bitmap
    layer->stream.bitmap = create_bitmap_ex(depth, layer->coded_width,
        layer->coded_height);
    if(!layer->stream.bitmap)
        apeg_error_jump("Couldn't create internal bitmap");

    // Initialize color placement tables
    apeg_reset_colors((APEG_STREAM*)layer);

    clear_to_color(layer->stream.bitmap, makecol_depth(depth, 0, 0, 0));
    layer->stream.frame_updated = -1;

    // The coded image may be larger than the actual displayed image.
    // Set the clip to the stream size
    set_clip_rect(layer->stream.bitmap, 0, 0, layer->stream.w-1,
        layer->stream.h-1);
}


// Function selector for different pixel formats
#define PICK_RENDERER(fmt, lyr, src) switch(bitmap_color_depth(lyr->stream.bitmap))	\
{	\
    case 8:	\
    Dither_Frame##fmt##_8(lyr, src);	\
    break;	\
    case 32:	\
    Dither_Frame##fmt##_32(lyr, src);	\
    break;	\
    case 16:	\
    case 15:	\
    Dither_Frame##fmt##_16(lyr, src);	\
    break;	\
    case 24:	\
    Dither_Frame##fmt##_24(lyr, src);	\
    break;	\
}

void apeg_display_frame(APEG_LAYER *layer, unsigned char **src)
{
    layer->stream.frame_updated = 1;

    if(layer->display_frame)
    {
        layer->display_frame((APEG_STREAM*)layer, src, layer->display_arg);
        return;
    }

    switch(layer->stream.pixel_format)
    {
    case APEG_420:
        PICK_RENDERER(420, layer, src);
        break;
    case APEG_422:
        PICK_RENDERER(422, layer, src);
        break;
    case APEG_444:
        PICK_RENDERER(444, layer, src);
        break;
    }
}


// Macro to create the rendering functions
#define MAKE_RENDERERS(bpp, bpp_type)	\
    static void Dither_Frame420_##bpp (APEG_LAYER *layer, unsigned char **src)	\
{	\
    const unsigned char *py = src[0];	\
    const unsigned char *pu = src[1];	\
    const unsigned char *pv = src[2];	\
    const unsigned int width  = layer->coded_width >> 1;	\
    const unsigned int height = layer->stream.h;	\
    unsigned int i, j;	\
    bpp_type *dst;	\
    j = 0;	\
    do {	\
    dst = (bpp_type*)(layer->stream.bitmap->line[j++]);	\
    i = 0;	\
    do {	\
    write1(0);  write2(8); write1(2); write2(10);	\
    } while((i+=2) < width);	\
    pu -= i;	\
    pv -= i;	\
    \
    dst = (bpp_type*)(layer->stream.bitmap->line[j++]);	\
    i = 0;	\
    do {	\
    write1(12); write2(4); write1(14); write2(6);	\
    } while((i+=2) < width);	\
    \
    dst = (bpp_type*)(layer->stream.bitmap->line[j++]);	\
    i = 0;	\
    do {	\
    write1(3); write2(11); write1(1);  write2(9);	\
    } while((i+=2) < width);	\
    pu -= i;	\
    pv -= i;	\
    \
    dst = (bpp_type*)(layer->stream.bitmap->line[j++]);	\
    i = 0;	\
    do {	\
    write1(15); write2(7); write1(13); write2(5);	\
    } while((i+=2) < width);	\
    } while(j < height);	\
}	\
    \
    static void Dither_Frame422_##bpp (APEG_LAYER *layer, unsigned char **src)	\
{	\
    const unsigned char *py = src[0];	\
    const unsigned char *pu = src[1];	\
    const unsigned char *pv = src[2];	\
    const unsigned int width  = layer->coded_width;	\
    const unsigned int height = layer->stream.h;	\
    unsigned int i, j;	\
    bpp_type *dst;	\
    j = 0;	\
    do {	\
    dst = (bpp_type*)(layer->stream.bitmap->line[j++]);	\
    i = 0;	\
    do {	\
    write1(0);  write2(8); write1(2); write2(10);	\
    } while((i+=4) < width);	\
    \
    dst = (bpp_type*)(layer->stream.bitmap->line[j++]);	\
    i = 0;	\
    do {	\
    write1(12); write2(4); write1(14); write2(6);	\
    } while((i+=4) < width);	\
    \
    dst = (bpp_type*)(layer->stream.bitmap->line[j++]);	\
    i = 0;	\
    do {	\
    write1(3); write2(11); write1(1);  write2(9);	\
    } while((i+=4) < width);	\
    \
    dst = (bpp_type*)(layer->stream.bitmap->line[j++]);	\
    i = 0;	\
    do {	\
    write1(15); write2(7); write1(13); write2(5);	\
    } while((i+=4) < width);	\
    } while(j < height);	\
}	\
    \
    static void Dither_Frame444_##bpp (APEG_LAYER *layer, unsigned char **src)	\
{	\
    const unsigned char *py = src[0];	\
    const unsigned char *pu = src[1];	\
    const unsigned char *pv = src[2];	\
    const unsigned int width  = layer->stream.w;	\
    const unsigned int height = layer->stream.h;	\
    unsigned int i, j;	\
    bpp_type *dst;	\
    j = 0;	\
    do {	\
    dst = (bpp_type*)(layer->stream.bitmap->line[j++]);	\
    i = 0;	\
    do {	\
    write2(0);  write2(8); write2(2); write2(10);	\
    } while((i+=4) < width);	\
    \
    dst = (bpp_type*)(layer->stream.bitmap->line[j++]);	\
    i = 0;	\
    do {	\
    write2(12); write2(4); write2(14); write2(6);	\
    } while((i+=4) < width);	\
    \
    dst = (bpp_type*)(layer->stream.bitmap->line[j++]);	\
    i = 0;	\
    do {	\
    write2(3); write2(11); write2(1);  write2(9);	\
    } while((i+=4) < width);	\
    \
    dst = (bpp_type*)(layer->stream.bitmap->line[j++]);	\
    i = 0;	\
    do {	\
    write2(15); write2(7); write2(13); write2(5);	\
    } while((i+=4) < width);	\
    } while(j < height);	\
}


#define write1(x)	\
    *dst = Y_Table8[(*py)+(x)] | Cb_Table8[(*pu)+(x)] | Cr_Table8[(*pv)+(x)];	\
    ++dst; ++py
#define write2(x)	\
    *dst = Y_Table8[(*py)+(x)] | Cb_Table8[(*pu)+(x)] | Cr_Table8[(*pv)+(x)];	\
    ++dst; ++py; ++pu; ++pv

MAKE_RENDERERS(8, uint8_t)

#undef write2
#undef write1


#define write1(x)	\
    *dst = R_Table[GET_R(*py, *pv)+(x)] | G_Table[GET_G(*py, *pu, *pv)+(x)] | B_Table[GET_B(*py, *pu)+(x)];	\
    ++dst; ++py
#define write2(x)	\
    *dst = R_Table[GET_R(*py, *pv)+(x)] | G_Table[GET_G(*py, *pu, *pv)+(x)] | B_Table[GET_B(*py, *pu)+(x)];	\
    ++dst; ++py; ++pu; ++pv

MAKE_RENDERERS(16, uint16_t)

#undef write2
#undef write1


#define write1(x)	\
    *dst = makeacol32(GET_R(*py, *pv), GET_G(*py, *pu, *pv), GET_B(*py, *pu), 255);	\
    ++dst; ++py
#define write2(x)	\
    *dst = makeacol32(GET_R(*py, *pv), GET_G(*py, *pu, *pv), GET_B(*py, *pu), 255);	\
    ++dst; ++py; ++pu; ++pv

MAKE_RENDERERS(32, uint32_t)

#undef write2
#undef write1

// Ugly hacks for 24-bit, but it works...
static int tmp;
#define write1(x)	\
    tmp = makecol24(GET_R(*py, *pv), GET_G(*py, *pu, *pv), GET_B(*py, *pu));	\
    memcpy(dst, &tmp, 3);	\
    dst += 3; ++py
#define write2(x)	\
    tmp = makecol24(GET_R(*py, *pv), GET_G(*py, *pu, *pv), GET_B(*py, *pu));	\
    memcpy(dst, &tmp, 3);	\
    dst += 3; ++py; ++pu; ++pv

MAKE_RENDERERS(24, uint8_t)

#undef write2
#undef write1
