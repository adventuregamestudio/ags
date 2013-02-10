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
#include "font/agsfontrenderer.h"
#include "font/ttffontrenderer.h"
#include "font/wfnfontrenderer.h"
#include "gfx/bitmap.h"
#include "util/wgt2allg.h"

using AGS::Common::Bitmap;
namespace BitmapHelper = AGS::Common::BitmapHelper;

int texttrans = 0;
int textcol;
int wtext_multiply = 1;

void init_font_renderer()
{
#ifdef USE_ALFONT
  alfont_init();
  alfont_text_mode(-1);
#endif

  for (int i = 0; i < MAX_FONTS; i++)
    fontRenderers[i] = NULL;
  wtexttransparent(TEXTFG);
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

void wouttextxy(int xxx, int yyy, int fontNumber, const char *texx)
{
  if (yyy > abuf->GetClip().Bottom)
    return;                   // each char is clipped but this speeds it up

  if (fontRenderers[fontNumber] != NULL)
  {
    fontRenderers[fontNumber]->RenderText(texx, fontNumber, (BITMAP*)abuf->GetBitmapObject(), xxx, yyy, textcol);
  }
}

// Loads a font from disk
bool wloadfont_size(int fontNumber, int fsize)
{
  if (ttfRenderer.LoadFromDisk(fontNumber, fsize))
  {
    fontRenderers[fontNumber] = &ttfRenderer;
    return true;
  }
  else if (wfnRenderer.LoadFromDisk(fontNumber, fsize))
  {
    fontRenderers[fontNumber] = &wfnRenderer;
    return true;
  }

  return false;
}

void wgtprintf(int xxx, int yyy, int fontNumber, char *fmt, ...)
{
  char tbuffer[2000];
  va_list ap;

  va_start(ap, fmt);
  vsprintf(tbuffer, fmt, ap);
  va_end(ap);
  wouttextxy(xxx, yyy, fontNumber, tbuffer);
}

void wtextcolor(int nval)
{
  __my_setcolor(&textcol, nval);
}

void wfreefont(int fontNumber)
{
  if (fontRenderers[fontNumber] != NULL)
    fontRenderers[fontNumber]->FreeMemory(fontNumber);

  fontRenderers[fontNumber] = NULL;
}

void wtexttransparent(int coo)
{
  texttrans = coo;
}
