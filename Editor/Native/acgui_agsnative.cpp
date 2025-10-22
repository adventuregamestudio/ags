//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// Implementation of GUI and font drawing functions specific to the Editor.
//
//=============================================================================

#pragma unmanaged

#include "ac/gamesetupstruct.h"
#include "font/fonts.h"
#include "gfx/bitmap.h"
#include "gfx/blender.h"
#include "gfx/gfx_def.h"
#include "gui/guimain.h"
#include "gui/guibutton.h"
#include "gui/guiinv.h"
#include "gui/guilabel.h"
#include "gui/guilistbox.h"
#include "gui/guitextbox.h"
#include "util/string_utils.h"

using namespace AGS::Common;
extern GameSetupStruct thisgame;

bool ShouldAntiAliasText()
{
    return (thisgame.GetColorDepth() >= 24) && (thisgame.options[OPT_ANTIALIASFONTS] != 0);
}

// Draw outline that is calculated from the text font, not derived from an outline font
static void wouttextxy_AutoOutline(Bitmap *ds, size_t font, int32_t color, BlendMode blend_mode, const char *text, int &x, int &y)
{
    const FontInfo &finfo = get_fontinfo(font);
    int const thickness = finfo.AutoOutlineThickness;
    auto const style = finfo.AutoOutlineStyle;
    if (thickness <= 0)
        return;

    // We use 32-bit stencils in any case when alpha-blending is required
    // because blending works correctly if there's an actual color
    // on the destination bitmap (and our intermediate bitmaps are transparent).
    int const  ds_cd = ds->GetColorDepth();
    bool const alpha_blend = (ds_cd == 32) && !is_bitmap_font(font) &&
        ((thisgame.options[OPT_ANTIALIASFONTS] != 0) || (geta32(color) < 255) || (blend_mode != kBlend_Normal));
    int const  stencil_cd = alpha_blend ? 32 : ds_cd;

    const int t_width = get_text_width(text, font);
    // Horizontal extent lets us know if any glyphs have negative horizontal offset
    const auto t_extent_hor = get_font_surface_hextent(font);
    const auto t_extent_ver = get_font_surface_vextent(font);
    const int t_height = t_extent_ver.second - t_extent_ver.first + 1;
    if (t_width == 0 || t_height == 0)
        return;
    // Prepare stencils
    const int t_xoff = (t_extent_hor.first < 0 ? t_extent_hor.first : 0);
    const int t_yoff = t_extent_ver.first;
    Bitmap *texx_stencil, *outline_stencil;
    alloc_font_outline_buffers(font, &texx_stencil, &outline_stencil,
                               t_width + (-t_xoff) * 2, t_height, stencil_cd);
    texx_stencil->ClearTransparent();
    outline_stencil->ClearTransparent();
    // Ready text stencil
    // Note we are drawing with x off, in case some glyphs have negative offsets
    // and appear further to the left (we create stencil with extra space in that case);
    // and y off, in case some font's glyphs exceed font's ascender
    // NOTE: t_xoff here will only fix the outline, but this won't resolve e.g. parts
    // of leftmost glyph positioned outside of a gui control or an overlay.
    wouttextxy(texx_stencil, -t_xoff, -t_yoff, font, color, blend_mode, text);
    // Anti-aliased TTFs require to be alpha-blended, not blit,
    // or the alpha values will be plain copied and final image will be broken.
    void(Bitmap::*pfn_drawstencil)(const Bitmap * src, int dst_x, int dst_y);
    if (alpha_blend)
    { // NOTE: we must set out blender AFTER wouttextxy, or it will be overidden
        SetBlender(blend_mode, 0xFF);
        pfn_drawstencil = &Bitmap::TransBlendBlt;
    }
    else
    {
        pfn_drawstencil = &Bitmap::MaskedBlit;
    }

    // move start of text so that the outline doesn't drop off the bitmap;
    // move by glyph offset in case the stencil was drawn with offset
    x += thickness + t_xoff;
    int const outline_y = y + t_yoff;
    y += thickness;

    // What we do here: first we paint text onto outline_stencil offsetting vertically;
    // then we paint resulting outline_stencil onto final dest offsetting horizontally.
    int largest_y_diff_reached_so_far = -1;
    for (int x_diff = thickness; x_diff >= 0; x_diff--)
    {
        // Integer arithmetics: In the following, we use terms k*(k + 1) to account for rounding.
        //     (k + 0.5)^2 == k*k + 2*k*0.5 + 0.5^2 == k*k + k + 0.25 ==approx. k*(k + 1)
        int y_term_limit = thickness * (thickness + 1);
        if (FontInfo::kRounded == style)
            y_term_limit -= x_diff * x_diff;

        // extend the outline stencil to the top and bottom
        for (int y_diff = largest_y_diff_reached_so_far + 1;
             y_diff <= thickness && y_diff * y_diff <= y_term_limit;
             y_diff++)
        {
            (outline_stencil->*pfn_drawstencil)(texx_stencil, 0, thickness - y_diff);
            if (y_diff > 0)
                (outline_stencil->*pfn_drawstencil)(texx_stencil, 0, thickness + y_diff);
            largest_y_diff_reached_so_far = y_diff;
        }

        // stamp the outline stencil to the left and right of the text
        (ds->*pfn_drawstencil)(outline_stencil, x - x_diff, outline_y);
        if (x_diff > 0)
            (ds->*pfn_drawstencil)(outline_stencil, x + x_diff, outline_y);
    }

    x -= t_xoff; // remove glyph offset from the final text position (keep std text pos)
}

void wouttext_outline(Common::Bitmap *ds, int x, int y, int font, color_t text_color, color_t outline_color, BlendMode blend_mode, const char *text)
{
    size_t const text_font = static_cast<size_t>(font);
    // Draw outline (a backdrop) if requested
    int const outline_font = get_font_outline(font);
    if (outline_font >= 0)
        wouttextxy(ds, x, y, static_cast<size_t>(outline_font), outline_color, blend_mode, text);
    else if (outline_font == FONT_OUTLINE_AUTO)
        wouttextxy_AutoOutline(ds, text_font, outline_color, blend_mode, text, x, y);
    else
        ; // no outline

    // Draw text on top
    wouttextxy(ds, x, y, text_font, text_color, blend_mode, text);
}

void wouttext_outline(Bitmap *ds, int x, int y, int font, color_t text_color, const char *text)
{
    const color_t outline_color = GUI::GetStandardColorForBitmap(16);
    wouttext_outline(ds, x, y, font, text_color, outline_color, kBlend_Normal, text);
}

void wouttext_outline(Bitmap *ds, int x, int y, int font, color_t text_color, BlendMode blend_mode, const char *text)
{
    const color_t outline_color = GUI::GetStandardColorForBitmap(16);
    wouttext_outline(ds, x, y, font, text_color, outline_color, blend_mode, text);
}

bool is_sprite_alpha(int spr)
{
    return false; // FIXME
}

void set_eip_guiobj(int eip)
{
    // do nothing
}

int get_eip_guiobj()
{
    return 0;
}

namespace AGS
{
namespace Common
{

String GUI::TransformTextForDrawing(const String &text, bool /*translate*/, bool /*apply_direction*/)
{
    return text;
}

size_t GUI::SplitLinesForDrawing(const String &text, bool /*apply_direction*/, SplitLines &lines, int font, int width, size_t max_lines)
{
    return split_lines(text.GetCStr(), lines, width, font, max_lines);
}

void GUIControl::MarkChanged()
{
    // do nothing: in Editor "guis" array is not even guaranteed to be filled!
}

void GUIControl::MarkVisualStateChanged()
{
    // do nothing: in Editor "guis" array is not even guaranteed to be filled!
}

void GUIControl::MarkPositionChanged(bool, bool)
{
    // do nothing: in Editor "guis" array is not even guaranteed to be filled!
}

void GUIControl::MarkStateChanged(bool, bool)
{
    // do nothing: in Editor "guis" array is not even guaranteed to be filled!
}

int GUILabel::PrepareTextToDraw()
{
    _textToDraw = _text;
    return GUI::SplitLinesForDrawing(_textToDraw, false, Lines, _font, _width);
}

void GUITextBox::DrawTextBoxContents(Bitmap *ds, int x, int y, color_t text_color)
{
    // print something fake so we can see what it looks like
    wouttext_outline(ds, x + 2, y + 2, _font, text_color, "Text Box Contents");
}

void GUIListBox::PrepareTextToDraw(const String &text)
{
    _textToDraw = text;
}

int  GUIInvWindow::GetCharacterID() const
{
    return -1;
}

void GUIInvWindow::Draw(Bitmap *ds, int x, int y)
{
    color_t draw_color = GUI::GetStandardColorForBitmap(15);
    ds->DrawRect(RectWH(x, y, _width, _height), draw_color);
}

void GUIButton::PrepareTextToDraw()
{
    if (IsWrapText())
    {
        _textToDraw = _text;
        GUI::SplitLinesForDrawing(_text, false, Lines, _font, _width - _textPaddingHor * 2);
    }
    else
    {
        _textToDraw = _text;
    }
}

} // namespace Common
} // namespace AGS
