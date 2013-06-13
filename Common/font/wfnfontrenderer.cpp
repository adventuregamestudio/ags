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


// **** WFN Renderer ****

const char *WFN_FILE_SIGNATURE = "WGT Font File  ";
WFNFontRenderer wfnRenderer;


void WFNFontRenderer::AdjustYCoordinateForFont(int *ycoord, int fontNumber)
{
  // Do nothing
}

void WFNFontRenderer::EnsureTextValidForFont(char *text, int fontNumber)
{
  // replace any extended characters with question marks
  while (text[0]!=0) {
    if ((unsigned char)text[0] > 126) 
    {
      text[0] = '?';
    }
    text++;
  }
}


// Get the position of a character in a bitmap font.
// Rewritten code to fulfill the alignment restrictions of pointers
// on the MIPS processor. Can and should be done more efficiently.
char* psp_get_char(wgtfont foon, int thisCharacter)
{
  char* tabaddr_ptr = NULL;
  short tabaddr_value = 0;

  //tabaddr = (short *)&foon[15];
  tabaddr_ptr = (char*)&foon[15];

  //tabaddr = (short *)&foon[tabaddr[0]];     // get address table
  memcpy(&tabaddr_value, tabaddr_ptr, 2);
#if defined (AGS_BIG_ENDIAN)
  AGS::Common::BitByteOperations::SwapBytesInt16(tabaddr_value);
#endif
  tabaddr_ptr = (char*)&foon[tabaddr_value];

  //tabaddr = (short *)&foon[tabaddr[thisCharacter]];      // use table to find character
  memcpy(&tabaddr_value, &tabaddr_ptr[thisCharacter*2], 2);
#if defined (AGS_BIG_ENDIAN)
  AGS::Common::BitByteOperations::SwapBytesInt16(tabaddr_value);
#endif

  return (char*)&foon[tabaddr_value];
}


int WFNFontRenderer::GetTextWidth(const char *texx, int fontNumber)
{
  wgtfont foon = fonts[fontNumber];

  int totlen = 0;
  unsigned int dd;

  char thisCharacter;
  for (dd = 0; dd < strlen(texx); dd++) 
  {
    thisCharacter = texx[dd];
    if ((thisCharacter >= 128) || (thisCharacter < 0)) thisCharacter = '?';

    char* fontaddr = psp_get_char(foon, thisCharacter);

    short tabaddr_d;
    memcpy(&tabaddr_d, (char*)((long)fontaddr + 0), 2);
#if defined (AGS_BIG_ENDIAN)
  AGS::Common::BitByteOperations::SwapBytesInt16(tabaddr_d);
#endif

    totlen += tabaddr_d;
  }
  return totlen * wtext_multiply;
}

int WFNFontRenderer::GetTextHeight(const char *texx, int fontNumber)
{
  int highest = 0;
  unsigned int dd;
  wgtfont foon = fonts[fontNumber];

  char thisCharacter;
  for (dd = 0; dd < strlen(texx); dd++) 
  {
    thisCharacter = texx[dd];
    if ((thisCharacter >= 128) || (thisCharacter < 0)) thisCharacter = '?';

    char* fontaddr = psp_get_char(foon, thisCharacter);
    short tabaddr_d;
    memcpy(&tabaddr_d, (char*)((long)fontaddr + 2), 2);
#if defined (AGS_BIG_ENDIAN)
  AGS::Common::BitByteOperations::SwapBytesInt16(tabaddr_d);
#endif

    int charHeight = tabaddr_d;

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
  color_t text_color = render_wrapper.GetCompatibleColor(colour);

  for (ee = 0; ee < strlen(text); ee++)
    x += printchar(&render_wrapper, x, y, fonts[fontNumber], text_color, text[ee]);

  set_our_eip(oldeip);
}

int WFNFontRenderer::printchar(Common::Bitmap *ds, int xxx, int yyy, wgtfont foo, color_t text_color, int charr)
{
  unsigned char *actdata;
  int tt, ss, bytewid, orixp = xxx;

  if ((charr > 127) || (charr < 0))
    charr = '?';

  char* tabaddr = psp_get_char(foo, charr);

  short tabaddr_d;
  memcpy(&tabaddr_d, (char*)((long)tabaddr), 2);
#if defined (AGS_BIG_ENDIAN)
  AGS::Common::BitByteOperations::SwapBytesInt16(tabaddr_d);
#endif
  int charWidth = tabaddr_d;

  memcpy(&tabaddr_d, (char*)((long)tabaddr + 2), 2);
#if defined (AGS_BIG_ENDIAN)
  AGS::Common::BitByteOperations::SwapBytesInt16(tabaddr_d);
#endif
  int charHeight = tabaddr_d;

  actdata = (unsigned char *)&tabaddr[2*2];
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
  char mbuffer[16];
  long lenof;

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

  mbuffer[15] = 0;
  ffi->ReadArray(mbuffer, 15, 1);
  if (strcmp(mbuffer, WFN_FILE_SIGNATURE) != 0) {
    delete ffi;
    return false;
  }

  lenof = flength_shared(ffi);

  wgtfont tempalloc = (wgtfont) malloc(lenof + 40);
  delete ffi;

  ffi = fopen_shared(filnm);
  ffi->ReadArray(tempalloc, lenof, 1);
  delete ffi;

  fonts[fontNumber] = tempalloc;
  return true;
}

void WFNFontRenderer::FreeMemory(int fontNumber)
{
  free(fonts[fontNumber]);
  fonts[fontNumber] = NULL;
}
