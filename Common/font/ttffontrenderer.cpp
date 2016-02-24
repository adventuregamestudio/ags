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
#include "ac/gamestructdefines.h" //FONT_OUTLINE_AUTO
#include "core/assetmanager.h"
#include "font/ttffontrenderer.h"
#include "util/stream.h"
#include "util/string.h"

using AGS::Common::AssetManager;
using AGS::Common::Stream;
using AGS::Common::String;

// project-specific implementation
extern bool ShouldAntiAliasText();

// Defined in the engine or editor (currently needed only for non-windows versions)
extern int  get_font_outline(int font_number);
extern void set_font_outline(int font_number, int outline_type);

#ifdef USE_ALFONT
ALFONT_FONT *tempttffnt;
ALFONT_FONT *get_ttf_block(unsigned char* fontptr)
{
  memcpy(&tempttffnt, &fontptr[4], sizeof(tempttffnt));
  return tempttffnt;
}
#endif // USE_ALFONT


// ***** TTF RENDERER *****
#ifdef USE_ALFONT	// declaration was not under USE_ALFONT though

void TTFFontRenderer::AdjustYCoordinateForFont(int *ycoord, int fontNumber)
{
  // TTF fonts already have space at the top, so try to remove the gap
  ycoord[0]--;
}

void TTFFontRenderer::EnsureTextValidForFont(char *text, int fontNumber)
{
  // do nothing, TTF can handle all characters
}

int TTFFontRenderer::GetTextWidth(const char *text, int fontNumber)
{
  return alfont_text_length(_fontData[fontNumber], text);
}

int TTFFontRenderer::GetTextHeight(const char *text, int fontNumber)
{
  return alfont_text_height(_fontData[fontNumber]);
}

void TTFFontRenderer::RenderText(const char *text, int fontNumber, BITMAP *destination, int x, int y, int colour)
{
  if (y > destination->cb)  // optimisation
    return;

  // Y - 1 because it seems to get drawn down a bit
  if ((ShouldAntiAliasText()) && (bitmap_color_depth(destination) > 8))
    alfont_textout_aa(destination, _fontData[fontNumber], text, x, y - 1, colour);
  else
    alfont_textout(destination, _fontData[fontNumber], text, x, y - 1, colour);
}

bool TTFFontRenderer::LoadFromDisk(int fontNumber, int fontSize)
{
  String file_name = String::FromFormat("agsfnt%d.ttf", fontNumber);
  Stream *reader = AssetManager::OpenAsset(file_name);
  char *membuffer;

  if (reader == NULL)
    return false;

  long lenof = AssetManager::GetLastAssetSize();

  membuffer = (char *)malloc(lenof);
  reader->ReadArray(membuffer, lenof, 1);
  delete reader;

  ALFONT_FONT *alfptr = alfont_load_font_from_mem(membuffer, lenof);
  free(membuffer);

  if (alfptr == NULL)
    return false;

  // TODO: move this somewhere, should not be right here
#if !defined(WINDOWS_VERSION)
  // FIXME: (!!!) this fix should be done differently:
  // 1. Find out which OUTLINE font was causing troubles;
  // 2. Replace outline method ONLY if that troublesome font is used as outline.
  // 3. Move this fix somewhere else!! (right after game load routine?)
  //
  // Check for the LucasFan font since it comes with an outline font that
  // is drawn incorrectly with Freetype versions > 2.1.3.
  // A simple workaround is to disable outline fonts for it and use
  // automatic outline drawing.
  if (get_font_outline(fontNumber) >=0 &&
      strcmp(alfont_get_name(alfptr), "LucasFan-Font") == 0)
      set_font_outline(fontNumber, FONT_OUTLINE_AUTO);
#endif

  if (fontSize > 0)
    alfont_set_font_size(alfptr, fontSize);

  _fontData[fontNumber] = alfptr;
  return true;
}

void TTFFontRenderer::FreeMemory(int fontNumber)
{
  alfont_destroy_font(_fontData[fontNumber]);
  _fontData.erase(fontNumber);
}

#endif   // USE_ALFONT
