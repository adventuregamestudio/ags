//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// Implementation from acgui.h and acgui.cpp specific to Engine runtime
//
//=============================================================================
#include "ac/game_version.h"
#include "ac/gui.h"
#include "ac/system.h"
#include "font/fonts.h"
#include <ctype.h>
#include "ac/gamesetupstruct.h"
#include "ac/global_translation.h"
#include "ac/string.h"
#include "ac/spritecache.h"
#include "gfx/bitmap.h"
#include "gfx/blender.h"

using namespace AGS::Common;

extern int eip_guiobj;

extern SpriteCache spriteset;
extern GameSetupStruct game;

//=============================================================================
// Engine-specific implementation split out of acgui.cpp
//=============================================================================

int get_adjusted_spritewidth(int spr)
{
  return game.SpriteInfos[spr].Width;
}

int get_adjusted_spriteheight(int spr)
{
  return game.SpriteInfos[spr].Height;
}

void set_eip_guiobj(int eip)
{
  eip_guiobj = eip;
}

int get_eip_guiobj()
{
  return eip_guiobj;
}

namespace AGS
{
namespace Common
{

String GUI::ApplyTextDirection(const String &text)
{
    if (game.options[OPT_RIGHTLEFTWRITE] == 0)
        return text;
    String res_text = text;
    (get_uformat() == U_UTF8) ? res_text.ReverseUTF8() : res_text.Reverse();
    return res_text;
}

String GUI::TransformTextForDrawing(const String &text, bool translate, bool apply_direction)
{
    String res_text = translate ? get_translation(text.GetCStr()) : text;
    if (translate && apply_direction)
        res_text = ApplyTextDirection(res_text);
    return res_text;
}

size_t GUI::SplitLinesForDrawing(const String &text, bool is_translated, SplitLines &lines, int font, int width, size_t max_lines)
{
    // Use the engine's word wrap tool, to have RTL writing and other features
    const char *draw_text = skip_voiceover_token(text.GetCStr());
    return break_up_text_into_lines(draw_text, is_translated, lines, width, font);
}

void GUIControl::MarkChanged()
{
    _hasChanged = true;
    if (_parentID >= 0)
        guis[_parentID].MarkControlChanged();
}

void GUIControl::MarkVisualStateChanged()
{
    if (_parentID >= 0)
        guis[_parentID].MarkControlChanged();
}

void GUIControl::MarkPositionChanged(bool self_changed, bool transform_changed)
{
    _hasChanged |= self_changed || (transform_changed && GUI::Context.SoftwareRender);
    if (_parentID >= 0)
        guis[_parentID].NotifyControlPosition();
}

void GUIControl::MarkStateChanged(bool self_changed, bool parent_changed)
{
    _hasChanged |= self_changed;
    if (_parentID >= 0)
        guis[_parentID].NotifyControlState(_id, self_changed || parent_changed);
}

int GUILabel::PrepareTextToDraw()
{
    const bool is_translated = (_flags & kGUICtrl_Translated) != 0;
    _textToDraw = GUI::ResolveMacroTokens(is_translated ? get_translation(_text.GetCStr()) : _text);
    return GUI::SplitLinesForDrawing(_textToDraw, is_translated, Lines, _font, _innerRect.GetWidth());
}

void GUITextBox::DrawTextBoxContents(Bitmap *ds, int x, int y)
{
    _textToDraw = _text;
    bool reverse = false;
    // Text boxes input is never "translated" in regular sense,
    // but they use this flag to apply text direction
    if ((loaded_game_file_version >= kGameVersion_361) && ((_flags & kGUICtrl_Translated) != 0))
    {
        _textToDraw = GUI::ApplyTextDirection(_text);
        reverse = game.options[OPT_RIGHTLEFTWRITE] != 0;
    }

    FrameAlignment text_align = kAlignTopLeft;
    // 3.6.1 -> 3.6.2 applied text alignment based on text direction
    if ((loaded_game_file_version >= kGameVersion_361) && (loaded_game_file_version < kGameVersion_363_04))
    {
        text_align = reverse ? kAlignTopRight : kAlignTopLeft;
    }
    // 3.6.3+ have explicit text alignment property
    else if (loaded_game_file_version >= kGameVersion_363_04)
    {
        text_align = _textAlignment;
    }

    // Cursor is drawn only if textbox is currently enabled
    const bool draw_cursor = GUI::IsGUIEnabled(this);
    const int cursor_width = 5;
    const int offset_left = draw_cursor && reverse ? (cursor_width + 3) : 0;
    const int offset_right = draw_cursor && !reverse ? (cursor_width + 3) : 0;

    color_t text_color = ds->GetCompatibleColor(_textColor);
    Rect text_rc;
    Point text_at = GUI::CalcTextPosition(_textToDraw, _font,
        RectWH(_innerRect.Left + x + offset_left, _innerRect.Top + y, _innerRect.GetWidth() - offset_left - offset_right, _innerRect.GetHeight()),
        text_align, &text_rc);
    wouttext_outline(ds, text_at.X, text_at.Y, _font, text_color, _textToDraw.GetCStr());

    // Draw cursor
    if (draw_cursor)
    {
        int draw_at_x = reverse ? text_rc.Left - 3 - cursor_width : text_rc.Right + 3;
        int draw_at_y = text_rc.Top + get_font_height(_font) - 1;
        ds->DrawRect(Rect(draw_at_x, draw_at_y, draw_at_x + cursor_width, draw_at_y), text_color);
    }
}

void GUIListBox::PrepareTextToDraw(const String &text)
{
     _textToDraw = GUI::TransformTextForDrawing(text, (_flags & kGUICtrl_Translated) != 0,
         (loaded_game_file_version >= kGameVersion_361));
}

void GUIButton::PrepareTextToDraw()
{
    if (IsWrapText())
    {
        _textToDraw = _text;
        GUI::SplitLinesForDrawing(_text, (_flags & kGUICtrl_Translated) != 0, Lines, _font, _innerRect.GetWidth());
    }
    else
    {
        _textToDraw = GUI::TransformTextForDrawing(_text, (_flags & kGUICtrl_Translated) != 0,
            (loaded_game_file_version >= kGameVersion_361));
    }
}

} // namespace Common
} // namespace AGS
