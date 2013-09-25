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

#include <stdio.h>
#include "gui/guislider.h"
#include "gui/guimain.h"
#include "ac/spritecache.h"
#include "util/stream.h"
#include "gfx/bitmap.h"
#include "util/wgt2allg.h"

using AGS::Common::Stream;
using AGS::Common::Bitmap;

DynamicArray<GUISlider> guislider;
int numguislider = 0;

void GUISlider::WriteToFile(Stream *out)
{
  GUIObject::WriteToFile(out);
  // MACPORT FIX: swap
  out->WriteArrayOfInt32(&min, 7);
}

void GUISlider::ReadFromFile(Stream *in, GuiVersion gui_version)
{
  int sizeToRead = 4;

  if (gui_version >= kGuiVersion_unkn_104)
    sizeToRead = 7;
  else {
    handlepic = -1;
    handleoffset = 0;
    bgimage = 0;
  }

  GUIObject::ReadFromFile(in, gui_version);
  in->ReadArrayOfInt32(&min, sizeToRead);
}

void GUISlider::Draw(Common::Bitmap *ds)
{
  int bartlx, bartly, barbrx, barbry;
  int handtlx, handtly, handbrx, handbry, thickness;

  if (min >= max)
    max = min + 1;

  if (value > max)
    value = max;

  if (value < min)
    value = min;

  // it's a horizontal slider
  if (wid > hit) {
    thickness = hit / 3;
    bartlx = x + 1;
    bartly = y + hit / 2 - thickness;
    barbrx = x + wid - 1;
    barbry = y + hit / 2 + thickness + 1;
    handtlx = (int)(((float)(value - min) / (float)(max - min)) * (float)(wid - 4) - 2) + bartlx + 1;
    handtly = bartly - (thickness - 1);
    handbrx = handtlx + get_fixed_pixel_size(4);
    handbry = barbry + (thickness - 1);

    if (handlepic > 0) {
      // store the centre of the pic rather than the top
      handtly = bartly + (barbry - bartly) / 2 + get_fixed_pixel_size(1);
      handtlx += get_fixed_pixel_size(2);
    }
    handtly += multiply_up_coordinate(handleoffset);
    handbry += multiply_up_coordinate(handleoffset);
  }
  // vertical slider
  else {
    thickness = wid / 3;
    bartlx = x + wid / 2 - thickness;
    bartly = y + 1;
    barbrx = x + wid / 2 + thickness + 1;
    barbry = y + hit - 1;
    handtly = (int)(((float)(max - value) / (float)(max - min)) * (float)(hit - 4) - 2) + bartly + 1;
    handtlx = bartlx - (thickness - 1);
    handbry = handtly + get_fixed_pixel_size(4);
    handbrx = barbrx + (thickness - 1);

    if (handlepic > 0) {
      // store the centre of the pic rather than the left
      handtlx = bartlx + (barbrx - bartlx) / 2 + get_fixed_pixel_size(1);
      handtly += get_fixed_pixel_size(2);
    }
    handtlx += multiply_up_coordinate(handleoffset);
    handbrx += multiply_up_coordinate(handleoffset);
  }

  color_t draw_color;

  if (bgimage > 0) {
    // tiled image as slider background
    int xinc = 0, yinc = 0;
    if (wid > hit) {
      // horizontal slider
      xinc = get_adjusted_spritewidth(bgimage);
      // centre the image vertically
      bartly = y + (hit / 2) - get_adjusted_spriteheight(bgimage) / 2;
    }
    else {
      // vertical slider
      yinc = get_adjusted_spriteheight(bgimage);
      // centre the image horizontally
      bartlx = x + (wid / 2) - get_adjusted_spritewidth(bgimage) / 2;
    }

    int cx = bartlx, cy = bartly;
    // draw the tiled background image
    do {
      draw_gui_sprite(ds, bgimage, cx, cy, true);
      cx += xinc;
      cy += yinc;
      // done as a do..while so that at least one of the image is drawn
    } while ((cx + xinc <= barbrx) && (cy + yinc <= barbry));

  }
  else {
    // normal grey background
    draw_color = ds->GetCompatibleColor(16);
    ds->FillRect(Rect(bartlx + 1, bartly + 1, barbrx - 1, barbry - 1), draw_color);

    draw_color = ds->GetCompatibleColor(8);
    ds->DrawLine(Line(bartlx, bartly, bartlx, barbry), draw_color);
    ds->DrawLine(Line(bartlx, bartly, barbrx, bartly), draw_color);

    draw_color = ds->GetCompatibleColor(15);
    ds->DrawLine(Line(barbrx, bartly + 1, barbrx, barbry), draw_color);
    ds->DrawLine(Line(bartlx, barbry, barbrx, barbry), draw_color);
  }

  if (handlepic > 0) {
    // an image for the slider handle
    if (spriteset[handlepic] == NULL)
      handlepic = 0;
    handtlx -= get_adjusted_spritewidth(handlepic) / 2;
    handtly -= get_adjusted_spriteheight(handlepic) / 2;
    draw_gui_sprite(ds, handlepic, handtlx, handtly, true);
    handbrx = handtlx + get_adjusted_spritewidth(handlepic);
    handbry = handtly + get_adjusted_spriteheight(handlepic);
  }
  else {
    // normal grey tracker handle
    draw_color = ds->GetCompatibleColor(7);
    ds->FillRect(Rect(handtlx, handtly, handbrx, handbry), draw_color);

    draw_color = ds->GetCompatibleColor(15);
    ds->DrawLine(Line(handtlx, handtly, handbrx, handtly), draw_color);
    ds->DrawLine(Line(handtlx, handtly, handtlx, handbry), draw_color);

    draw_color = ds->GetCompatibleColor(16);
    ds->DrawLine(Line(handbrx, handtly + 1, handbrx, handbry), draw_color);
    ds->DrawLine(Line(handtlx + 1, handbry, handbrx, handbry), draw_color);
  }

  cached_handtlx = handtlx;
  cached_handtly = handtly;
  cached_handbrx = handbrx;
  cached_handbry = handbry;
}

void GUISlider::MouseMove(int xp, int yp)
{
  if (mpressed == 0)
    return;

  if (wid > hit)                // horizontal slider
    value = (int)(((float)((xp - x) - 2) / (float)(wid - 4)) * (float)(max - min)) + min;
  else                          // vertical slider
    value = (int)(((float)(((y + hit) - yp) - 2) / (float)(hit - 4)) * (float)(max - min)) + min;

  if (value > max)
    value = max;

  if (value < min)
    value = min;

  guis_need_update = 1;
  activated = 1;
}
