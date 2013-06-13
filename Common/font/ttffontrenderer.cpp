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
#include "font/ttffontrenderer.h"
#include "util/stream.h"
#include "gfx/bitmap.h"
#include "core/assetmanager.h"
#include "util/wgt2allg.h"

using AGS::Common::Bitmap;
using AGS::Common::Stream;

// project-specific implementation
extern bool ShouldAntiAliasText();

#if !defined (WINDOWS_VERSION)
#include <sys/stat.h>
#define _fileno fileno
off_t _filelength(int fd) {
	struct stat st;
	fstat(fd, &st);
	return st.st_size;
}
#endif

// Defined in the engine or editor (currently needed only for non-windows versions)
extern void set_font_outline(int font_number, int outline_type);

TTFFontRenderer ttfRenderer;

#ifdef USE_ALFONT
ALFONT_FONT *tempttffnt;
ALFONT_FONT *get_ttf_block(wgtfont fontptr)
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
  return alfont_text_length(get_ttf_block(fonts[fontNumber]), text);
}

int TTFFontRenderer::GetTextHeight(const char *text, int fontNumber)
{
  return alfont_text_height(get_ttf_block(fonts[fontNumber]));
}

void TTFFontRenderer::RenderText(const char *text, int fontNumber, BITMAP *destination, int x, int y, int colour)
{
  if (y > destination->cb)  // optimisation
    return;

  ALFONT_FONT *alfpt = get_ttf_block(fonts[fontNumber]);
  // Y - 1 because it seems to get drawn down a bit
  if ((ShouldAntiAliasText()) && (bitmap_color_depth(destination) > 8))
    alfont_textout_aa(destination, alfpt, text, x, y - 1, colour);
  else
    alfont_textout(destination, alfpt, text, x, y - 1, colour);
}

bool TTFFontRenderer::LoadFromDisk(int fontNumber, int fontSize)
{
  char filnm[20];
  sprintf(filnm, "agsfnt%d.ttf", fontNumber);

  // we read the font in manually to make it load from library file
  Stream *reader = Common::AssetManager::OpenAsset(filnm);
  char *membuffer;

  if (reader == NULL)
    return false;

  long lenof = Common::AssetManager::GetAssetSize(filnm);

  // if not in the library, get size manually
  if (lenof < 1)
  {
	  lenof = reader->GetLength();
  }

  membuffer = (char *)malloc(lenof);
  reader->ReadArray(membuffer, lenof, 1);
  delete reader;

  ALFONT_FONT *alfptr = alfont_load_font_from_mem(membuffer, lenof);
  free(membuffer);

  if (alfptr == NULL)
    return false;

#if !defined(WINDOWS_VERSION)
  // Check for the LucasFan font since it comes with an outline font that
  // is drawn incorrectly with Freetype versions > 2.1.3.
  // A simple workaround is to disable outline fonts for it and use
  // automatic outline drawing.
  if (strcmp(alfont_get_name(alfptr), "LucasFan-Font") == 0)
      //game.fontoutline[fontNumber] = FONT_OUTLINE_AUTO;
      set_font_outline(fontNumber, FONT_OUTLINE_AUTO);
#endif

  if (fontSize > 0)
    alfont_set_font_size(alfptr, fontSize);

  wgtfont tempalloc = (wgtfont) malloc(20);
  strcpy((char *)tempalloc, "TTF");
  memcpy(&((char *)tempalloc)[4], &alfptr, sizeof(alfptr));
  fonts[fontNumber] = tempalloc;

  return true;
}

void TTFFontRenderer::FreeMemory(int fontNumber)
{
  alfont_destroy_font(get_ttf_block(fonts[fontNumber]));
  free(fonts[fontNumber]);
  fonts[fontNumber] = NULL;
}

#endif   // USE_ALFONT
