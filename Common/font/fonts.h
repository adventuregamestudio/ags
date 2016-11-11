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

using namespace AGS;

class IAGSFontRenderer;
class IAGSFontRenderer2;

// Font render params, mainly for dealing with various compatibility
// issues and broken fonts.
struct FontRenderParams
{
    int YOffset; // vertical offset for the line of text (can be negative)

    FontRenderParams()
        : YOffset(0)
    {
    }
};

void init_font_renderer();
void shutdown_font_renderer();
void adjust_y_coordinate_for_text(int* ypos, int fontnum);
IAGSFontRenderer* font_replace_renderer(int fontNumber, IAGSFontRenderer* renderer);
bool font_first_renderer_loaded();
bool font_supports_extended_characters(int fontNumber);
void ensure_text_valid_for_font(char *text, int fontnum);
int wgettextwidth(const char *texx, int fontNumber);
int wgettextheight(const char *text, int fontNumber);
void wouttextxy(Common::Bitmap *ds, int xxx, int yyy, int fontNumber, color_t text_color, const char *texx);
// Loads a font from disk
bool wloadfont_size(int fontNumber, int fsize, const FontRenderParams *params = NULL);
void wgtprintf(Common::Bitmap *ds, int xxx, int yyy, int fontNumber, color_t text_color, char *fmt, ...);
void wfreefont(int fontNumber);

extern int wtext_multiply;

#endif // __AC_FONT_H
