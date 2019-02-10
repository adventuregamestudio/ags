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
#include "ac/gamestructdefines.h"
#include "font/fonts.h"
#include "font/agsfontrenderer.h"
#include "font/ttffontrenderer.h"
#include "font/wfnfontrenderer.h"
#include "gfx/bitmap.h"
#include "util/wgt2allg.h"

using namespace AGS::Common;

int wtext_multiply = 1;

namespace AGS
{
namespace Common
{

struct Font
{
    IAGSFontRenderer   *Renderer;
    IAGSFontRenderer2  *Renderer2;
    FontInfo            Info;

    Font();
};

Font::Font()
    : Renderer(NULL)
    , Renderer2(NULL)
{}

} // Common
} // AGS

static std::vector<Font> fonts;
static TTFFontRenderer ttfRenderer;
static WFNFontRenderer wfnRenderer;


FontInfo::FontInfo()
    : Flags(0)
    , SizePt(0)
    , Outline(FONT_OUTLINE_NONE)
    , YOffset(0)
    , LineSpacing(0)
{}


void init_font_renderer()
{
#ifdef USE_ALFONT
  alfont_init();
  alfont_text_mode(-1);
#endif
}

void shutdown_font_renderer()
{
#ifdef USE_ALFONT
  set_our_eip(9919);
  alfont_exit();
#endif
}

void adjust_y_coordinate_for_text(int* ypos, size_t fontnum)
{
  if (fontnum >= fonts.size() || !fonts[fontnum].Renderer)
    return;
  fonts[fontnum].Renderer->AdjustYCoordinateForFont(ypos, fontnum);
}

bool font_first_renderer_loaded()
{
  return fonts.size() > 0 && fonts[0].Renderer != NULL;
}

IAGSFontRenderer* font_replace_renderer(size_t fontNumber, IAGSFontRenderer* renderer)
{
  if (fontNumber >= fonts.size())
    return NULL;
  IAGSFontRenderer* oldRender = fonts[fontNumber].Renderer;
  fonts[fontNumber].Renderer = renderer;
  fonts[fontNumber].Renderer2 = NULL;
  return oldRender;
}

bool font_supports_extended_characters(size_t fontNumber)
{
  if (fontNumber >= fonts.size() || !fonts[fontNumber].Renderer)
    return false;
  return fonts[fontNumber].Renderer->SupportsExtendedCharacters(fontNumber);
}

void ensure_text_valid_for_font(char *text, size_t fontnum)
{
  if (fontnum >= fonts.size() || !fonts[fontnum].Renderer)
    return;
  fonts[fontnum].Renderer->EnsureTextValidForFont(text, fontnum);
}

int wgettextwidth(const char *texx, size_t fontNumber)
{
  if (fontNumber >= fonts.size() || !fonts[fontNumber].Renderer)
    return 0;
  return fonts[fontNumber].Renderer->GetTextWidth(texx, fontNumber);
}

int wgettextheight(const char *text, size_t fontNumber)
{
  if (fontNumber >= fonts.size() || !fonts[fontNumber].Renderer)
    return 0;
  return fonts[fontNumber].Renderer->GetTextHeight(text, fontNumber);
}

int get_font_outline(size_t font_number)
{
    if (font_number >= fonts.size())
        return FONT_OUTLINE_NONE;
    return fonts[font_number].Info.Outline;
}

void set_font_outline(size_t font_number, int outline_type)
{
    if (font_number >= fonts.size())
        return;
    fonts[font_number].Info.Outline = FONT_OUTLINE_AUTO;
}

int getfontheight(size_t fontNumber)
{
  if (fontNumber >= fonts.size() || !fonts[fontNumber].Renderer)
    return 0;
  // There is no explicit method for getting maximal possible height of any
  // random font renderer at the moment; the implementations of GetTextHeight
  // are allowed to return varied results depending on the text parameter.
  // We use special line of text to get more or less reliable font height.
  const char *height_test_string = "ZHwypgfjqhkilIK";
  return fonts[fontNumber].Renderer->GetTextHeight(height_test_string, fontNumber);
}

int getfontlinespacing(size_t fontNumber)
{
  if (fontNumber >= fonts.size())
    return 0;
  int spacing = fonts[fontNumber].Info.LineSpacing;
  // If the spacing parameter is not provided, then return default
  // spacing, that is font's height.
  return spacing > 0 ? spacing : getfontheight(fontNumber);
}

bool use_default_linespacing(size_t fontNumber)
{
    if (fontNumber >= fonts.size())
        return false;
    return fonts[fontNumber].Info.LineSpacing == 0;
}

void wouttextxy(Common::Bitmap *ds, int xxx, int yyy, size_t fontNumber, color_t text_color, const char *texx)
{
  if (fontNumber >= fonts.size())
    return;
  yyy += fonts[fontNumber].Info.YOffset;
  if (yyy > ds->GetClip().Bottom)
    return;                   // each char is clipped but this speeds it up

  if (fonts[fontNumber].Renderer != NULL)
  {
    fonts[fontNumber].Renderer->RenderText(texx, fontNumber, (BITMAP*)ds->GetAllegroBitmap(), xxx, yyy, text_color);
  }
}

void set_fontinfo(size_t fontNumber, const FontInfo &finfo)
{
    if (fontNumber < fonts.size() && fonts[fontNumber].Renderer)
        fonts[fontNumber].Info = finfo;
}

// Loads a font from disk
bool wloadfont_size(size_t fontNumber, const FontInfo &font_info, const FontRenderParams *params)
{
  fonts.resize(fontNumber + 1);
  if (ttfRenderer.LoadFromDiskEx(fontNumber, font_info.SizePt, params))
  {
    fonts[fontNumber].Renderer  = &ttfRenderer;
    fonts[fontNumber].Renderer2 = &ttfRenderer;
  }
  else if (wfnRenderer.LoadFromDiskEx(fontNumber, font_info.SizePt, params))
  {
    fonts[fontNumber].Renderer  = &wfnRenderer;
    fonts[fontNumber].Renderer2 = &wfnRenderer;
  }

  if (fonts[fontNumber].Renderer)
  {
      fonts[fontNumber].Info = font_info;
      return true;
  }
  return false;
}

void wgtprintf(Common::Bitmap *ds, int xxx, int yyy, size_t fontNumber, color_t text_color, char *fmt, ...)
{
  if (fontNumber >= fonts.size())
    return;

  char tbuffer[2000];
  va_list ap;

  va_start(ap, fmt);
  vsprintf(tbuffer, fmt, ap);
  va_end(ap);
  wouttextxy(ds, xxx, yyy, fontNumber, text_color, tbuffer);
}

void wfreefont(size_t fontNumber)
{
  if (fontNumber >= fonts.size())
    return;

  if (fonts[fontNumber].Renderer != NULL)
    fonts[fontNumber].Renderer->FreeMemory(fontNumber);

  fonts[fontNumber].Renderer = NULL;
}
