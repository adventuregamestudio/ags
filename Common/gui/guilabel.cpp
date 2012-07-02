
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gui/guilabel.h"
#include "gui/guimain.h"
#include "wgt2allg.h"

DynamicArray<GUILabel> guilabels;
int numguilabels = 0;

void GUILabel::WriteToFile(FILE * ooo)
{
  GUIObject::WriteToFile(ooo);
  // MACPORT FIXES: swap
  //fwrite(&text[0], sizeof(char), 200, ooo);
  putw((int)strlen(text) + 1, ooo);
  fwrite(&text[0], sizeof(char), strlen(text) + 1, ooo);
  fwrite(&font, sizeof(int), 3, ooo);
}

void GUILabel::ReadFromFile(FILE * ooo, int version)
{
  GUIObject::ReadFromFile(ooo, version);

  if (textBufferLen > 0)
    free(text);

  if (version < 113) {
    textBufferLen = 200;
  }
  else {
    textBufferLen = getw(ooo);
  }

  text = (char*)malloc(textBufferLen);
  fread(&text[0], sizeof(char), textBufferLen, ooo);

  fread(&font, sizeof(int), 3, ooo);
  if (textcol == 0)
    textcol = 16;
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

void GUILabel::printtext_align(int yy, char *teptr)
{
  int outxp = x;
  if (align == GALIGN_CENTRE)
    outxp += wid / 2 - wgettextwidth(teptr, font) / 2;
  else if (align == GALIGN_RIGHT)
    outxp += wid - wgettextwidth(teptr, font);

  wouttext_outline(outxp, yy, font, teptr);
}

void GUILabel::Draw()
{
  int cyp = y, TEXT_HT;
  char oritext[MAX_GUILABEL_TEXT_LEN], *teptr;

  check_font(&font);

  Draw_replace_macro_tokens(oritext, text);

  teptr = &oritext[0];
  TEXT_HT = wgettextheight("ZhypjIHQFb", font) + 1;

  wtextcolor(textcol);

  Draw_split_lines(teptr, wid, font, numlines);

  for (int aa = 0; aa < numlines; aa++) {
    printtext_align(cyp, lines[aa]);
    cyp += TEXT_HT;
    if (cyp > y + hit)
      break;
  }

}
