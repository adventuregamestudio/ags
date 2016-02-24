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

#include "alfont.h"
#include "ac/common.h"
#include "core/assetmanager.h"
#include "debug/out.h"
#include "font/fonts.h"
#include "font/wfnfontrenderer.h"
#include "gfx/allegrobitmap.h"
#include "util/file.h"
#include "util/stream.h"
#include "util/string.h"

using namespace AGS::Common;

const char *WFN_FILE_SIGNATURE = "WGT Font File  ";

static unsigned char GetCharCode(unsigned char wanted_code, const WFNFont* font)
{
    return wanted_code < font->GetCharCount() ? wanted_code : '?';
}

static int RenderChar(Common::Bitmap *ds, const int at_x, const int at_y, const WFNChar &wfn_char, const color_t text_color);

void WFNFontRenderer::AdjustYCoordinateForFont(int *ycoord, int fontNumber)
{
  // Do nothing
}

void WFNFontRenderer::EnsureTextValidForFont(char *text, int fontNumber)
{
  const WFNFont* font = _fontData[fontNumber];
  // replace any extended characters with question marks
  for (; *text; ++text)
  {
    if ((unsigned char)*text >= font->GetCharCount()) 
    {
      *text = '?';
    }
  }
}

int WFNFontRenderer::GetTextWidth(const char *text, int fontNumber)
{
  const WFNFont* font = _fontData[fontNumber];
  int text_width = 0;

  for (; *text; ++text)
  {
    const WFNChar &wfn_char = font->GetChar(GetCharCode(*text, font));
    text_width += wfn_char.Width;
  }
  return text_width * wtext_multiply;
}

int WFNFontRenderer::GetTextHeight(const char *text, int fontNumber)
{
  const WFNFont* font = _fontData[fontNumber];
  int max_height = 0;

  for (; *text; ++text) 
  {
    const WFNChar &wfn_char = font->GetChar(GetCharCode(*text, font));
    const uint16_t height = wfn_char.Height;
    if (height > max_height)
      max_height = height;
  }
  return max_height * wtext_multiply;
}

Common::Bitmap render_wrapper;
void WFNFontRenderer::RenderText(const char *text, int fontNumber, BITMAP *destination, int x, int y, int colour)
{
  int oldeip = get_our_eip();
  set_our_eip(415);

  const WFNFont* font = _fontData[fontNumber];
  render_wrapper.WrapAllegroBitmap(destination, true);

  for (; *text; ++text)
    x += RenderChar(&render_wrapper, x, y, font->GetChar(GetCharCode(*text, font)), colour);

  set_our_eip(oldeip);
}

int RenderChar(Common::Bitmap *ds, const int at_x, const int at_y, const WFNChar &wfn_char, const color_t text_color)
{
  const int width = wfn_char.Width;
  const int height = wfn_char.Height;
  const unsigned char *actdata = wfn_char.Data;
  const int bytewid = wfn_char.GetRowByteCount();

  int x = at_x;
  int y = at_y;
  for (int h = 0; h < height; ++h)
  {
    for (int w = 0; w < width; ++w)
    {
      if (((actdata[h * bytewid + (w / 8)] & (0x80 >> (w % 8))) != 0)) {
        if (wtext_multiply > 1)
        {
          ds->FillRect(Rect(x + w, y + h, x + w + (wtext_multiply - 1),
              y + h + (wtext_multiply - 1)), text_color);
        } 
        else
        {
          ds->PutPixel(x + w, y + h, text_color);
        }
      }

      x += wtext_multiply - 1;
    }
    y += wtext_multiply - 1;
    x = at_x;
  }
  return width * wtext_multiply;
}

bool WFNFontRenderer::LoadFromDisk(int fontNumber, int fontSize)
{
  String file_name;
  Stream *ffi = NULL;

  file_name.Format("agsfnt%d.wfn", fontNumber);
  ffi = AssetManager::OpenAsset(file_name);
  if (ffi == NULL)
  {
    // actual font not found, try font 0 instead
    file_name = "agsfnt0.wfn";
    ffi = AssetManager::OpenAsset(file_name);
    if (ffi == NULL)
      return false;
  }

  WFNFont *font = new WFNFont();
  WFNError err = font->ReadFromFile(ffi, AssetManager::GetLastAssetSize());
  delete ffi;
  if (err == kWFNErr_HasBadCharacters)
    Out::FPrint("WARNING: font '%s' has mistakes in data format, some characters may be displayed incorrectly", file_name.GetCStr());
  else if (err != kWFNErr_NoError)
  {
    delete font;
    return false;
  }
  _fontData[fontNumber] = font;
  return true;
}

void WFNFontRenderer::FreeMemory(int fontNumber)
{
  delete _fontData[fontNumber];
  _fontData.erase(fontNumber);
}

bool WFNFontRenderer::SupportsExtendedCharacters(int fontNumber)
{
  return _fontData[fontNumber]->GetCharCount() > 128;
}
