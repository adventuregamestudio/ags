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
 *      Top level bitmap reading routines.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include <string.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"


/* _fixup_loaded_bitmap:
 *  Helper function for adjusting the color depth of a loaded image.
 *  Converts the bitmap BMP to the color depth BPP. If BMP is a 8-bit
 *  bitmap, PAL must be the palette attached to the bitmap. If BPP is
 *  equal to 8, the conversion is performed either by building a palette
 *  optimized for the bitmap if PAL is not NULL (in which case PAL gets
 *  filled in with this palette) or by using the current palette if PAL
 *  is NULL. In any other cases, PAL is unused.
 */
BITMAP *_fixup_loaded_bitmap(BITMAP *bmp, PALETTE pal, int bpp)
{
   BITMAP *b2;
   ASSERT(bmp);

   b2 = create_bitmap_ex(bpp, bmp->w, bmp->h);
   if (!b2) {
      destroy_bitmap(bmp);
      return NULL;
   }

   if (bpp == 8) {
      RGB_MAP *old_map = rgb_map;

      if (pal)
	 generate_optimized_palette(bmp, pal, NULL);
      else
	 pal = _current_palette;

      rgb_map = _AL_MALLOC(sizeof(RGB_MAP));
      if (rgb_map != NULL)
	 create_rgb_table(rgb_map, pal, NULL);

      blit(bmp, b2, 0, 0, 0, 0, bmp->w, bmp->h);

      if (rgb_map != NULL)
	 _AL_FREE(rgb_map);
      rgb_map = old_map;
   }
   else if (bitmap_color_depth(bmp) == 8) {
      select_palette(pal);
      blit(bmp, b2, 0, 0, 0, 0, bmp->w, bmp->h);
      unselect_palette();
   }
   else {
      blit(bmp, b2, 0, 0, 0, 0, bmp->w, bmp->h);
   }

   destroy_bitmap(bmp);

   return b2;
}
