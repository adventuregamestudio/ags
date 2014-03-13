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

#ifndef USE_ALFONT
#define USE_ALFONT
#endif

#include <stdio.h>
#include "alfont.h"
#include "ac/common.h"
#include "font/fonts.h"
#include "font/wfnfont.h"
#include "font/wfnfontrenderer.h"
#include "util/stream.h"
#include "util/file.h"
#include "util/bbop.h"
#include "gfx/allegrobitmap.h"
#include "util/wgt2allg.h"

using AGS::Common::Bitmap;
using AGS::Common::Stream;
using namespace AGS; // FIXME later

extern Stream *fopen_shared(char *,
                                 Common::FileOpenMode open_mode = Common::kFile_Open,
                                 Common::FileWorkMode work_mode = Common::kFile_Read);
extern int flength_shared(Stream *ffi);
WFNFontRenderer wfnRenderer;


void WFNFontRenderer::AdjustYCoordinateForFont(int *ycoord, int fontNumber)
{
  // Do nothing
}

void WFNFontRenderer::EnsureTextValidForFont(char *text, int fontNumber)
{
  WFNFont *font = (WFNFont*)fonts[fontNumber];
  // replace any extended characters with question marks
  while (text[0]!=0) {
    if ((unsigned char)text[0] >= font->CharCount) 
    {
      text[0] = '?';
    }
    text++;
  }
}

int WFNFontRenderer::GetTextWidth(const char *texx, int fontNumber)
{
  WFNFont *foon = (WFNFont*)fonts[fontNumber];

  int totlen = 0;
  unsigned int dd;

  unsigned char thisCharacter;
  for (dd = 0; dd < strlen(texx); dd++) 
  {
    thisCharacter = texx[dd];
    if (thisCharacter >= foon->CharCount) thisCharacter = '?';

    WFNFont::WFNChar &wfn_char = foon->Chars[thisCharacter];
    totlen += wfn_char.Width;
  }
  return totlen * wtext_multiply;
}

int WFNFontRenderer::GetTextHeight(const char *texx, int fontNumber)
{
  int highest = 0;
  unsigned int dd;
  WFNFont *foon = (WFNFont*)fonts[fontNumber];

  unsigned char thisCharacter;
  for (dd = 0; dd < strlen(texx); dd++) 
  {
    thisCharacter = texx[dd];
    if (thisCharacter >= foon->CharCount) thisCharacter = '?';

    WFNFont::WFNChar &wfn_char = foon->Chars[thisCharacter];
    int charHeight = wfn_char.Height;

    if (charHeight > highest)
      highest = charHeight;
  }
  return highest * wtext_multiply;
}

Common::Bitmap render_wrapper;
void WFNFontRenderer::RenderText(const char *text, int fontNumber, BITMAP *destination, int x, int y, int colour)
{
  unsigned int ee;

  int oldeip = get_our_eip();
  set_our_eip(415);

  render_wrapper.WrapAllegroBitmap(destination, true);

  for (ee = 0; ee < strlen(text); ee++)
    x += printchar(&render_wrapper, x, y, (WFNFont*)fonts[fontNumber], colour, (unsigned char)text[ee]);

  set_our_eip(oldeip);
}

int WFNFontRenderer::printchar(Common::Bitmap *ds, int xxx, int yyy, WFNFont *foo, color_t text_color, int charr)
{
  unsigned char *actdata;
  int tt, ss, bytewid, orixp = xxx;

  if (charr >= foo->CharCount)
    charr = '?';

  WFNFont::WFNChar &wfn_char = foo->Chars[charr];
  int charWidth = wfn_char.Width;
  int charHeight = wfn_char.Height;

  actdata = wfn_char.Data;
  bytewid = ((charWidth - 1) / 8) + 1;

  // MACPORT FIX: switch now using charWidth and charHeight
  color_t draw_color = text_color;
  for (tt = 0; tt < charHeight; tt++) {
    for (ss = 0; ss < charWidth; ss++) {
      if (((actdata[tt * bytewid + (ss / 8)] & (0x80 >> (ss % 8))) != 0)) {
        if (wtext_multiply > 1) {
          ds->FillRect(Rect(xxx + ss, yyy + tt, xxx + ss + (wtext_multiply - 1),
              yyy + tt + (wtext_multiply - 1)), draw_color);
        } 
        else
        {
            ds->PutPixel(xxx + ss, yyy + tt, draw_color);
        }
      }

      xxx += wtext_multiply - 1;
    }
    yyy += wtext_multiply - 1;
    xxx = orixp;
  }
  return charWidth * wtext_multiply;
}

bool WFNFontRenderer::LoadFromDisk(int fontNumber, int fontSize)
{
  char filnm[20];
  Stream *ffi = NULL;

  sprintf(filnm, "agsfnt%d.wfn", fontNumber);
  ffi = fopen_shared(filnm);
  if (ffi == NULL)
  {
    // actual font not found, try font 0 instead
    strcpy(filnm, "agsfnt0.wfn");
    ffi = fopen_shared(filnm);
    if (ffi == NULL)
      return false;
  }

  WFNFont *font = new WFNFont();
  bool result = font->ReadFromFile(ffi, flength_shared(ffi));
  delete ffi;
  if (!result)
  {
      delete font;
      return false;
  }
  fonts[fontNumber] = (IFont*)font;
  return true;
}

void WFNFontRenderer::FreeMemory(int fontNumber)
{
  delete (WFNFont*)fonts[fontNumber];
  fonts[fontNumber] = NULL;
}

bool WFNFontRenderer::SupportsExtendedCharacters(int fontNumber)
{
  WFNFont *font = (WFNFont*)fonts[fontNumber];
  return font->CharCount > 128;
}
