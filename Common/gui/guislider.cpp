
#include <stdio.h>
#include "util/wgt2allg.h"
#include "gui/guislider.h"
#include "gui/guimain.h"
#include "ac/spritecache.h"
#include "util/datastream.h"
#include "gfx/bitmap.h"

using AGS::Common::DataStream;
using AGS::Common::Bitmap;

extern SpriteCache spriteset;

DynamicArray<GUISlider> guislider;
int numguislider = 0;

void GUISlider::WriteToFile(DataStream *out)
{
  GUIObject::WriteToFile(out);
  // MACPORT FIX: swap
  out->WriteArrayOfInt32(&min, 7);
}

void GUISlider::ReadFromFile(DataStream *in, int version)
{
  int sizeToRead = 4;

  if (version >= 104)
    sizeToRead = 7;
  else {
    handlepic = -1;
    handleoffset = 0;
    bgimage = 0;
  }

  GUIObject::ReadFromFile(in, version);
  in->ReadArrayOfInt32(&min, sizeToRead);
}

void GUISlider::Draw()
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
      draw_sprite_compensate(bgimage, cx, cy, 1);
      cx += xinc;
      cy += yinc;
      // done as a do..while so that at least one of the image is drawn
    } while ((cx + xinc <= barbrx) && (cy + yinc <= barbry));

  }
  else {
    // normal grey background
    wsetcolor(16);
    abuf->FillRect(Rect(bartlx + 1, bartly + 1, barbrx - 1, barbry - 1), currentcolor);

    wsetcolor(8);
    abuf->DrawLine(Line(bartlx, bartly, bartlx, barbry), currentcolor);
    abuf->DrawLine(Line(bartlx, bartly, barbrx, bartly), currentcolor);

    wsetcolor(15);
    abuf->DrawLine(Line(barbrx, bartly + 1, barbrx, barbry), currentcolor);
    abuf->DrawLine(Line(bartlx, barbry, barbrx, barbry), currentcolor);
  }

  if (handlepic > 0) {
    // an image for the slider handle
    if (spriteset[handlepic] == NULL)
      handlepic = 0;
    handtlx -= get_adjusted_spritewidth(handlepic) / 2;
    handtly -= get_adjusted_spriteheight(handlepic) / 2;
    draw_sprite_compensate(handlepic, handtlx, handtly, 1);
    handbrx = handtlx + get_adjusted_spritewidth(handlepic);
    handbry = handtly + get_adjusted_spriteheight(handlepic);
  }
  else {
    // normal grey tracker handle
    wsetcolor(7);
    abuf->FillRect(Rect(handtlx, handtly, handbrx, handbry), currentcolor);

    wsetcolor(15);
    abuf->DrawLine(Line(handtlx, handtly, handbrx, handtly), currentcolor);
    abuf->DrawLine(Line(handtlx, handtly, handtlx, handbry), currentcolor);

    wsetcolor(16);
    abuf->DrawLine(Line(handbrx, handtly + 1, handbrx, handbry), currentcolor);
    abuf->DrawLine(Line(handtlx + 1, handbry, handbrx, handbry), currentcolor);
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
