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
#include "ac/gamestructdefines.h"
#include "font/fonts.h"
#include "font/agsfontrenderer.h"
#include "font/ttffontrenderer.h"
#include "font/wfnfontrenderer.h"
#include "gfx/bitmap.h"
#include "util/wgt2allg.h"

namespace BitmapHelper = AGS::Common::BitmapHelper;

int wtext_multiply = 1;

static IAGSFontRenderer* fontRenderers[MAX_FONTS];
static IAGSFontRenderer2* fontRenderers2[MAX_FONTS];
static TTFFontRenderer ttfRenderer;
static WFNFontRenderer wfnRenderer;

void init_font_renderer()
{
#ifdef USE_ALFONT
  alfont_init();
  alfont_text_mode(-1);
#endif

  for (int i = 0; i < MAX_FONTS; i++)
  {
    fontRenderers[i] = NULL;
    fontRenderers2[i] = NULL;
  }
}

void shutdown_font_renderer()
{
#ifdef USE_ALFONT
  set_our_eip(9919);
  alfont_exit();
#endif
}

void adjust_y_coordinate_for_text(int* ypos, int fontnum)
{
  fontRenderers[fontnum]->AdjustYCoordinateForFont(ypos, fontnum);
}

bool font_first_renderer_loaded() {
    return fontRenderers[0] != NULL;
}

IAGSFontRenderer* font_replace_renderer(int fontNumber, IAGSFontRenderer* renderer)
{
  IAGSFontRenderer* oldRender = fontRenderers[fontNumber];
  fontRenderers[fontNumber] = renderer;
  fontRenderers2[fontNumber] = NULL;
  return oldRender;
}

bool font_supports_extended_characters(int fontNumber)
{
  return fontRenderers[fontNumber]->SupportsExtendedCharacters(fontNumber);
}

void ensure_text_valid_for_font(char *text, int fontnum)
{
  fontRenderers[fontnum]->EnsureTextValidForFont(text, fontnum);
}

int wgettextwidth(const char *texx, int fontNumber)
{
  return fontRenderers[fontNumber]->GetTextWidth(texx, fontNumber);
}

int wgettextheight(const char *text, int fontNumber)
{
  return fontRenderers[fontNumber]->GetTextHeight(text, fontNumber);
}

void wouttextxy(Common::Bitmap *ds, int xxx, int yyy, int fontNumber, color_t text_color, const char *texx)
{
  if (yyy > ds->GetClip().Bottom)
    return;                   // each char is clipped but this speeds it up

  if (fontRenderers[fontNumber] != NULL)
  {
    fontRenderers[fontNumber]->RenderText(texx, fontNumber, (BITMAP*)ds->GetAllegroBitmap(), xxx, yyy, text_color);
  }
}

// Loads a font from disk
bool wloadfont_size(int fontNumber, int fsize, const FontRenderParams *params)
{
  if (ttfRenderer.LoadFromDiskEx(fontNumber, fsize, params))
  {
    fontRenderers[fontNumber] = &ttfRenderer;
    fontRenderers2[fontNumber] = &ttfRenderer;
    return true;
  }
  else if (wfnRenderer.LoadFromDiskEx(fontNumber, fsize, params))
  {
    fontRenderers[fontNumber] = &wfnRenderer;
    fontRenderers2[fontNumber] = &wfnRenderer;
    return true;
  }
  return false;
}

void wgtprintf(Common::Bitmap *ds, int xxx, int yyy, int fontNumber, color_t text_color, char *fmt, ...)
{
  char tbuffer[2000];
  va_list ap;

  va_start(ap, fmt);
  vsprintf(tbuffer, fmt, ap);
  va_end(ap);
  wouttextxy(ds, xxx, yyy, fontNumber, text_color, tbuffer);
}

void wfreefont(int fontNumber)
{
  if (fontRenderers[fontNumber] != NULL)
    fontRenderers[fontNumber]->FreeMemory(fontNumber);

  fontRenderers[fontNumber] = NULL;
}
