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
 *      Basic graphics support routines.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_GFX_H
#define ALLEGRO_GFX_H

#include "base.h"
#include "fixed.h"

#ifdef __cplusplus
   extern "C" {
#endif

struct RLE_SPRITE;
struct FONT_GLYPH;
struct RGB;

/* drawing modes for draw_sprite_ex() */
#define DRAW_SPRITE_NORMAL 0
#define DRAW_SPRITE_LIT 1
#define DRAW_SPRITE_TRANS 2

/* flipping modes for draw_sprite_ex() */
#define DRAW_SPRITE_NO_FLIP 0x0
#define DRAW_SPRITE_H_FLIP  0x1
#define DRAW_SPRITE_V_FLIP  0x2
#define DRAW_SPRITE_VH_FLIP 0x3

/* Blender mode defines, for the gfx_driver->set_blender_mode() function */
#define blender_mode_none            0
#define blender_mode_trans           1
#define blender_mode_add             2
#define blender_mode_burn            3
#define blender_mode_color           4
#define blender_mode_difference      5
#define blender_mode_dissolve        6
#define blender_mode_dodge           7
#define blender_mode_hue             8
#define blender_mode_invert          9
#define blender_mode_luminance      10
#define blender_mode_multiply       11
#define blender_mode_saturation     12
#define blender_mode_screen         13
#define blender_mode_alpha          14


typedef AL_METHOD(uintptr_t, _BMP_BANK_SWITCHER, (struct BITMAP *bmp, int lyne));
typedef AL_METHOD(void, _BMP_UNBANK_SWITCHER, (struct BITMAP *bmp));


typedef struct GFX_VTABLE        /* functions for drawing onto bitmaps */
{
   int color_depth;
   int mask_color;
   _BMP_UNBANK_SWITCHER unwrite_bank;  /* C function on some machines, asm on i386 */
   AL_METHOD(void, set_clip, (struct BITMAP *bmp));
   AL_METHOD(void, acquire, (struct BITMAP *bmp));
   AL_METHOD(void, release, (struct BITMAP *bmp));
   AL_METHOD(struct BITMAP *, create_sub_bitmap, (struct BITMAP *parent, int x, int y, int width, int height));
   AL_METHOD(void, created_sub_bitmap, (struct BITMAP *bmp, struct BITMAP *parent));
   AL_METHOD(int,  getpixel, (struct BITMAP *bmp, int x, int y));
   AL_METHOD(void, putpixel, (struct BITMAP *bmp, int x, int y, int color));
   AL_METHOD(void, vline, (struct BITMAP *bmp, int x, int y_1, int y2, int color));
   AL_METHOD(void, hline, (struct BITMAP *bmp, int x1, int y, int x2, int color));
   AL_METHOD(void, hfill, (struct BITMAP *bmp, int x1, int y, int x2, int color));
   AL_METHOD(void, line, (struct BITMAP *bmp, int x1, int y_1, int x2, int y2, int color));
   AL_METHOD(void, fastline, (struct BITMAP *bmp, int x1, int y_1, int x2, int y2, int color));
   AL_METHOD(void, rectfill, (struct BITMAP *bmp, int x1, int y_1, int x2, int y2, int color));
   AL_METHOD(void, triangle, (struct BITMAP *bmp, int x1, int y_1, int x2, int y2, int x3, int y3, int color));
   AL_METHOD(void, draw_sprite, (struct BITMAP *bmp, struct BITMAP *sprite, int x, int y));
   AL_METHOD(void, draw_256_sprite, (struct BITMAP *bmp, struct BITMAP *sprite, int x, int y));
   AL_METHOD(void, draw_sprite_v_flip, (struct BITMAP *bmp, struct BITMAP *sprite, int x, int y));
   AL_METHOD(void, draw_sprite_h_flip, (struct BITMAP *bmp, struct BITMAP *sprite, int x, int y));
   AL_METHOD(void, draw_sprite_vh_flip, (struct BITMAP *bmp, struct BITMAP *sprite, int x, int y));
   AL_METHOD(void, draw_trans_sprite, (struct BITMAP *bmp, struct BITMAP *sprite, int x, int y));
   AL_METHOD(void, draw_trans_rgba_sprite, (struct BITMAP *bmp, struct BITMAP *sprite, int x, int y));
   AL_METHOD(void, draw_lit_sprite, (struct BITMAP *bmp, struct BITMAP *sprite, int x, int y, int color));
   AL_METHOD(void, draw_rle_sprite, (struct BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y));
   AL_METHOD(void, draw_trans_rle_sprite, (struct BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y));
   AL_METHOD(void, draw_trans_rgba_rle_sprite, (struct BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y));
   AL_METHOD(void, draw_lit_rle_sprite, (struct BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y, int color));
   AL_METHOD(void, draw_character, (struct BITMAP *bmp, struct BITMAP *sprite, int x, int y, int color, int bg));
   AL_METHOD(void, draw_glyph, (struct BITMAP *bmp, AL_CONST struct FONT_GLYPH *glyph, int x, int y, int color, int bg));
   AL_METHOD(void, blit_from_memory, (struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
   AL_METHOD(void, blit_to_memory, (struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
   AL_METHOD(void, blit_from_system, (struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
   AL_METHOD(void, blit_to_system, (struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
   AL_METHOD(void, blit_to_self, (struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
   AL_METHOD(void, blit_to_self_forward, (struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
   AL_METHOD(void, blit_to_self_backward, (struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
   AL_METHOD(void, blit_between_formats, (struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
   AL_METHOD(void, masked_blit, (struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
   AL_METHOD(void, clear_to_color, (struct BITMAP *bitmap, int color));
   AL_METHOD(void, pivot_scaled_sprite_flip, (struct BITMAP *bmp, struct BITMAP *sprite, fixed x, fixed y, fixed cx, fixed cy, fixed angle, fixed scale, int v_flip));
   AL_METHOD(void, do_stretch_blit, (struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int source_width, int source_height, int dest_x, int dest_y, int dest_width, int dest_height, int masked));
   AL_METHOD(void, draw_gouraud_sprite, (struct BITMAP *bmp, struct BITMAP *sprite, int x, int y, int c1, int c2, int c3, int c4));
   AL_METHOD(void, draw_sprite_end, (void));
   AL_METHOD(void, blit_end, (void));
   AL_METHOD(void, polygon, (struct BITMAP *bmp, int vertices, AL_CONST int *points, int color));
   AL_METHOD(void, rect, (struct BITMAP *bmp, int x1, int y_1, int x2, int y2, int color));
   AL_METHOD(void, circle, (struct BITMAP *bmp, int x, int y, int radius, int color));
   AL_METHOD(void, circlefill, (struct BITMAP *bmp, int x, int y, int radius, int color));
   AL_METHOD(void, ellipse, (struct BITMAP *bmp, int x, int y, int rx, int ry, int color));
   AL_METHOD(void, ellipsefill, (struct BITMAP *bmp, int x, int y, int rx, int ry, int color));
   AL_METHOD(void, arc, (struct BITMAP *bmp, int x, int y, fixed ang1, fixed ang2, int r, int color));
   AL_METHOD(void, spline, (struct BITMAP *bmp, AL_CONST int points[8], int color));
   AL_METHOD(void, floodfill, (struct BITMAP *bmp, int x, int y, int color));
   AL_METHOD(void, __reserved1_, (struct BITMAP *bmp));
   AL_METHOD(void, __reserved2_, (struct BITMAP *bmp));
   AL_METHOD(void, __reserved3_, (struct BITMAP *bmp));
   AL_METHOD(void, __reserved4_, (struct BITMAP *bmp));
   AL_METHOD(void, __reserved5_, (struct BITMAP *bmp));
   AL_METHOD(void, __reserved6_, (struct BITMAP *bmp));

   AL_METHOD(void, draw_sprite_ex, (struct BITMAP *bmp, struct BITMAP *sprite, int x, int y, int mode, int flip ));
} GFX_VTABLE;


AL_VAR(GFX_VTABLE, __linear_vtable8);
AL_VAR(GFX_VTABLE, __linear_vtable15);
AL_VAR(GFX_VTABLE, __linear_vtable16);
AL_VAR(GFX_VTABLE, __linear_vtable24);
AL_VAR(GFX_VTABLE, __linear_vtable32);


typedef struct _VTABLE_INFO
{
   int color_depth;
   GFX_VTABLE *vtable;
} _VTABLE_INFO;

AL_ARRAY(_VTABLE_INFO, _vtable_list);


/* macros for constructing the vtable list */
#define BEGIN_COLOR_DEPTH_LIST               \
   _VTABLE_INFO _vtable_list[] =             \
   {

#define END_COLOR_DEPTH_LIST                 \
      {  0,    NULL  }                       \
   };

#define COLOR_DEPTH_8                        \
   {  8,    &__linear_vtable8    },

#define COLOR_DEPTH_15                       \
   {  15,   &__linear_vtable15   },

#define COLOR_DEPTH_16                       \
   {  16,   &__linear_vtable16   },

#define COLOR_DEPTH_24                       \
   {  24,   &__linear_vtable24   },

#define COLOR_DEPTH_32                       \
   {  32,   &__linear_vtable32   },


typedef struct BITMAP            /* a bitmap structure */
{
   int w, h;                     /* width and height in pixels */
   int clip;                     /* flag if clipping is turned on */
   int cl, cr, ct, cb;           /* clip left, right, top and bottom values */
   GFX_VTABLE *vtable;           /* drawing functions */
   _BMP_BANK_SWITCHER write_bank;/* C func on some machines, asm on i386 */
   _BMP_BANK_SWITCHER read_bank; /* C func on some machines, asm on i386 */
   void *dat;                    /* the memory we allocated for the bitmap */
   unsigned long id;             /* for identifying sub-bitmaps */
   void *extra;                  /* points to a structure with more info */
   int x_ofs;                    /* horizontal offset (for sub-bitmaps) */
   int y_ofs;                    /* vertical offset (for sub-bitmaps) */
   int seg;                      /* bitmap segment */
   ZERO_SIZE_ARRAY(unsigned char *, line);
} BITMAP;


#define BMP_ID_SUB         0x20000000
#define BMP_ID_NOBLIT      0x08000000
#define BMP_ID_LOCKED      0x04000000
#define BMP_ID_AUTOLOCK    0x02000000
#define BMP_ID_MASK        0x01FFFFFF

#define COLORCONV_NONE              0

#define COLORCONV_8_TO_15           1
#define COLORCONV_8_TO_16           2
#define COLORCONV_8_TO_24           4
#define COLORCONV_8_TO_32           8

#define COLORCONV_15_TO_8           0x10
#define COLORCONV_15_TO_16          0x20
#define COLORCONV_15_TO_24          0x40
#define COLORCONV_15_TO_32          0x80

#define COLORCONV_16_TO_8           0x100
#define COLORCONV_16_TO_15          0x200
#define COLORCONV_16_TO_24          0x400
#define COLORCONV_16_TO_32          0x800

#define COLORCONV_24_TO_8           0x1000
#define COLORCONV_24_TO_15          0x2000
#define COLORCONV_24_TO_16          0x4000
#define COLORCONV_24_TO_32          0x8000

#define COLORCONV_32_TO_8           0x10000
#define COLORCONV_32_TO_15          0x20000
#define COLORCONV_32_TO_16          0x40000
#define COLORCONV_32_TO_24          0x80000

#define COLORCONV_32A_TO_8          0x100000
#define COLORCONV_32A_TO_15         0x200000
#define COLORCONV_32A_TO_16         0x400000
#define COLORCONV_32A_TO_24         0x800000

#define COLORCONV_DITHER_PAL        0x1000000
#define COLORCONV_DITHER_HI         0x2000000
#define COLORCONV_KEEP_TRANS        0x4000000

#define COLORCONV_DITHER            (COLORCONV_DITHER_PAL |          \
                                     COLORCONV_DITHER_HI)

#define COLORCONV_EXPAND_256        (COLORCONV_8_TO_15 |             \
                                     COLORCONV_8_TO_16 |             \
                                     COLORCONV_8_TO_24 |             \
                                     COLORCONV_8_TO_32)

#define COLORCONV_REDUCE_TO_256     (COLORCONV_15_TO_8 |             \
                                     COLORCONV_16_TO_8 |             \
                                     COLORCONV_24_TO_8 |             \
                                     COLORCONV_32_TO_8 |             \
                                     COLORCONV_32A_TO_8)

#define COLORCONV_EXPAND_15_TO_16    COLORCONV_15_TO_16

#define COLORCONV_REDUCE_16_TO_15    COLORCONV_16_TO_15

#define COLORCONV_EXPAND_HI_TO_TRUE (COLORCONV_15_TO_24 |            \
                                     COLORCONV_15_TO_32 |            \
                                     COLORCONV_16_TO_24 |            \
                                     COLORCONV_16_TO_32)

#define COLORCONV_REDUCE_TRUE_TO_HI (COLORCONV_24_TO_15 |            \
                                     COLORCONV_24_TO_16 |            \
                                     COLORCONV_32_TO_15 |            \
                                     COLORCONV_32_TO_16)

#define COLORCONV_24_EQUALS_32      (COLORCONV_24_TO_32 |            \
                                     COLORCONV_32_TO_24)

#define COLORCONV_TOTAL             (COLORCONV_EXPAND_256 |          \
                                     COLORCONV_REDUCE_TO_256 |       \
                                     COLORCONV_EXPAND_15_TO_16 |     \
                                     COLORCONV_REDUCE_16_TO_15 |     \
                                     COLORCONV_EXPAND_HI_TO_TRUE |   \
                                     COLORCONV_REDUCE_TRUE_TO_HI |   \
                                     COLORCONV_24_EQUALS_32 |        \
                                     COLORCONV_32A_TO_15 |           \
                                     COLORCONV_32A_TO_16 |           \
                                     COLORCONV_32A_TO_24)

#define COLORCONV_PARTIAL           (COLORCONV_EXPAND_15_TO_16 |     \
                                     COLORCONV_REDUCE_16_TO_15 |     \
                                     COLORCONV_24_EQUALS_32)

#define COLORCONV_MOST              (COLORCONV_EXPAND_15_TO_16 |     \
                                     COLORCONV_REDUCE_16_TO_15 |     \
                                     COLORCONV_EXPAND_HI_TO_TRUE |   \
                                     COLORCONV_REDUCE_TRUE_TO_HI |   \
                                     COLORCONV_24_EQUALS_32)

#define COLORCONV_KEEP_ALPHA        (COLORCONV_TOTAL                 \
                                     & ~(COLORCONV_32A_TO_8 |        \
                                         COLORCONV_32A_TO_15 |       \
                                         COLORCONV_32A_TO_16 |       \
                                         COLORCONV_32A_TO_24))

AL_FUNC(void, set_color_depth, (int depth));
AL_FUNC(int, get_color_depth, (void));
AL_FUNC(void, set_color_conversion, (int mode));
AL_FUNC(int, get_color_conversion, (void));
AL_FUNC(BITMAP *, create_bitmap, (int width, int height));
AL_FUNC(BITMAP *, create_bitmap_ex, (int color_depth, int width, int height));
AL_FUNC(BITMAP *, create_sub_bitmap, (BITMAP *parent, int x, int y, int width, int height));
AL_FUNC(void, destroy_bitmap, (BITMAP *bitmap));
AL_FUNC(void, set_clip_rect, (BITMAP *bitmap, int x1, int y_1, int x2, int y2));
AL_FUNC(void, add_clip_rect, (BITMAP *bitmap, int x1, int y_1, int x2, int y2));
AL_FUNC(void, clear_bitmap, (BITMAP *bitmap));

AL_FUNC(BITMAP *, _fixup_loaded_bitmap, (BITMAP *bmp, PALETTE pal, int bpp));

AL_FUNC(void, lock_bitmap, (struct BITMAP *bmp));

#ifdef __cplusplus
   }
#endif

#include "inline/gfx.inl"

#endif          /* ifndef ALLEGRO_GFX_H */


