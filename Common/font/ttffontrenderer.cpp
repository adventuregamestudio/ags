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

#include <alfont.h>
#include "core/platform.h"

#include "core/assetmanager.h"
#include "font/ttffontrenderer.h"
#include "util/stream.h"

using namespace AGS::Common;

// project-specific implementation
extern bool ShouldAntiAliasText();


// ***** TTF RENDERER *****
void TTFFontRenderer::AdjustYCoordinateForFont(int *ycoord, int fontNumber)
{
  // TTF fonts already have space at the top, so try to remove the gap
  // TODO: adding -1 was here before (check the comment above),
  // but how universal is this "space at the top"?
  // Also, why is this function used only in one case of text rendering?
  // Review this after we upgrade the font library.
  ycoord[0]--;
}

void TTFFontRenderer::EnsureTextValidForFont(char *text, int fontNumber)
{
  // do nothing, TTF can handle all characters
}

int TTFFontRenderer::GetTextWidth(const char *text, int fontNumber)
{
  return alfont_text_length(_fontData[fontNumber].AlFont, text);
}

int TTFFontRenderer::GetTextHeight(const char *text, int fontNumber)
{
  return alfont_get_font_real_height(_fontData[fontNumber].AlFont);
}

void TTFFontRenderer::RenderText(const char *text, int fontNumber, BITMAP *destination, int x, int y, int colour)
{
  if (y > destination->cb)  // optimisation
    return;

  // Y - 1 because it seems to get drawn down a bit
  if ((ShouldAntiAliasText()) && (bitmap_color_depth(destination) > 8))
    alfont_textout_aa(destination, _fontData[fontNumber].AlFont, text, x, y - 1, colour);
  else
    alfont_textout(destination, _fontData[fontNumber].AlFont, text, x, y - 1, colour);
}

bool TTFFontRenderer::LoadFromDisk(int fontNumber, int fontSize)
{
  return LoadFromDiskEx(fontNumber, fontSize, nullptr, nullptr);
}

bool TTFFontRenderer::IsBitmapFont()
{
    return false;
}

bool TTFFontRenderer::LoadFromDiskEx(int fontNumber, int fontSize,
    const FontRenderParams *params, FontMetrics *metrics)
{
  String file_name = String::FromFormat("agsfnt%d.ttf", fontNumber);
  Stream *reader = AssetMgr->OpenAsset(file_name);
  char *membuffer;

  if (reader == nullptr)
    return false;

  const size_t lenof = reader->GetLength();
  membuffer = (char *)malloc(lenof);
  reader->ReadArray(membuffer, lenof, 1);
  delete reader;

  ALFONT_FONT *alfptr = alfont_load_font_from_mem(membuffer, lenof);
  free(membuffer);

  if (alfptr == nullptr)
    return false;

  if (fontSize == 0)
      fontSize = 8; // compatibility fix
  if (params && params->SizeMultiplier > 1)
      fontSize *= params->SizeMultiplier;
  if (fontSize > 0)
    alfont_set_font_size(alfptr, fontSize);

  _fontData[fontNumber].AlFont = alfptr;
  _fontData[fontNumber].Params = params ? *params : FontRenderParams();

  if (metrics)
  {
      metrics->Height = alfont_get_font_height(alfptr);
      metrics->RealHeight = alfont_get_font_real_height(alfptr);
  }
  return true;
}

const char *TTFFontRenderer::GetName(int fontNumber)
{
  return alfont_get_name(_fontData[fontNumber].AlFont);
}

void TTFFontRenderer::FreeMemory(int fontNumber)
{
  alfont_destroy_font(_fontData[fontNumber].AlFont);
  _fontData.erase(fontNumber);
}
