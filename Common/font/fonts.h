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

#ifndef __AC_FONT_H
#define __AC_FONT_H

#include "gfx/bitmap.h"

// TODO: we need to make some kind of TextManager class of this module

using namespace AGS;

class IAGSFontRenderer;
class IAGSFontRenderer2;

// Various font parameters, defining and extending font rendering behavior.
// While FontRenderer object's main goal is to render single line of text at
// the strictly determined position on canvas, FontInfo may additionally
// provide instructions on adjusting drawing position, as well as arranging
// multiple lines, and similar cases.
struct FontInfo
{
    // General font's loading and rendering flags
    unsigned char Flags;
    // Font size, in points (basically means pixels in AGS)
    int           SizePt;
    // Outlining font index, or auto-outline flag
    char          Outline;
    // Custom vertical render offset, used mainly for fixing broken fonts
    int           YOffset;

    FontInfo();
};

// Font render params, mainly for dealing with various compatibility issues and
// broken fonts. NOTE: currently left empty as a result of rewrite, but may be
// used again in the future.
struct FontRenderParams {};

void init_font_renderer();
void shutdown_font_renderer();
void adjust_y_coordinate_for_text(int* ypos, int fontnum);
IAGSFontRenderer* font_replace_renderer(int fontNumber, IAGSFontRenderer* renderer);
bool font_first_renderer_loaded();
bool font_supports_extended_characters(int fontNumber);
void ensure_text_valid_for_font(char *text, int fontnum);
int wgettextwidth(const char *texx, int fontNumber);
int wgettextheight(const char *text, int fontNumber);
int  get_font_outline(int font_number);
void set_font_outline(int font_number, int outline_type);
// Outputs a single line of text on the defined position on bitmap, using defined font, color and parameters
void wouttextxy(Common::Bitmap *ds, int xxx, int yyy, int fontNumber, color_t text_color, const char *texx);
// Loads a font from disk
bool wloadfont_size(int fontNumber, const FontInfo &font_info, const FontRenderParams *params = NULL);
void wgtprintf(Common::Bitmap *ds, int xxx, int yyy, int fontNumber, color_t text_color, char *fmt, ...);
void wfreefont(int fontNumber);

extern int wtext_multiply;

#endif // __AC_FONT_H
