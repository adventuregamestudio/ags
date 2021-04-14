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
 *      Sprite drawing functions.
 *
 *      By Michael Bukin.
 *
 *      See readme.txt for copyright information.
 */

#ifndef __bma_cspr_h
#define __bma_cspr_h

/* _linear_draw_sprite:
 *  Draws a sprite onto a linear bitmap at the specified x, y position,
 *  using a masked drawing mode where zero pixels are not output.
 */
void FUNC_LINEAR_DRAW_SPRITE(BITMAP *dst, BITMAP *src, int dx, int dy)
{
   int x, y, w, h;
   int dxbeg, dybeg;
   int sxbeg, sybeg;

   ASSERT(dst);
   ASSERT(src);

   if (dst->clip) {
      int tmp;

      tmp = dst->cl - dx;
      sxbeg = ((tmp < 0) ? 0 : tmp);
      dxbeg = sxbeg + dx;

      tmp = dst->cr - dx;
      w = ((tmp > src->w) ? src->w : tmp) - sxbeg;
      if (w <= 0)
	 return;

      tmp = dst->ct - dy;
      sybeg = ((tmp < 0) ? 0 : tmp);
      dybeg = sybeg + dy;

      tmp = dst->cb - dy;
      h = ((tmp > src->h) ? src->h : tmp) - sybeg;
      if (h <= 0)
	 return;
   }
   else {
      w = src->w;
      h = src->h;
      sxbeg = 0;
      sybeg = 0;
      dxbeg = dx;
      dybeg = dy;
   }

   {
      for (y = 0; y < h; y++) {
	 PIXEL_PTR s = OFFSET_PIXEL_PTR(src->line[sybeg + y], sxbeg);
	 PIXEL_PTR d = OFFSET_PIXEL_PTR(dst->line[dybeg + y], dxbeg);

	 for (x = w - 1; x >= 0; INC_PIXEL_PTR(s), INC_PIXEL_PTR(d), x--) {
	    unsigned long c = GET_MEMORY_PIXEL(s);
	    if (!IS_SPRITE_MASK(src, c)) {
	       PUT_MEMORY_PIXEL(d, c);
	    }
	 }
      }
   }
}

void FUNC_LINEAR_DRAW_SPRITE_END(void) { }



/* _linear_draw_256_sprite:
 *  Draws a 256 coor sprite onto a linear bitmap at the specified x, y
 *  position, using a masked drawing mode where zero pixels are not output.
 */
void FUNC_LINEAR_DRAW_256_SPRITE(BITMAP *dst, BITMAP *src, int dx, int dy)
{
   int x, y, w, h;
   int dxbeg, dybeg;
   int sxbeg, sybeg;
   int *table;

   ASSERT(dst);
   ASSERT(src);

   if (dst->clip) {
      int tmp;

      tmp = dst->cl - dx;
      sxbeg = ((tmp < 0) ? 0 : tmp);
      dxbeg = sxbeg + dx;

      tmp = dst->cr - dx;
      w = ((tmp > src->w) ? src->w : tmp) - sxbeg;
      if (w <= 0)
	 return;

      tmp = dst->ct - dy;
      sybeg = ((tmp < 0) ? 0 : tmp);
      dybeg = sybeg + dy;

      tmp = dst->cb - dy;
      h = ((tmp > src->h) ? src->h : tmp) - sybeg;
      if (h <= 0)
	 return;
   }
   else {
      w = src->w;
      h = src->h;
      sxbeg = 0;
      sybeg = 0;
      dxbeg = dx;
      dybeg = dy;
   }

   table = _palette_expansion_table(bitmap_color_depth(dst));
   ASSERT(table);

   {
      for (y = 0; y < h; y++) {
	 unsigned char *s = src->line[sybeg + y] + sxbeg;
	 PIXEL_PTR d = OFFSET_PIXEL_PTR(dst->line[dybeg + y], dxbeg);

	 for (x = w - 1; x >= 0; s++, INC_PIXEL_PTR(d), x--) {
	    unsigned long c = *s;
	    if (c != 0) {
	       c = table[c];
	       PUT_MEMORY_PIXEL(d, c);
	    }
	 }
      }
   }
}



/* _linear_draw_sprite_v_flip:
 *  Draws a sprite to a linear bitmap, flipping vertically.
 */
void FUNC_LINEAR_DRAW_SPRITE_V_FLIP(BITMAP *dst, BITMAP *src, int dx, int dy)
{
   int x, y, w, h;
   int dxbeg, dybeg;
   int sxbeg, sybeg;

   ASSERT(dst);
   ASSERT(src);

   if (dst->clip) {
      int tmp;

      tmp = dst->cl - dx;
      sxbeg = ((tmp < 0) ? 0 : tmp);
      dxbeg = sxbeg + dx;

      tmp = dst->cr - dx;
      w = ((tmp > src->w) ? src->w : tmp) - sxbeg;
      if (w <= 0)
	 return;

      tmp = dst->ct - dy;
      sybeg = ((tmp < 0) ? 0 : tmp);
      dybeg = sybeg + dy;

      tmp = dst->cb - dy;
      h = ((tmp > src->h) ? src->h : tmp) - sybeg;
      if (h <= 0)
	 return;

      /* use backward drawing onto dst */
      sybeg = src->h - (sybeg + h);
      dybeg += h - 1;
   }
   else {
      w = src->w;
      h = src->h;
      sxbeg = 0;
      sybeg = 0;
      dxbeg = dx;
      dybeg = dy + h - 1;
   }

   {
      for (y = 0; y < h; y++) {
	 PIXEL_PTR s = OFFSET_PIXEL_PTR(src->line[sybeg + y], sxbeg);
	 PIXEL_PTR d = OFFSET_PIXEL_PTR(dst->line[dybeg - y], dxbeg);

	 for (x = w - 1; x >= 0; INC_PIXEL_PTR(s), INC_PIXEL_PTR(d), x--) {
	    unsigned long c = GET_MEMORY_PIXEL(s);
	    if (!IS_SPRITE_MASK(src, c)) {
	       PUT_MEMORY_PIXEL(d, c);
	    }
	 }
      }
   }
}



/* _linear_draw_sprite_h_flip:
 *  Draws a sprite to a linear bitmap, flipping horizontally.
 */
void FUNC_LINEAR_DRAW_SPRITE_H_FLIP(BITMAP *dst, BITMAP *src, int dx, int dy)
{
   int x, y, w, h;
   int dxbeg, dybeg;
   int sxbeg, sybeg;

   ASSERT(dst);
   ASSERT(src);

   if (dst->clip) {
      int tmp;

      tmp = dst->cl - dx;
      sxbeg = ((tmp < 0) ? 0 : tmp);
      dxbeg = sxbeg + dx;

      tmp = dst->cr - dx;
      w = ((tmp > src->w) ? src->w : tmp) - sxbeg;
      if (w <= 0)
	 return;

      /* use backward drawing onto dst */
      sxbeg = src->w - (sxbeg + w);
      dxbeg += w - 1;

      tmp = dst->ct - dy;
      sybeg = ((tmp < 0) ? 0 : tmp);
      dybeg = sybeg + dy;

      tmp = dst->cb - dy;
      h = ((tmp > src->h) ? src->h : tmp) - sybeg;
      if (h <= 0)
	 return;
   }
   else {
      w = src->w;
      h = src->h;
      sxbeg = 0;
      sybeg = 0;
      dxbeg = dx + w - 1;
      dybeg = dy;
   }

   {
      for (y = 0; y < h; y++) {
	 PIXEL_PTR s = OFFSET_PIXEL_PTR(src->line[sybeg + y], sxbeg);
	 PIXEL_PTR d = OFFSET_PIXEL_PTR(dst->line[dybeg + y], dxbeg);

	 for (x = w - 1; x >= 0; INC_PIXEL_PTR(s), DEC_PIXEL_PTR(d), x--) {
	    unsigned long c = GET_MEMORY_PIXEL(s);
	    if (!IS_SPRITE_MASK(src, c)) {
	       PUT_MEMORY_PIXEL(d, c);
	    }
	 }
      }
   }
}



/* _linear_draw_sprite_vh_flip:
 *  Draws a sprite to a linear bitmap, flipping both vertically and horizontally.
 */
void FUNC_LINEAR_DRAW_SPRITE_VH_FLIP(BITMAP *dst, BITMAP *src, int dx, int dy)
{
   int x, y, w, h;
   int dxbeg, dybeg;
   int sxbeg, sybeg;

   ASSERT(dst);
   ASSERT(src);

   if (dst->clip) {
      int tmp;

      tmp = dst->cl - dx;
      sxbeg = ((tmp < 0) ? 0 : tmp);
      dxbeg = sxbeg + dx;

      tmp = dst->cr - dx;
      w = ((tmp > src->w) ? src->w : tmp) - sxbeg;
      if (w <= 0)
	 return;

      /* use backward drawing onto dst */
      sxbeg = src->w - (sxbeg + w);
      dxbeg += w - 1;

      tmp = dst->ct - dy;
      sybeg = ((tmp < 0) ? 0 : tmp);
      dybeg = sybeg + dy;

      tmp = dst->cb - dy;
      h = ((tmp > src->h) ? src->h : tmp) - sybeg;
      if (h <= 0)
	 return;

      /* use backward drawing onto dst */
      sybeg = src->h - (sybeg + h);
      dybeg += h - 1;
   }
   else {
      w = src->w;
      h = src->h;
      sxbeg = 0;
      sybeg = 0;
      dxbeg = dx + w - 1;
      dybeg = dy + h - 1;
   }

   {
      for (y = 0; y < h; y++) {
	 PIXEL_PTR s = OFFSET_PIXEL_PTR(src->line[sybeg + y], sxbeg);
	 PIXEL_PTR d = OFFSET_PIXEL_PTR(dst->line[dybeg - y], dxbeg);

	 for (x = w - 1; x >= 0; INC_PIXEL_PTR(s), DEC_PIXEL_PTR(d), x--) {
	    unsigned long c = GET_MEMORY_PIXEL(s);
	    if (!IS_SPRITE_MASK(src, c)) {
	       PUT_MEMORY_PIXEL(d, c);
	    }
	 }
      }
   }
}



/* _linear_draw_trans_sprite:
 *  Draws a translucent sprite onto a linear bitmap.
 */
void FUNC_LINEAR_DRAW_TRANS_SPRITE(BITMAP *dst, BITMAP *src, int dx, int dy)
{
   int x, y, w, h;
   int dxbeg, dybeg;
   int sxbeg, sybeg;
   DTS_BLENDER blender;

   ASSERT(dst);
   ASSERT(src);

   if (dst->clip) {
      int tmp;

      tmp = dst->cl - dx;
      sxbeg = ((tmp < 0) ? 0 : tmp);
      dxbeg = sxbeg + dx;

      tmp = dst->cr - dx;
      w = ((tmp > src->w) ? src->w : tmp) - sxbeg;
      if (w <= 0)
	 return;

      tmp = dst->ct - dy;
      sybeg = ((tmp < 0) ? 0 : tmp);
      dybeg = sybeg + dy;

      tmp = dst->cb - dy;
      h = ((tmp > src->h) ? src->h : tmp) - sybeg;
      if (h <= 0)
	 return;
   }
   else {
      w = src->w;
      h = src->h;
      sxbeg = 0;
      sybeg = 0;
      dxbeg = dx;
      dybeg = dy;
   }

   blender = MAKE_DTS_BLENDER();

   if ((src->vtable->color_depth == 8) && (dst->vtable->color_depth != 8)) {
      bmp_select(dst);

      for (y = 0; y < h; y++) {
	 unsigned char *s = src->line[sybeg + y] + sxbeg;
	 PIXEL_PTR ds = OFFSET_PIXEL_PTR(bmp_read_line(dst, dybeg + y), dxbeg);
	 PIXEL_PTR dd = OFFSET_PIXEL_PTR(bmp_write_line(dst, dybeg + y), dxbeg);

	 for (x = w - 1; x >= 0; s++, INC_PIXEL_PTR(ds), INC_PIXEL_PTR(dd), x--) {
	    unsigned long c = *s;
#if PP_DEPTH == 8
	    c = DTS_BLEND(blender, GET_PIXEL(ds), c);
	    PUT_PIXEL(dd, c);
#else
            if (!IS_SPRITE_MASK(src, c)) {
	       c = DTS_BLEND(blender, GET_PIXEL(ds), c);
	       PUT_PIXEL(dd, c);
	    }
#endif
	 }
      }

      bmp_unwrite_line(dst);
   }
   else {
      for (y = 0; y < h; y++) {
	 PIXEL_PTR s = OFFSET_PIXEL_PTR(src->line[sybeg + y], sxbeg);
	 PIXEL_PTR d = OFFSET_PIXEL_PTR(dst->line[dybeg + y], dxbeg);

	 for (x = w - 1; x >= 0; INC_PIXEL_PTR(s), INC_PIXEL_PTR(d), x--) {
	    unsigned long c = GET_MEMORY_PIXEL(s);
#if PP_DEPTH == 8
	    c = DTS_BLEND(blender, GET_MEMORY_PIXEL(d), c);
	    PUT_MEMORY_PIXEL(d, c);
#else
	    if (!IS_SPRITE_MASK(src, c)) {
	       c = DTS_BLEND(blender, GET_MEMORY_PIXEL(d), c);
	       PUT_MEMORY_PIXEL(d, c);
	    }
#endif
	 }
      }
   }
}



#if (PP_DEPTH != 8) && (PP_DEPTH != 32)

/* _linear_draw_trans_rgba_sprite:
 *  Draws a translucent RGBA sprite onto a linear bitmap.
 */
void FUNC_LINEAR_DRAW_TRANS_RGBA_SPRITE(BITMAP *dst, BITMAP *src, int dx, int dy)
{
   int x, y, w, h;
   int dxbeg, dybeg;
   int sxbeg, sybeg;
   RGBA_BLENDER blender;

   ASSERT(dst);
   ASSERT(src);

   if (dst->clip) {
      int tmp;

      tmp = dst->cl - dx;
      sxbeg = ((tmp < 0) ? 0 : tmp);
      dxbeg = sxbeg + dx;

      tmp = dst->cr - dx;
      w = ((tmp > src->w) ? src->w : tmp) - sxbeg;
      if (w <= 0)
	 return;

      tmp = dst->ct - dy;
      sybeg = ((tmp < 0) ? 0 : tmp);
      dybeg = sybeg + dy;

      tmp = dst->cb - dy;
      h = ((tmp > src->h) ? src->h : tmp) - sybeg;
      if (h <= 0)
	 return;
   }
   else {
      w = src->w;
      h = src->h;
      sxbeg = 0;
      sybeg = 0;
      dxbeg = dx;
      dybeg = dy;
   }

   blender = MAKE_RGBA_BLENDER();

   bmp_select(dst);

   for (y = 0; y < h; y++) {
      uint32_t *s = (uint32_t *)src->line[sybeg + y] + sxbeg;
      PIXEL_PTR ds = OFFSET_PIXEL_PTR(bmp_read_line(dst, dybeg + y), dxbeg);
      PIXEL_PTR dd = OFFSET_PIXEL_PTR(bmp_write_line(dst, dybeg + y), dxbeg);

      for (x = w - 1; x >= 0; s++, INC_PIXEL_PTR(ds), INC_PIXEL_PTR(dd), x--) {
	 unsigned long c = *s;

	 if (c != MASK_COLOR_32) {
	    c = RGBA_BLEND(blender, GET_PIXEL(ds), c);
	    PUT_PIXEL(dd, c);
	 }
      }
   }

   bmp_unwrite_line(dst);
}

#endif



/* _linear_draw_lit_sprite:
 *  Draws a lit sprite onto a linear bitmap.
 */
void FUNC_LINEAR_DRAW_LIT_SPRITE(BITMAP *dst, BITMAP *src, int dx, int dy, int color)
{
   int x, y, w, h;
   int dxbeg, dybeg;
   int sxbeg, sybeg;
   DLS_BLENDER blender;

   ASSERT(dst);
   ASSERT(src);

   if (dst->clip) {
      int tmp;

      tmp = dst->cl - dx;
      sxbeg = ((tmp < 0) ? 0 : tmp);
      dxbeg = sxbeg + dx;

      tmp = dst->cr - dx;
      w = ((tmp > src->w) ? src->w : tmp) - sxbeg;
      if (w <= 0)
	 return;

      tmp = dst->ct - dy;
      sybeg = ((tmp < 0) ? 0 : tmp);
      dybeg = sybeg + dy;

      tmp = dst->cb - dy;
      h = ((tmp > src->h) ? src->h : tmp) - sybeg;
      if (h <= 0)
	 return;
   }
   else {
      w = src->w;
      h = src->h;
      sxbeg = 0;
      sybeg = 0;
      dxbeg = dx;
      dybeg = dy;
   }

   blender = MAKE_DLS_BLENDER(color);

   {
      for (y = 0; y < h; y++) {
	 PIXEL_PTR s = OFFSET_PIXEL_PTR(src->line[sybeg + y], sxbeg);
	 PIXEL_PTR d = OFFSET_PIXEL_PTR(dst->line[dybeg + y], dxbeg);

	 for (x = w - 1; x >= 0; INC_PIXEL_PTR(s), INC_PIXEL_PTR(d), x--) {
	    unsigned long c = GET_MEMORY_PIXEL(s);

	    if (!IS_MASK(c)) {
	       c = DLS_BLEND(blender, color, c);
	       PUT_MEMORY_PIXEL(d, c);
	    }
	 }
      }
   }
}

#endif /* !__bma_cspr_h */

