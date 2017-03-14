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
#include <stdlib.h>
#include <string.h>
#include "ac/game_version.h"
#include "font/fonts.h"
#include "gui/guilabel.h"
#include "gui/guimain.h"
#include "util/stream.h"
#include "util/wgt2allg.h"

using namespace AGS::Common;

std::vector<GUILabel> guilabels;
int numguilabels = 0;

void GUILabel::WriteToFile(Stream *out)
{
  GUIObject::WriteToFile(out);
  out->WriteInt32(text.GetLength() + 1);
  text.Write(out);
  out->WriteArrayOfInt32(&font, 3);
}

void GUILabel::ReadFromFile(Stream *in, GuiVersion gui_version)
{
  GUIObject::ReadFromFile(in, gui_version);

  int text_buf_len;
  if (gui_version < kGuiVersion_272c) {
    text_buf_len = 200;
  }
  else {
    text_buf_len = in->ReadInt32();
  }

  text.ReadCount(in, text_buf_len);

  in->ReadArrayOfInt32(&font, 3);
  if (textcol == 0)
    textcol = 16;

  // All labels are translated at the moment
  flags |= GUIF_TRANSLATED;
}

void GUILabel::SetText(const char *newText) {

  // restrict to this length
  // CHECKME: is there any sense in this restriction?
  text = String(newText).Left(MAX_GUILABEL_TEXT_LEN);
}

const char *GUILabel::GetText() {
  return text;
}

void GUILabel::printtext_align(Common::Bitmap *ds, int yy, color_t text_color, char *teptr)
{
  int outxp = x;
  if (align == GALIGN_CENTRE)
    outxp += wid / 2 - wgettextwidth(teptr, font) / 2;
  else if (align == GALIGN_RIGHT)
    outxp += wid - wgettextwidth(teptr, font);

  wouttext_outline(ds, outxp, yy, font, text_color, teptr);
}

void GUILabel::Draw(Common::Bitmap *ds)
{
  int cyp = y, linespacing;
  char oritext[MAX_GUILABEL_TEXT_LEN], *teptr;

  check_font(&font);

  Draw_replace_macro_tokens(oritext, text);

  teptr = &oritext[0];
  linespacing = getfontlinespacing(font) + 1;

  color_t text_color = ds->GetCompatibleColor(textcol);

  Draw_split_lines(teptr, wid, font, numlines);

  // < 2.72 labels did not limit vertical size of text
  const bool limit_by_label_frame = loaded_game_file_version >= kGameVersion_272;
  for (int aa = 0; aa < numlines; aa++) {
    printtext_align(ds, cyp, text_color, lines[aa]);
    cyp += linespacing;
    if (limit_by_label_frame && cyp > y + hit)
      break;
  }

}
