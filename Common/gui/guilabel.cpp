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
#include "font/fonts.h"
#include "gui/guilabel.h"
#include "gui/guimain.h"
#include "util/stream.h"
#include "util/wgt2allg.h"

using AGS::Common::Stream;

DynamicArray<GUILabel> guilabels;
int numguilabels = 0;

void GUILabel::WriteToFile(Stream *out)
{
  GUIObject::WriteToFile(out);
  // MACPORT FIXES: swap
  //->WriteArray(&text[0], sizeof(char), 200);
  out->WriteInt32((int)strlen(text) + 1);
  out->Write(&text[0], strlen(text) + 1);
  out->WriteArrayOfInt32(&font, 3);
}

void GUILabel::ReadFromFile(Stream *in, GuiVersion gui_version)
{
  GUIObject::ReadFromFile(in, gui_version);

  if (textBufferLen > 0)
    free(text);

  if (gui_version < kGuiVersion_272c) {
    textBufferLen = 200;
  }
  else {
    textBufferLen = in->ReadInt32();
  }

  text = (char*)malloc(textBufferLen);
  in->Read(&text[0], textBufferLen);

  in->ReadArrayOfInt32(&font, 3);
  if (textcol == 0)
    textcol = 16;

  // All labels are translated at the moment
  flags |= GUIF_TRANSLATED;
}

void GUILabel::ReadFromSavedGame(Common::Stream *in, RuntimeGUIVersion gui_version)
{
    GUIObject::ReadFromSavedGame(in, gui_version);

    if (textBufferLen > 0)
        free(text);

    textBufferLen = in->ReadInt32();
    text = (char*)malloc(textBufferLen);
    in->Read(&text[0], textBufferLen);
    in->ReadArrayOfInt32(&font, 3);
}

void GUILabel::SetText(const char *newText) {

  if ((int)strlen(newText) < textBufferLen) {
    strcpy(this->text, newText);
    return;
  }

  if (textBufferLen > 0)
    free(this->text);

  textBufferLen = (int)strlen(newText) + 1;

  this->text = (char*)malloc(textBufferLen);
  strcpy(this->text, newText);

  // restrict to this length
  if (textBufferLen >= MAX_GUILABEL_TEXT_LEN)
    this->text[MAX_GUILABEL_TEXT_LEN - 1] = 0;
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
  int cyp = y, TEXT_HT;
  char oritext[MAX_GUILABEL_TEXT_LEN], *teptr;

  check_font(&font);

  Draw_replace_macro_tokens(oritext, text);

  teptr = &oritext[0];
  TEXT_HT = wgettextheight("ZhypjIHQFb", font) + 1;

  color_t text_color = ds->GetCompatibleColor(textcol);

  Draw_split_lines(teptr, wid, font, numlines);

  for (int aa = 0; aa < numlines; aa++) {
    printtext_align(ds, cyp, text_color, lines[aa]);
    cyp += TEXT_HT;
    if (cyp > y + hit)
      break;
  }

}
