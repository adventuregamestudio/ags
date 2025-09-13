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
#include "util/wgt2allg.h"
#include "gfx/bitmap.h"

using namespace AGS::Common;

int my_setcolor(const int color, int color_depth, bool fix_alpha)
{
    if (color_depth == 8)
    {
        // indexed color
        return color;
    }
    else if ((color >= 32) && (color_depth > 16))
    {
        // true-color RGB
        return makeacol32(getr16(color), getg16(color), getb16(color), 255);
    }
    else if (color >= 32)
    {
        // 15-bit or 16-bit color RGB
        if (color_depth == 15)
            return (color & 0x001f) | ((color >> 1) & 0x7fe0);
        else
            return color;
    }
    else if (color >= 0)
    {
        // special 0-31 slots, use standard palette regardless of color depth
        int res_color = makecol_depth(color_depth, col_lookups[color] >> 16,
                                  (col_lookups[color] >> 8) & 0x000ff, col_lookups[color] & 0x000ff);

        if (fix_alpha && (color_depth > 16))
            res_color |= 0xff000000; // add alpha for 32-bit color depth
        return res_color;
    }
    else
    {
        assert(false); // invalid input color index
        return 0;
    }
}

  void wsetrgb(int coll, int r, int g, int b, RGB * pall)
  {
    pall[coll].r = r;
    pall[coll].g = g;
    pall[coll].b = b;
  }

  void wcolrotate(unsigned char start, unsigned char finish, int dir, RGB * pall)
  {
    int jj;
    if (dir == 0) {
      // rotate left
        RGB tempp = pall[start];

      for (jj = start; jj < finish; jj++)
        pall[jj] = pall[jj + 1];

      pall[finish] = tempp;
    }
    else {
      // rotate right
        RGB tempp = pall[finish];

      for (jj = finish - 1; jj >= start; jj--)
        pall[jj + 1] = pall[jj];

      pall[start] = tempp;
    }
  }

  Bitmap *wnewblock(Bitmap *src, int x1, int y1, int x2, int y2)
  {
    Bitmap *tempbitm;
    int twid = (x2 - x1) + 1, thit = (y2 - y1) + 1;

    if (twid < 1)
      twid = 1;

    if (thit < 1)
      thit = 1;

    tempbitm = BitmapHelper::CreateBitmap(twid, thit);

    if (tempbitm == nullptr)
      return nullptr;

    tempbitm->Blit(src, x1, y1, 0, 0, tempbitm->GetWidth(), tempbitm->GetHeight());
    return tempbitm;
  }

  void wputblock(Bitmap *ds, int xx, int yy, Bitmap *bll, int xray)
  {
    if (xray)
	  ds->Blit(bll, xx, yy, kBitmap_Transparency);
    else
      ds->Blit(bll, 0, 0, xx, yy, bll->GetWidth(), bll->GetHeight());
  }

  Bitmap wputblock_wrapper; // [IKM] argh! :[
  void wputblock_raw(Bitmap *ds, int xx, int yy, BITMAP *bll, int xray)
  {
	wputblock_wrapper.WrapAllegroBitmap(bll, true);
    if (xray)
      ds->Blit(&wputblock_wrapper, xx, yy, kBitmap_Transparency);
    else
      ds->Blit(&wputblock_wrapper, 0, 0, xx, yy, wputblock_wrapper.GetWidth(), wputblock_wrapper.GetHeight());
  }

  const int col_lookups[32] = {
    0x000000, 0x0000A0, 0x00A000, 0x00A0A0, 0xA00000,   // 4
    0xA000A0, 0xA05000, 0xA0A0A0, 0x505050, 0x5050FF, 0x50FF50, 0x50FFFF,       // 11
    0xFF5050, 0xFF50FF, 0xFFFF50, 0xFFFFFF, 0x000000, 0x101010, 0x202020,       // 18
    0x303030, 0x404040, 0x505050, 0x606060, 0x707070, 0x808080, 0x909090,       // 25
    0xA0A0A0, 0xB0B0B0, 0xC0C0C0, 0xD0D0D0, 0xE0E0E0, 0xF0F0F0
  };

  void wremap(const RGB * pal1, Bitmap *picc, const RGB * pal2, bool keep_transparent)
  {
    unsigned char color_mapped_table[256];

    for (int jj = 0; jj < 256; jj++)
    {
      if ((pal1[jj].r == 0) && (pal1[jj].g == 0) && (pal1[jj].b == 0))
      {
        color_mapped_table[jj] = 0;
      }
      else
      {
        color_mapped_table[jj] = bestfit_color(pal2, pal1[jj].r, pal1[jj].g, pal1[jj].b);
      }
    }

    if (keep_transparent) {
      // keep transparency
      color_mapped_table[0] = 0;
      // any other pixels which are being mapped to 0, map to 16 instead
      for (int jj = 1; jj < 256; jj++) {
        if (color_mapped_table[jj] == 0)
          color_mapped_table[jj] = 16;
      }
    }

    int pic_size = picc->GetWidth() * picc->GetHeight();
    for (int jj = 0; jj < pic_size; jj++) {
      int xxl = jj % (picc->GetWidth()), yyl = jj / (picc->GetWidth());
      int rr = picc->GetPixel(xxl, yyl);
      picc->PutPixel(xxl, yyl, color_mapped_table[rr]);
    }
  }

  void wremapall(const RGB * pal1, Bitmap *picc, const RGB * pal2)
  {
    wremap(pal1, picc, pal2, false);
  }
