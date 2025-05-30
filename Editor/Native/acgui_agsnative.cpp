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
// Implementation from acgui.h and acgui.cpp specific to AGS.Native library
//
//=============================================================================

#pragma unmanaged

#include "ac/gamesetupstruct.h"
#include "font/fonts.h"
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

//=============================================================================
// AGS.Native-specific implementation split out of acgui.h
//=============================================================================

void wouttext_outline(Bitmap *ds, int xxp, int yyp, int usingfont, color_t text_color, const char *texx)
{
  wouttextxy(ds, xxp, yyp, usingfont, text_color, texx);
}

void wouttext_outline(Bitmap *ds, int xxp, int yyp, int usingfont, color_t text_color, BlendMode blend_mode, const char *texx)
{
    wouttextxy(ds, xxp, yyp, usingfont, text_color, texx);
}

//=============================================================================
// AGS.Native-specific implementation split out of acgui.cpp
//=============================================================================

int final_col_dep = 32;

bool is_sprite_alpha(int spr)
{
  return false;
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

void GUIControl::MarkPositionChanged(bool)
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
