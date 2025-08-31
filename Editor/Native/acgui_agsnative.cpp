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

//-----------------------------------------------------------------------------
// Following is a partial reimplementation of the engine's blender module
//-----------------------------------------------------------------------------

extern "C" {
    // Fallback routine for when we don't have anything better to do.
    uint32_t _blender_black(uint32_t x, uint32_t y, uint32_t n);
    // Standard Allegro 4 trans blenders for 16 and 15-bit color modes
    uint32_t _blender_trans15(uint32_t x, uint32_t y, uint32_t n);
    uint32_t _blender_trans16(uint32_t x, uint32_t y, uint32_t n);
    // Standard Allegro 4 alpha blenders for 16 and 15-bit color modes
    uint32_t _blender_alpha15(uint32_t x, uint32_t y, uint32_t n);
    uint32_t _blender_alpha16(uint32_t x, uint32_t y, uint32_t n);
    uint32_t _blender_alpha24(uint32_t x, uint32_t y, uint32_t n);
}

static uint32_t argb2argb_blend_core(uint32_t src_col, uint32_t dst_col, uint32_t src_alpha)
{
    uint32_t dst_g, dst_alpha;
    src_alpha++;
    dst_alpha = geta32(dst_col);
    if (dst_alpha)
        dst_alpha++;

    // dst_g now contains the green hue from destination color
    dst_g = (dst_col & 0x00FF00) * dst_alpha / 256;
    // dst_col now contains the red & blue hues from destination color
    dst_col = (dst_col & 0xFF00FF) * dst_alpha / 256;

    // res_g now contains the green hue of the pre-final color
    dst_g = (((src_col & 0x00FF00) - (dst_g & 0x00FF00)) * src_alpha / 256 + dst_g) & 0x00FF00;
    // res_rb now contains the red & blue hues of the pre-final color
    dst_col = (((src_col & 0xFF00FF) - (dst_col & 0xFF00FF)) * src_alpha / 256 + dst_col) & 0xFF00FF;

    // dst_alpha now contains the final alpha
    // we assume that final alpha will never be zero
    dst_alpha = 256 - (256 - src_alpha) * (256 - dst_alpha) / 256;
    // src_alpha is now the final alpha factor made for being multiplied by,
    // instead of divided by: this makes it possible to use it in faster
    // calculation below
    src_alpha = /* 256 * 256 == */ 0x10000 / dst_alpha;

    // setting up final color hues
    dst_g = (dst_g * src_alpha / 256) & 0x00FF00;
    dst_col = (dst_col * src_alpha / 256) & 0xFF00FF;
    return dst_col | dst_g | (--dst_alpha << 24);
}

// blend source to destination with respect to source and destination alphas;
// assign new alpha value as a multiplication of translucenses.
// combined_alpha = front.alpha + back.alpha * (1 - front.alpha);
// combined_rgb = (front.rgb * front.alpha + back.rgb * (1 - front.alpha) * back.alpha) / combined_alpha;
uint32_t _argb2argb_blender(uint32_t src_col, uint32_t dst_col, uint32_t src_alpha)
{
    if (src_alpha > 0)
        src_alpha = geta32(src_col) * ((src_alpha & 0xFF) + 1) / 256;
    else
        src_alpha = geta32(src_col);
    if (src_alpha == 0)
        return dst_col;
    return argb2argb_blend_core(src_col, dst_col, src_alpha);
}

static void set_argb2any_blender()
{
    set_blender_mode_ex(_blender_black, _blender_black, _blender_black, _argb2argb_blender,
                        _blender_alpha15, _blender_alpha16, _blender_alpha24,
                        0, 0, 0, 0xff);
}

// Draw outline that is calculated from the text font, not derived from an outline font
static void wouttextxy_AutoOutline(Bitmap *ds, size_t font, int color, const char *texx, int &xxp, int &yyp)
{
    const FontInfo &finfo = get_fontinfo(font);
    int const thickness = finfo.AutoOutlineThickness;
    auto const style = finfo.AutoOutlineStyle;
    if (thickness <= 0)
        return;

    // 16-bit games should use 32-bit stencils to keep anti-aliasing working
    // because 16-bit blending works correctly if there's an actual color
    // on the destination bitmap (and our intermediate bitmaps are transparent).
    int const  ds_cd = ds->GetColorDepth();
    bool const antialias = ds_cd >= 16 && thisgame.options[OPT_ANTIALIASFONTS] != 0 && !is_bitmap_font(font);
    int const  stencil_cd = antialias ? 32 : ds_cd;
    if (antialias) // This is to make sure TTFs render proper alpha channel in 16-bit games too
        color |= makeacol32(0, 0, 0, 0xff);

    const int t_width = get_text_width(texx, font);
    const auto t_extent = get_font_surface_extent(font);
    const int t_height = t_extent.second - t_extent.first + 1;
    if (t_width == 0 || t_height == 0)
        return;
    // Prepare stencils
    const int t_yoff = t_extent.first;
    Bitmap *texx_stencil, *outline_stencil;
    alloc_font_outline_buffers(font, &texx_stencil, &outline_stencil,
                               t_width, t_height, stencil_cd);
    texx_stencil->ClearTransparent();
    outline_stencil->ClearTransparent();
    // Ready text stencil
    // Note we are drawing with y off, in case some font's glyphs exceed font's ascender
    wouttextxy(texx_stencil, 0, -t_yoff, font, color, texx);
    // Anti-aliased TTFs require to be alpha-blended, not blit,
    // or the alpha values will be plain copied and final image will be broken.
    void(Bitmap::*pfn_drawstencil)(const Bitmap * src, int dst_x, int dst_y);
    if (antialias)
    { // NOTE: we must set out blender AFTER wouttextxy, or it will be overidden
        set_argb2any_blender();
        pfn_drawstencil = &Bitmap::TransBlendBlt;
    }
    else
    {
        pfn_drawstencil = &Bitmap::MaskedBlit;
    }

    // move start of text so that the outline doesn't drop off the bitmap
    xxp += thickness;
    int const outline_y = yyp + t_yoff;
    yyp += thickness;

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
        (ds->*pfn_drawstencil)(outline_stencil, xxp - x_diff, outline_y);
        if (x_diff > 0)
            (ds->*pfn_drawstencil)(outline_stencil, xxp + x_diff, outline_y);
    }
}

void wouttext_outline(Common::Bitmap *ds, int xxp, int yyp, int font, color_t text_color, color_t outline_color, const char *texx)
{
    size_t const text_font = static_cast<size_t>(font);
    // Draw outline (a backdrop) if requested
    int const outline_font = get_font_outline(font);
    if (outline_font >= 0)
        wouttextxy(ds, xxp, yyp, static_cast<size_t>(outline_font), outline_color, texx);
    else if (outline_font == FONT_OUTLINE_AUTO)
        wouttextxy_AutoOutline(ds, text_font, outline_color, texx, xxp, yyp);
    else
        ; // no outline

    // Draw text on top
    wouttextxy(ds, xxp, yyp, text_font, text_color, texx);
}

void wouttext_outline(Bitmap *ds, int xxp, int yyp, int font, color_t text_color, const char *texx)
{
    const color_t outline_color = ds->GetCompatibleColor(16);
    wouttext_outline(ds, xxp, yyp, font, text_color, outline_color, texx);
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

void GUIObject::MarkChanged()
{
    // do nothing: in Editor "guis" array is not even guaranteed to be filled!
}

void GUIObject::MarkParentChanged()
{
    // do nothing: in Editor "guis" array is not even guaranteed to be filled!
}

void GUIObject::MarkPositionChanged(bool)
{
    // do nothing: in Editor "guis" array is not even guaranteed to be filled!
}

void GUIObject::MarkStateChanged(bool, bool)
{
    // do nothing: in Editor "guis" array is not even guaranteed to be filled!
}

bool GUIMain::HasAlphaChannel() const
{
    if (this->_bgImage > 0)
    {
        // alpha state depends on background image
        return is_sprite_alpha(this->_bgImage);
    }
    if (this->_bgColor > 0)
    {
        // not alpha transparent if there is a background color
        return false;
    }
    // transparent background, enable alpha blending
    return thisgame.color_depth * 8 >= 24;
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

bool GUIInvWindow::HasAlphaChannel() const
{
    return false; // don't do alpha in the editor
}

int  GUIInvWindow::GetCharacterID() const
{
    return -1;
}

void GUIInvWindow::Draw(Bitmap *ds, int x, int y)
{
    color_t draw_color = ds->GetCompatibleColor(15);
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
