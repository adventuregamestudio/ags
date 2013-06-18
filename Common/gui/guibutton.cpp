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
#include "font/fonts.h"
#include "gui/guibutton.h"
#include "gui/guimain.h"
#include "ac/spritecache.h"
#include "util/stream.h"
#include "gfx/bitmap.h"
#include "util/wgt2allg.h"

using AGS::Common::Stream;
using AGS::Common::Bitmap;


DynamicArray<GUIButton> guibuts;
//GUIButton guibuts[MAX_OBJ_EACH_TYPE];
int numguibuts = 0;

void GUIButton::WriteToFile(Stream *out)
{
  GUIObject::WriteToFile(out);
  // MACPORT FIXES: swap
  out->WriteArrayOfInt32(&pic, 12);
  out->Write(&text[0], 50);
  out->WriteInt32(textAlignment);
  out->WriteInt32(reserved1);
}

void GUIButton::ReadFromFile(Stream *in, GuiVersion gui_version)
{
  GUIObject::ReadFromFile(in, gui_version);
  // MACPORT FIXES: swap
  in->ReadArrayOfInt32(&pic, 12);
  in->Read(&text[0], 50);
  if (textcol == 0)
    textcol = 16;
  usepic = pic;

  if (gui_version >= kGuiVersion_272a) {
    textAlignment = in->ReadInt32();
    reserved1 = in->ReadInt32();
  }
  else {
    textAlignment = GBUT_ALIGN_TOPMIDDLE;
    reserved1 = 0;
  }

  // All buttons are translated at the moment
  flags |= GUIF_TRANSLATED;
}

void GUIButton::ReadFromSavedGame(Stream *in, RuntimeGUIVersion gui_version)
{
    GUIObject::ReadFromSavedGame(in, gui_version);
    in->ReadArrayOfInt32(&pic, 12);
    in->Read(&text[0], 50);
    usepic = pic;

    textAlignment = in->ReadInt32();
    reserved1 = in->ReadInt32();
}

void GUIButton::Draw(Bitmap *ds)
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
      ds->SetClip(Rect(x, y, x + wid - 1, y + hit - 1));

    if (spriteset[usepic] != NULL)
      draw_sprite_compensate(ds, usepic, x, y, 1);

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
        ds->StretchBlt(spriteset[gui_inv_pic], RectWH(x + 3, y + 3, wid - 6, hit - 6), Common::kBitmap_Transparency);
      else if (drawInv == 2)
        draw_sprite_compensate(ds, gui_inv_pic,
                               x + wid / 2 - get_adjusted_spritewidth(gui_inv_pic) / 2,
                               y + hit / 2 - get_adjusted_spriteheight(gui_inv_pic) / 2, 1);
    }

    if ((drawDisabled) && (gui_disabled_style == GUIDIS_GREYOUT)) {
      color_t draw_color = ds->GetCompatibleColor(8);
      int jj, kk;             // darken the button when disabled
      for (jj = 0; jj < spriteset[usepic]->GetWidth(); jj++) {
        for (kk = jj % 2; kk < spriteset[usepic]->GetHeight(); kk += 2)
          ds->PutPixel(x + jj, y + kk, draw_color);
      }
    }

    ds->SetClip(Rect(0, 0, ds->GetWidth() - 1, ds->GetHeight() - 1));
  } 
  else if (text[0] != 0) {
    // it's a text button

    color_t draw_color = ds->GetCompatibleColor(7);
    ds->FillRect(Rect(x, y, x + wid - 1, y + hit - 1), draw_color);
    if (flags & GUIF_DEFAULT) {
      draw_color = ds->GetCompatibleColor(16);
      ds->DrawRect(Rect(x - 1, y - 1, x + wid, y + hit), draw_color);
    }

    if ((isover) && (ispushed))
      draw_color = ds->GetCompatibleColor(15);
    else
      draw_color = ds->GetCompatibleColor(8);

    if (drawDisabled)
      draw_color = ds->GetCompatibleColor(8);

    ds->DrawLine(Line(x, y + hit - 1, x + wid - 1, y + hit - 1), draw_color);
    ds->DrawLine(Line(x + wid - 1, y, x + wid - 1, y + hit - 1), draw_color);
    if ((isover) && (ispushed))
      draw_color = ds->GetCompatibleColor(8);
    else
      draw_color = ds->GetCompatibleColor(15);

    if (drawDisabled)
      draw_color = ds->GetCompatibleColor(8);

    ds->DrawLine(Line(x, y, x + wid - 1, y), draw_color);
    ds->DrawLine(Line(x, y, x, y + hit - 1), draw_color);
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

    color_t text_color = ds->GetCompatibleColor(textcol);
    if (drawDisabled)
      text_color = ds->GetCompatibleColor(8);

    wouttext_outline(ds, usex, usey, font, text_color, oritext);
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

