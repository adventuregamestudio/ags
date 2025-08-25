//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "font/wfnfontrenderer.h"
#include <algorithm>
#include "ac/common.h" // our_eip
#include "core/assetmanager.h"
#include "debug/out.h"
#include "font/wfnfont.h"
#include "gfx/bitmap.h"
#include "util/stream.h"

using namespace AGS::Common;

void WFNFontRenderer::AdjustYCoordinateForFont(int * /*ycoord*/, int /*fontNumber*/)
{
    // Do nothing
}

void WFNFontRenderer::EnsureTextValidForFont(char *text, int fontNumber)
{
    // Do nothing
}

int WFNFontRenderer::GetTextWidth(const char *text, int fontNumber)
{
    const WFNFont* font = _fontData[fontNumber].Font;
    const FontRenderParams &params = _fontData[fontNumber].Params;
    int text_width = 0;

    for (int code = ugetxc(&text); code; code = ugetxc(&text))
    {
        text_width += font->GetChar(code).Width;
    }
    return text_width * params.SizeMultiplier;
}

int WFNFontRenderer::GetTextHeight(const char *text, int fontNumber)
{
    const WFNFont* font = _fontData[fontNumber].Font;
    const FontRenderParams &params = _fontData[fontNumber].Params;
    int max_height = 0;

    for (int code = ugetxc(&text); code; code = ugetxc(&text))
    {
        const uint16_t height = font->GetChar(code).Height;
        max_height = std::max(max_height, static_cast<int>(height));
    }
    return max_height * params.SizeMultiplier;
}

static int RenderChar(Bitmap *ds, const int at_x, const int at_y, Rect clip,
    const WFNChar &wfn_char, const int scale, const color_t text_color);

void WFNFontRenderer::RenderText(const char *text, int fontNumber, BITMAP *destination, int x, int y, int colour)
{
  int oldeip = get_our_eip();
  set_our_eip(415);

  const WFNFont* font = _fontData[fontNumber].Font;
  const FontRenderParams &params = _fontData[fontNumber].Params;
  Bitmap ds(destination, true);

  // NOTE: allegro's putpixel ignores clipping (optimization),
  // so we'll have to accomodate for that ourselves
  Rect clip = ds.GetClip();
  for (int code = ugetxc(&text); code; code = ugetxc(&text))
    x += RenderChar(&ds, x, y, clip, font->GetChar(code), params.SizeMultiplier, colour);

  set_our_eip(oldeip);
}

static int RenderChar(Bitmap *ds, const int at_x, const int at_y, Rect clip,
    const WFNChar &wfn_char, const int scale, const color_t text_color)
{
  const int width = wfn_char.Width;
  const int height = wfn_char.Height;
  const unsigned char *actdata = wfn_char.Data;
  const int bytewid = wfn_char.GetRowByteCount();

  int sx = std::max(at_x, clip.Left), ex = clip.Right + 1;
  int sy = std::max(at_y, clip.Top), ey = clip.Bottom + 1;
  int sw = std::max(0, clip.Left - at_x);
  int sh = std::max(0, clip.Top - at_y);
  for (int h = sh, y = sy; h < height && y < ey; ++h, y += scale)
  {
    for (int w = sw, x = sx; w < width && x < ex; ++w, x += scale)
    {
      if (((actdata[h * bytewid + (w / 8)] & (0x80 >> (w % 8))) != 0))
      {
        if (scale > 1)
        {
          ds->FillRect(RectWH(x, y, scale, scale), text_color);
        } 
        else
        {
          ds->PutPixel(x, y, text_color);
        }
      }
    }
  }
  return width * scale;
}

bool WFNFontRenderer::LoadFromDisk(int fontNumber, int fontSize)
{
  return LoadFromDiskEx(fontNumber, fontSize, String(), nullptr, nullptr, nullptr);
}

bool WFNFontRenderer::IsBitmapFont()
{
    return true;
}

// Fill the FontMetrics struct from the given WFNFont
static void FillMetrics(const WFNFont *font, FontMetrics *metrics)
{
    metrics->NominalHeight = font->GetHeight();
    metrics->RealHeight = font->GetHeight();
    metrics->CompatHeight = metrics->NominalHeight; // just set to default here
    metrics->BBox = font->GetBBox();
    metrics->VExtent = std::make_pair(metrics->BBox.Top, metrics->BBox.Bottom);
    // fix it up to be *not less* than realheight
    metrics->VExtent.first = std::min(0, metrics->VExtent.first);
    metrics->VExtent.second = std::max(metrics->RealHeight - 1, metrics->VExtent.second);
}

bool WFNFontRenderer::LoadFromDiskEx(int fontNumber, int /*fontSize*/, const String &filename,
    String *src_filename, const FontRenderParams *params, FontMetrics *metrics)
{
  String use_filename = !filename.IsEmpty() ? filename :
    String::FromFormat("agsfnt%d.wfn", fontNumber);
  auto ffi = _amgr->OpenAsset(use_filename);
  if (!ffi)
    return false;

  WFNFont *font = new WFNFont();
  WFNError err = font->ReadFromFile(ffi.get());
  if (err == kWFNErr_HasBadCharacters)
  {
    Debug::Printf(kDbgMsg_Warn, "WARNING: font '%s' has mistakes in data format, some characters may be displayed incorrectly", use_filename.GetCStr());
  }
  else if (err != kWFNErr_NoError)
  {
    delete font;
    return false;
  }
  _fontData[fontNumber].Font = font;
  _fontData[fontNumber].Params = params ? *params : FontRenderParams();
  if (src_filename)
    *src_filename = use_filename;
  if (metrics)
    FillMetrics(font, metrics);
  return true;
}

void WFNFontRenderer::GetFontMetrics(int fontNumber, FontMetrics *metrics)
{
    FillMetrics(_fontData[fontNumber].Font, metrics);
}

void WFNFontRenderer::GetCharCodeRange(int fontNumber, std::pair<int, int> *char_codes)
{
    if (_fontData[fontNumber].Font->GetCharCount() == 0)
        *char_codes = std::make_pair(0, 0);
    else
        *char_codes = std::make_pair(0,
            static_cast<int>(_fontData[fontNumber].Font->GetCharCount() - 1));
}

void WFNFontRenderer::GetValidCharCodes(int fontNumber, std::vector<int> &char_codes)
{
    const WFNFont *font = _fontData[fontNumber].Font;
    for (int i = 0; i < font->GetCharCount(); ++i)
    {
        if (font->GetChar(i).IsValid())
            char_codes.push_back(i);
    }
}

void WFNFontRenderer::FreeMemory(int fontNumber)
{
  delete _fontData[fontNumber].Font;
  _fontData.erase(fontNumber);
}

bool WFNFontRenderer::SupportsExtendedCharacters(int fontNumber)
{
  return _fontData[fontNumber].Font->GetCharCount() > 128;
}
