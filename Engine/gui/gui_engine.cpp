//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
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
extern void replace_macro_tokens(const char*, String&);

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
    _hasChanged |= self_changed | (transform_changed && GUI::Context.SoftwareRender);
    if (_parentID >= 0)
        guis[_parentID].NotifyControlPosition();
}

void GUIControl::MarkStateChanged(bool self_changed, bool parent_changed)
{
    _hasChanged |= self_changed;
    if (_parentID >= 0)
        guis[_parentID].NotifyControlState(_id, self_changed | parent_changed);
}

int GUILabel::PrepareTextToDraw()
{
    const bool is_translated = (_flags & kGUICtrl_Translated) != 0;
    replace_macro_tokens(is_translated ? get_translation(_text.GetCStr()) : _text.GetCStr(), _textToDraw);
    return GUI::SplitLinesForDrawing(_textToDraw, is_translated, Lines, _font, _width);
}

void GUITextBox::DrawTextBoxContents(Bitmap *ds, int x, int y, color_t text_color)
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

    Line tpos = GUI::CalcTextPositionHor(_textToDraw, _font,
        x + 2, x + _width - 1, y + 2,
        reverse ? kAlignTopRight : kAlignTopLeft);
    wouttext_outline(ds, tpos.X1, tpos.Y1, _font, text_color, _textToDraw.GetCStr());

    if (GUI::IsGUIEnabled(this))
    {
        // draw a cursor
        const int cursor_width = 5;
        int draw_at_x = reverse ? tpos.X1 - 3 - cursor_width : tpos.X2 + 3;
        int draw_at_y = y + 1 + get_font_height(_font);
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
        GUI::SplitLinesForDrawing(_text, (_flags & kGUICtrl_Translated) != 0, Lines, _font, _width - _textPaddingHor * 2);
    }
    else
    {
        _textToDraw = GUI::TransformTextForDrawing(_text, (_flags & kGUICtrl_Translated) != 0,
            (loaded_game_file_version >= kGameVersion_361));
    }
}

} // namespace Common
} // namespace AGS
