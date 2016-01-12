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
#include "gui/guitextbox.h"
#include "gui/guimain.h"
#include "util/stream.h"
#include "gfx/bitmap.h"
#include "util/wgt2allg.h"

using AGS::Common::Stream;
using AGS::Common::Bitmap;

DynamicArray<GUITextBox> guitext;
int numguitext = 0;

void GUITextBox::WriteToFile(Stream *out)
{
  GUIObject::WriteToFile(out);
  // MACPORT FIXES: swap
  out->Write(&text[0], 200);
  out->WriteArrayOfInt32(&font, 3);
}

void GUITextBox::ReadFromFile(Stream *in, GuiVersion gui_version)
{
  GUIObject::ReadFromFile(in, gui_version);
  // MACPORT FIXES: swap
  in->Read(&text[0], 200);
  in->ReadArrayOfInt32(&font, 3);
  if (textcol == 0)
    textcol = 16;
}

void GUITextBox::Draw(Common::Bitmap *ds)
{

  check_font(&font);
  color_t text_color = ds->GetCompatibleColor(textcol);
  color_t draw_color = ds->GetCompatibleColor(textcol);
  if ((exflags & GTF_NOBORDER) == 0) {
    ds->DrawRect(Rect(x, y, x + wid - 1, y + hit - 1), draw_color);
    if (get_fixed_pixel_size(1) > 1)
      ds->DrawRect(Rect(x + 1, y + 1, x + wid - get_fixed_pixel_size(1), y + hit - get_fixed_pixel_size(1)), draw_color);
  }

  Draw_text_box_contents(ds, text_color);
}

void GUITextBox::KeyPress(int kp)
{
  guis_need_update = 1;
  // backspace, remove character
  if ((kp == 8) && (strlen(text) > 0)) {
    text[strlen(text) - 1] = 0;
    return;
  } else if (kp == 8)
    return;

  // other key, continue
  if ((kp >= 128) && (!font_supports_extended_characters(font)))
    return;

  if (kp == 13) {
    activated++;
    return;
  }

  if (strlen(text) >= 199)
    return;

  text[strlen(text) + 1] = 0;
  text[strlen(text)] = kp;

  // if the new string is too long, remove the new character
  if (wgettextwidth(text, font) > (wid - (6 + get_fixed_pixel_size(5))))
    text[strlen(text) - 1] = 0;

}
