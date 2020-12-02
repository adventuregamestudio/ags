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
 *      Graphics routines: palette fading, circles, etc.
 *
 *      By Shawn Hargreaves.
 *
 *      Optimised line drawer by Michael Bukin.
 *
 *      Bresenham arc routine by Romano Signorelli.
 *
 *      Cohen-Sutherland line clipping by Jon Rafkind.
 *
 *      See readme.txt for copyright information.
 */


#include <math.h>
#include "allegro.h"
#include "allegro/internal/aintern.h"



/* drawing_mode:
 *  Sets the drawing mode. This only affects routines like putpixel,
 *  lines, rectangles, triangles, etc, not the blitting or sprite
 *  drawing functions.
 */
void drawing_mode(int mode, BITMAP *pattern, int x_anchor, int y_anchor)
{
   _drawing_mode = mode;
   _drawing_pattern = pattern;
   _drawing_x_anchor = x_anchor;
   _drawing_y_anchor = y_anchor;

   if (pattern) {
      _drawing_x_mask = 1; 
      while (_drawing_x_mask < (unsigned)pattern->w)
	 _drawing_x_mask <<= 1;        /* find power of two greater than w */

      if (_drawing_x_mask > (unsigned)pattern->w) {
	 ASSERT(FALSE);
	 _drawing_x_mask >>= 1;        /* round down if required */
      }

      _drawing_x_mask--;               /* convert to AND mask */

      _drawing_y_mask = 1;
      while (_drawing_y_mask < (unsigned)pattern->h)
	 _drawing_y_mask <<= 1;        /* find power of two greater than h */

      if (_drawing_y_mask > (unsigned)pattern->h) {
	 ASSERT(FALSE);
	 _drawing_y_mask >>= 1;        /* round down if required */
      }

      _drawing_y_mask--;               /* convert to AND mask */
   }
   else
      _drawing_x_mask = _drawing_y_mask = 0;
}



/* set_blender_mode:
 *  Specifies a custom set of blender functions for interpolating between
 *  truecolor pixels. The 24 bit blender is shared between the 24 and 32 bit
 *  modes. Pass a NULL table for unused color depths (you must not draw
 *  translucent graphics in modes without a handler, though!). Your blender
 *  will be passed two 32 bit colors in the appropriate format (5.5.5, 5.6.5,
 *  or 8.8.8), and an alpha value, should return the result of combining them.
 *  In translucent drawing modes, the two colors are taken from the source
 *  and destination images and the alpha is specified by this function. In
 *  lit modes, the alpha is specified when you call the drawing routine, and
 *  the interpolation is between the source color and the RGB values you pass
 *  to this function.
 */
void set_blender_mode(BLENDER_FUNC b15, BLENDER_FUNC b16, BLENDER_FUNC b24, int r, int g, int b, int a)
{
   _blender_func15 = b15;
   _blender_func16 = b16;
   _blender_func24 = b24;
   _blender_func32 = b24;

   _blender_func15x = _blender_black;
   _blender_func16x = _blender_black;
   _blender_func24x = _blender_black;

   _blender_col_15 = makecol15(r, g, b);
   _blender_col_16 = makecol16(r, g, b);
   _blender_col_24 = makecol24(r, g, b);
   _blender_col_32 = makecol32(r, g, b);

   _blender_alpha = a;
}



/* set_blender_mode_ex
 *  Specifies a custom set of blender functions for interpolating between
 *  truecolor pixels, providing a more complete set of routines, which
 *  differentiate between 24 and 32 bit modes, and have special routines
 *  for blending 32 bit RGBA pixels onto a destination of any format.
 */
void set_blender_mode_ex(BLENDER_FUNC b15, BLENDER_FUNC b16, BLENDER_FUNC b24, BLENDER_FUNC b32, BLENDER_FUNC b15x, BLENDER_FUNC b16x, BLENDER_FUNC b24x, int r, int g, int b, int a)
{
   _blender_func15 = b15;
   _blender_func16 = b16;
   _blender_func24 = b24;
   _blender_func32 = b32;

   _blender_func15x = b15x;
   _blender_func16x = b16x;
   _blender_func24x = b24x;

   _blender_col_15 = makecol15(r, g, b);
   _blender_col_16 = makecol16(r, g, b);
   _blender_col_24 = makecol24(r, g, b);
   _blender_col_32 = makecol32(r, g, b);

   _blender_alpha = a;
}



/* xor_mode:
 *  Shortcut function for toggling XOR mode on and off.
 */
void xor_mode(int on)
{
   drawing_mode(on ? DRAW_MODE_XOR : DRAW_MODE_SOLID, NULL, 0, 0);
}



/* solid_mode:
 *  Shortcut function for selecting solid drawing mode.
 */
void solid_mode(void)
{
   drawing_mode(DRAW_MODE_SOLID, NULL, 0, 0);
}



/* clear_bitmap:
 *  Clears the bitmap to color 0.
 */
void clear_bitmap(BITMAP *bitmap)
{
   clear_to_color(bitmap, 0);
}



/* _bitmap_has_alpha:
 *  Checks whether this bitmap has an alpha channel.
 */
int _bitmap_has_alpha(BITMAP *bmp)
{
   int x, y, c;

   if (bitmap_color_depth(bmp) != 32)
      return FALSE;

   for (y = 0; y < bmp->h; y++) {
      for (x = 0; x < bmp->w; x++) {
	 c = getpixel(bmp, x, y);
	 if (geta32(c))
	    return TRUE;
      }
   }

   return FALSE;
}



/* set_color:
 *  Sets a single palette entry.
 */
void set_color(int index, AL_CONST RGB *p)
{
   ASSERT(index >= 0 && index < PAL_SIZE);
   set_palette_range((struct RGB *)p-index, index, index, FALSE);
}



/* set_palette:
 *  Sets the entire color palette.
 */
void set_palette(AL_CONST PALETTE p)
{
   set_palette_range(p, 0, PAL_SIZE-1, TRUE);
}



/* set_palette_range:
 *  Sets a part of the color palette.
 */
void set_palette_range(AL_CONST PALETTE p, int from, int to, int vsync)
{
   int c;

   ASSERT(from >= 0 && from < PAL_SIZE);
   ASSERT(to >= 0 && to < PAL_SIZE)

   for (c=from; c<=to; c++) {
      _current_palette[c] = p[c];

      if (_color_depth != 8)
	 palette_color[c] = makecol(_rgb_scale_6[p[c].r], _rgb_scale_6[p[c].g], _rgb_scale_6[p[c].b]);
   }

   _current_palette_changed = 0xFFFFFFFF & ~(1<<(_color_depth-1));
}



/* previous palette, so the image loaders can restore it when they are done */
int _got_prev_current_palette = FALSE;
PALETTE _prev_current_palette;
static int prev_palette_color[PAL_SIZE];



/* select_palette:
 *  Sets the aspects of the palette tables that are used for converting
 *  between different image formats, without altering the display settings.
 *  The previous settings are copied onto a one-deep stack, from where they
 *  can be restored by calling unselect_palette().
 */
void select_palette(AL_CONST PALETTE p)
{
   int c;

   for (c=0; c<PAL_SIZE; c++) {
      _prev_current_palette[c] = _current_palette[c];
      _current_palette[c] = p[c];
   }

   if (_color_depth != 8) {
      for (c=0; c<PAL_SIZE; c++) {
	 prev_palette_color[c] = palette_color[c];
	 palette_color[c] = makecol(_rgb_scale_6[p[c].r], _rgb_scale_6[p[c].g], _rgb_scale_6[p[c].b]);
      }
   }

   _got_prev_current_palette = TRUE;

   _current_palette_changed = 0xFFFFFFFF & ~(1<<(_color_depth-1));
}



/* unselect_palette:
 *  Restores palette settings from before the last call to select_palette().
 */
void unselect_palette(void)
{
   int c;

   for (c=0; c<PAL_SIZE; c++)
      _current_palette[c] = _prev_current_palette[c];

   if (_color_depth != 8) {
      for (c=0; c<PAL_SIZE; c++)
	 palette_color[c] = prev_palette_color[c];
   }

   ASSERT(_got_prev_current_palette == TRUE);
   _got_prev_current_palette = FALSE;

   _current_palette_changed = 0xFFFFFFFF & ~(1<<(_color_depth-1));
}



/* _palette_expansion_table:
 *  Creates a lookup table for expanding 256->truecolor.
 */
static int *palette_expansion_table(int bpp)
{
   int *table;
   int c;

   switch (bpp) {
      case 15: table = _palette_color15; break;
      case 16: table = _palette_color16; break;
      case 24: table = _palette_color24; break;
      case 32: table = _palette_color32; break;
      default: ASSERT(FALSE); return NULL;
   }

   if (_current_palette_changed & (1<<(bpp-1))) {
      for (c=0; c<PAL_SIZE; c++) {
	 table[c] = makecol_depth(bpp,
				  _rgb_scale_6[_current_palette[c].r], 
				  _rgb_scale_6[_current_palette[c].g], 
				  _rgb_scale_6[_current_palette[c].b]);
      }

      _current_palette_changed &= ~(1<<(bpp-1));
   } 

   return table;
}



/* this has to be called through a function pointer, so MSVC asm can use it */
int *(*_palette_expansion_table)(int) = palette_expansion_table;



/* generate_332_palette:
 *  Used when loading a truecolor image into an 8 bit bitmap, to generate
 *  a 3.3.2 RGB palette.
 */
void generate_332_palette(PALETTE pal)
{
   int c;

   for (c=0; c<PAL_SIZE; c++) {
      pal[c].r = ((c>>5)&7) * 63/7;
      pal[c].g = ((c>>2)&7) * 63/7;
      pal[c].b = (c&3) * 63/3;
   }

   pal[0].r = 63;
   pal[0].g = 0;
   pal[0].b = 63;

   pal[254].r = pal[254].g = pal[254].b = 0;
}



/* get_color:
 *  Retrieves a single color from the palette.
 */
void get_color(int index, RGB *p)
{
   ASSERT(index >= 0 && index < PAL_SIZE);
   ASSERT(p);
   get_palette_range(p-index, index, index);
}



/* get_palette:
 *  Retrieves the entire color palette.
 */
void get_palette(PALETTE p)
{
   get_palette_range(p, 0, PAL_SIZE-1);
}



/* get_palette_range:
 *  Retrieves a part of the color palette.
 */
void get_palette_range(PALETTE p, int from, int to)
{
   int c;

   ASSERT(from >= 0 && from < PAL_SIZE);
   ASSERT(to >= 0 && to < PAL_SIZE);

   for (c=from; c<=to; c++)
      p[c] = _current_palette[c];
}



/* fade_interpolate: 
 *  Calculates a palette part way between source and dest, returning it
 *  in output. The pos indicates how far between the two extremes it should
 *  be: 0 = return source, 64 = return dest, 32 = return exactly half way.
 *  Only affects colors between from and to (inclusive).
 */
void fade_interpolate(AL_CONST PALETTE source, AL_CONST PALETTE dest, PALETTE output, int pos, int from, int to)
{
   int c;

   ASSERT(pos >= 0 && pos <= 64);
   ASSERT(from >= 0 && from < PAL_SIZE);
   ASSERT(to >= 0 && to < PAL_SIZE);

   for (c=from; c<=to; c++) { 
      output[c].r = ((int)source[c].r * (63-pos) + (int)dest[c].r * pos) / 64;
      output[c].g = ((int)source[c].g * (63-pos) + (int)dest[c].g * pos) / 64;
      output[c].b = ((int)source[c].b * (63-pos) + (int)dest[c].b * pos) / 64;
   }
}



/* rect:
 *  Draws an outline rectangle.
 */
void _soft_rect(BITMAP *bmp, int x1, int y1, int x2, int y2, int color)
{
   int t;

   if (x2 < x1) {
      t = x1;
      x1 = x2;
      x2 = t;
   }

   if (y2 < y1) {
      t = y1;
      y1 = y2;
      y2 = t;
   }

   acquire_bitmap(bmp);

   hline(bmp, x1, y1, x2, color);

   if (y2 > y1)
      hline(bmp, x1, y2, x2, color);

   if (y2-1 >= y1+1) {
      vline(bmp, x1, y1+1, y2-1, color);

      if (x2 > x1)
	 vline(bmp, x2, y1+1, y2-1, color);
   }

   release_bitmap(bmp);
}



/* _normal_rectfill:
 *  Draws a solid filled rectangle, using hfill() to do the work.
 */
void _normal_rectfill(BITMAP *bmp, int x1, int y1, int x2, int y2, int color)
{
   int t;

   if (y1 > y2) {
      t = y1;
      y1 = y2;
      y2 = t;
   }

   if (bmp->clip) {
      if (x1 > x2) {
	 t = x1;
	 x1 = x2;
	 x2 = t;
      }

      if (x1 < bmp->cl)
	 x1 = bmp->cl;

      if (x2 >= bmp->cr)
	 x2 = bmp->cr-1;

      if (x2 < x1)
	 return;

      if (y1 < bmp->ct)
	 y1 = bmp->ct;

      if (y2 >= bmp->cb)
	 y2 = bmp->cb-1;

      if (y2 < y1)
	 return;

      bmp->clip = FALSE;
      t = TRUE;
   }
   else
      t = FALSE;

   acquire_bitmap(bmp);

   while (y1 <= y2) {
      bmp->vtable->hfill(bmp, x1, y1, x2, color);
      y1++;
   };

   release_bitmap(bmp);

   bmp->clip = t;
}



/* do_line:
 *  Calculates all the points along a line between x1, y1 and x2, y2, 
 *  calling the supplied function for each one. This will be passed a 
 *  copy of the bmp parameter, the x and y position, and a copy of the 
 *  d parameter (so do_line() can be used with putpixel()).
 */
void do_line(BITMAP *bmp, int x1, int y1, int x2, int y2, int d, void (*proc)(BITMAP *, int, int, int))
{
   int dx = x2-x1;
   int dy = y2-y1;
   int i1, i2;
   int x, y;
   int dd;

   /* worker macro */
   #define DO_LINE(pri_sign, pri_c, pri_cond, sec_sign, sec_c, sec_cond)     \
   {                                                                         \
      if (d##pri_c == 0) {                                                   \
	 proc(bmp, x1, y1, d);                                               \
	 return;                                                             \
      }                                                                      \
									     \
      i1 = 2 * d##sec_c;                                                     \
      dd = i1 - (sec_sign (pri_sign d##pri_c));                              \
      i2 = dd - (sec_sign (pri_sign d##pri_c));                              \
									     \
      x = x1;                                                                \
      y = y1;                                                                \
									     \
      while (pri_c pri_cond pri_c##2) {                                      \
	 proc(bmp, x, y, d);                                                 \
									     \
	 if (dd sec_cond 0) {                                                \
	    sec_c = sec_c sec_sign 1;                                        \
	    dd += i2;                                                        \
	 }                                                                   \
	 else                                                                \
	    dd += i1;                                                        \
									     \
	 pri_c = pri_c pri_sign 1;                                           \
      }                                                                      \
   }

   if (dx >= 0) {
      if (dy >= 0) {
	 if (dx >= dy) {
	    /* (x1 <= x2) && (y1 <= y2) && (dx >= dy) */
	    DO_LINE(+, x, <=, +, y, >=);
	 }
	 else {
	    /* (x1 <= x2) && (y1 <= y2) && (dx < dy) */
	    DO_LINE(+, y, <=, +, x, >=);
	 }
      }
      else {
	 if (dx >= -dy) {
	    /* (x1 <= x2) && (y1 > y2) && (dx >= dy) */
	    DO_LINE(+, x, <=, -, y, <=);
	 }
	 else {
	    /* (x1 <= x2) && (y1 > y2) && (dx < dy) */
	    DO_LINE(-, y, >=, +, x, >=);
	 }
      }
   }
   else {
      if (dy >= 0) {
	 if (-dx >= dy) {
	    /* (x1 > x2) && (y1 <= y2) && (dx >= dy) */
	    DO_LINE(-, x, >=, +, y, >=);
	 }
	 else {
	    /* (x1 > x2) && (y1 <= y2) && (dx < dy) */
	    DO_LINE(+, y, <=, -, x, <=);
	 }
      }
      else {
	 if (-dx >= -dy) {
	    /* (x1 > x2) && (y1 > y2) && (dx >= dy) */
	    DO_LINE(-, x, >=, -, y, <=);
	 }
	 else {
	    /* (x1 > x2) && (y1 > y2) && (dx < dy) */
	    DO_LINE(-, y, >=, -, x, <=);
	 }
      }
   }

   #undef DO_LINE
}



/* _normal_line:
 *  Draws a line from x1, y1 to x2, y2, using putpixel() to do the work.
 */
void _normal_line(BITMAP *bmp, int x1, int y1, int x2, int y2, int color)
{
   int sx, sy, dx, dy, t;

   if (x1 == x2) {
      vline(bmp, x1, y1, y2, color);
      return;
   }

   if (y1 == y2) {
      hline(bmp, x1, y1, x2, color);
      return;
   }

   /* use a bounding box to check if the line needs clipping */
   if (bmp->clip) {
      sx = x1;
      sy = y1;
      dx = x2;
      dy = y2;

      if (sx > dx) {
	 t = sx;
	 sx = dx;
	 dx = t;
      }

      if (sy > dy) {
	 t = sy;
	 sy = dy;
	 dy = t;
      }

      if ((sx >= bmp->cr) || (sy >= bmp->cb) || (dx < bmp->cl) || (dy < bmp->ct))
	 return;

      if ((sx >= bmp->cl) && (sy >= bmp->ct) && (dx < bmp->cr) && (dy < bmp->cb))
	 bmp->clip = FALSE;

      t = TRUE;
   }
   else
      t= FALSE;

   acquire_bitmap(bmp);

   do_line(bmp, x1, y1, x2, y2, color, bmp->vtable->putpixel);

   release_bitmap(bmp);

   bmp->clip = t;
}


/* circlefill:
 *  Draws a filled circle.
 */
void _soft_circlefill(BITMAP *bmp, int x, int y, int radius, int color)
{
   int cx = 0;
   int cy = radius;
   int df = 1 - radius; 
   int d_e = 3;
   int d_se = -2 * radius + 5;
   int clip, sx, sy, dx, dy;
   ASSERT(bmp);

   if (bmp->clip) {
      sx = x-radius-1;
      sy = y-radius-1;
      dx = x+radius+1;
      dy = y+radius+1;

      if ((sx >= bmp->cr) || (sy >= bmp->cb) || (dx < bmp->cl) || (dy < bmp->ct))
	 return;

      if ((sx >= bmp->cl) && (sy >= bmp->ct) && (dx < bmp->cr) && (dy < bmp->cb))
	 bmp->clip = FALSE;

      clip = TRUE;
   }
   else
      clip = FALSE;

   acquire_bitmap(bmp);

   do {
      bmp->vtable->hfill(bmp, x-cy, y-cx, x+cy, color);

      if (cx)
	 bmp->vtable->hfill(bmp, x-cy, y+cx, x+cy, color);

      if (df < 0)  {
	 df += d_e;
	 d_e += 2;
	 d_se += 2;
      }
      else { 
	 if (cx != cy) {
	    bmp->vtable->hfill(bmp, x-cx, y-cy, x+cx, color);

	    if (cy)
	       bmp->vtable->hfill(bmp, x-cx, y+cy, x+cx, color);
	 }

	 df += d_se;
	 d_e += 2;
	 d_se += 4;
	 cy--;
      } 

      cx++; 

   } while (cx <= cy);

   release_bitmap(bmp);

   bmp->clip = clip;
}
