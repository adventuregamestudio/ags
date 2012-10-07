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
#include "util/wgt2allg.h"
#include "gui/guibutton.h"
#include "gui/guimain.h"
#include "ac/spritecache.h"
#include "util/datastream.h"
#include "gfx/bitmap.h"

using AGS::Common::DataStream;
using AGS::Common::Bitmap;

extern SpriteCache spriteset;



DynamicArray<GUIButton> guibuts;
//GUIButton guibuts[MAX_OBJ_EACH_TYPE];
int numguibuts = 0;

void GUIButton::WriteToFile(DataStream *out)
{
  GUIObject::WriteToFile(out);
  // MACPORT FIXES: swap
  out->WriteArrayOfInt32(&pic, 12);
  out->Write(&text[0], 50);
  out->WriteInt32(textAlignment);
  out->WriteInt32(reserved1);
}

void GUIButton::ReadFromFile(DataStream *in, int version)
{
  GUIObject::ReadFromFile(in, version);
  // MACPORT FIXES: swap
  in->ReadArrayOfInt32(&pic, 12);
  in->Read(&text[0], 50);
  if (textcol == 0)
    textcol = 16;
  usepic = pic;

  if (version >= 111) {
    textAlignment = in->ReadInt32();
    reserved1 = in->ReadInt32();
  }
  else {
    textAlignment = GBUT_ALIGN_TOPMIDDLE;
    reserved1 = 0;
  }
}

void GUIButton::Draw()
{
  int drawDisabled = IsDisabled();

  check_font(&font);
  // if it's "Unchanged when disabled" or "GUI Off", don't grey out
  if ((gui_disabled_style == GUIDIS_UNCHANGED) ||
      (gui_disabled_style == GUIDIS_GUIOFF))
    drawDisabled = 0;

  if (usepic <= 0)
    usepic = pic;

  if (drawDisabled)
    usepic = pic;

  if ((drawDisabled) && (gui_disabled_style == GUIDIS_BLACKOUT))
    // buttons off when disabled - no point carrying on
    return;

  // draw it!!
  if ((usepic > 0) && (pic > 0)) {

    if (flags & GUIF_CLIP)
      abuf->SetClip(Rect(x, y, x + wid - 1, y + hit - 1));

    if (spriteset[usepic] != NULL)
      draw_sprite_compensate(usepic, x, y, 1);

    if (gui_inv_pic >= 0) {
      int drawInv = 0;

      // Stretch to fit button
      if (stricmp(text, "(INV)") == 0)
        drawInv = 1;
      // Draw at actual size
      if (stricmp(text, "(INVNS)") == 0)
        drawInv = 2;

      // Stretch if too big, actual size if not
      if (stricmp(text, "(INVSHR)") == 0) {
        if ((get_adjusted_spritewidth(gui_inv_pic) > wid - 6) ||
            (get_adjusted_spriteheight(gui_inv_pic) > hit - 6))
          drawInv = 1;
        else
          drawInv = 2;
      }

      if (drawInv == 1)
        abuf->StretchBlt(spriteset[gui_inv_pic], RectWH(x + 3, y + 3, wid - 6, hit - 6), Common::kBitmap_Transparency);
      else if (drawInv == 2)
        draw_sprite_compensate(gui_inv_pic,
                               x + wid / 2 - get_adjusted_spritewidth(gui_inv_pic) / 2,
                               y + hit / 2 - get_adjusted_spriteheight(gui_inv_pic) / 2, 1);
    }

    if ((drawDisabled) && (gui_disabled_style == GUIDIS_GREYOUT)) {
      int col8 = get_col8_lookup(8);
      int jj, kk;             // darken the button when disabled
      for (jj = 0; jj < spriteset[usepic]->GetWidth(); jj++) {
        for (kk = jj % 2; kk < spriteset[usepic]->GetHeight(); kk += 2)
          abuf->PutPixel(x + jj, y + kk, col8);
      }
    }

    abuf->SetClip(Rect(0, 0, abuf->GetWidth() - 1, abuf->GetHeight() - 1));
  } 
  else if (text[0] != 0) {
    // it's a text button

    wsetcolor(7);
    abuf->FillRect(Rect(x, y, x + wid - 1, y + hit - 1), currentcolor);
    if (flags & GUIF_DEFAULT) {
      wsetcolor(16);
      abuf->DrawRect(Rect(x - 1, y - 1, x + wid, y + hit), currentcolor);
    }

    if ((isover) && (ispushed))
      wsetcolor(15);
    else
      wsetcolor(8);

    if (drawDisabled)
      wsetcolor(8);

    abuf->DrawLine(Line(x, y + hit - 1, x + wid - 1, y + hit - 1), currentcolor);
    abuf->DrawLine(Line(x + wid - 1, y, x + wid - 1, y + hit - 1), currentcolor);
    if ((isover) && (ispushed))
      wsetcolor(8);
    else
      wsetcolor(15);

    if (drawDisabled)
      wsetcolor(8);

    abuf->DrawLine(Line(x, y, x + wid - 1, y), currentcolor);
    abuf->DrawLine(Line(x, y, x, y + hit - 1), currentcolor);
  }                           // end if text

  // Don't print text of (INV) (INVSHR) (INVNS)
  if ((text[0] == '(') && (text[1] == 'I') && (text[2] == 'N')) ; 
  // Don't print the text if there's a graphic and it hasn't been named
  else if ((usepic > 0) && (pic > 0) && (strcmp(text, "New Button") == 0)) ;
  // if there is some text, print it
  else if (text[0] != 0) {
    int usex = x, usey = y;

    char oritext[200]; // text[] can be not longer than 50 characters due declaration
    Draw_set_oritext(oritext, text);

    if ((ispushed) && (isover)) {
      // move the text a bit while pushed
      usex++;
      usey++;
    }

    switch (textAlignment) {
    case GBUT_ALIGN_TOPMIDDLE:
      usex += (wid / 2 - wgettextwidth(oritext, font) / 2);
      usey += 2;
      break;
    case GBUT_ALIGN_TOPLEFT:
      usex += 2;
      usey += 2;
      break;
    case GBUT_ALIGN_TOPRIGHT:
      usex += (wid - wgettextwidth(oritext, font)) - 2;
      usey += 2;
      break;
    case GBUT_ALIGN_MIDDLELEFT:
      usex += 2;
      usey += (hit / 2 - (wgettextheight(oritext, font) + 1) / 2);
      break;
    case GBUT_ALIGN_CENTRED:
      usex += (wid / 2 - wgettextwidth(oritext, font) / 2);
      usey += (hit / 2 - (wgettextheight(oritext, font) + 1) / 2);
      break;
    case GBUT_ALIGN_MIDDLERIGHT:
      usex += (wid - wgettextwidth(oritext, font)) - 2;
      usey += (hit / 2 - (wgettextheight(oritext, font) + 1) / 2);
      break;
    case GBUT_ALIGN_BOTTOMLEFT:
      usex += 2;
      usey += (hit - wgettextheight(oritext, font)) - 2;
      break;
    case GBUT_ALIGN_BOTTOMMIDDLE:
      usex += (wid / 2 - wgettextwidth(oritext, font) / 2);
      usey += (hit - wgettextheight(oritext, font)) - 2;
      break;
    case GBUT_ALIGN_BOTTOMRIGHT:
      usex += (wid - wgettextwidth(oritext, font)) - 2;
      usey += (hit - wgettextheight(oritext, font)) - 2;
      break;
    }

    wtextcolor(textcol);
    if (drawDisabled)
      wtextcolor(8);

    WOUTTEXT_REVERSE(usex, usey, font, oritext);
  }
  
}

void GUIButton::MouseUp()
{
  if (isover) {
    usepic = overpic;
    if ((!IsDisabled()) && (IsClickable()))
      activated++;
  }
  else
    usepic = pic;

  ispushed = 0;
}

