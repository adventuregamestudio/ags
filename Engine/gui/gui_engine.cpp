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
// Implementation from acgui.h and acgui.cpp specific to Engine runtime
//
//=============================================================================
#include "ac/game_version.h"
#include "ac/system.h"
#include "font/fonts.h"
#include "gui/guimain.h"
#include "gui/guibutton.h"
#include "gui/guiinv.h"
#include "gui/guilabel.h"
#include "gui/guilistbox.h"
#include "gui/guitextbox.h"
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

bool GUIMain::HasAlphaChannel() const
{
    if (this->BgImage > 0)
    {
        // alpha state depends on background image
        return is_sprite_alpha(this->BgImage);
    }
    if (this->BgColor > 0)
    {
        // not alpha transparent if there is a background color
        return false;
    }
    // transparent background, enable alpha blending
    return game.GetColorDepth() >= 24 &&
        // transparent background have alpha channel only since 3.2.0;
        // "classic" gui rendering mode historically had non-alpha transparent backgrounds
        // (3.2.0 broke the compatibility, now we restore it)
        loaded_game_file_version >= kGameVersion_320 && game.options[OPT_NEWGUIALPHA] != kGuiAlphaRender_Legacy;
}

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

bool is_sprite_alpha(int spr)
{
  return ((game.SpriteInfos[spr].Flags & SPF_ALPHACHANNEL) != 0);
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

size_t GUI::SplitLinesForDrawing(const char *text, bool is_translated, SplitLines &lines, int font, int width, size_t max_lines)
{
    // Use the engine's word wrap tool, to have RTL writing and other features
    const char *draw_text = skip_voiceover_token(text);
    return break_up_text_into_lines(draw_text, is_translated, lines, width, font);
}

void GUIObject::MarkChanged()
{
    _hasChanged = true;
    guis[ParentId].MarkControlChanged();
}

void GUIObject::MarkParentChanged()
{
    guis[ParentId].MarkControlChanged();
}

void GUIObject::MarkPositionChanged(bool self_changed)
{
    _hasChanged |= self_changed;
    guis[ParentId].NotifyControlPosition();
}

void GUIObject::MarkStateChanged(bool self_changed, bool parent_changed)
{
    _hasChanged |= self_changed;
    guis[ParentId].NotifyControlState(Id, self_changed | parent_changed);
}

void GUIObject::ClearChanged()
{
    _hasChanged = false;
}

int GUILabel::PrepareTextToDraw()
{
    const bool is_translated = (Flags & kGUICtrl_Translated) != 0;
    replace_macro_tokens(is_translated ? get_translation(Text.GetCStr()) : Text.GetCStr(), _textToDraw);
    return GUI::SplitLinesForDrawing(_textToDraw.GetCStr(), is_translated, Lines, Font, _width);
}

void GUITextBox::DrawTextBoxContents(Bitmap *ds, int x, int y, color_t text_color)
{
    _textToDraw = Text;
    bool reverse = false;
    // Text boxes input is never "translated" in regular sense,
    // but they use this flag to apply text direction
    if ((loaded_game_file_version >= kGameVersion_361) && ((Flags & kGUICtrl_Translated) != 0))
    {
        _textToDraw = GUI::ApplyTextDirection(Text);
        reverse = game.options[OPT_RIGHTLEFTWRITE] != 0;
    }

    Line tpos = GUI::CalcTextPositionHor(_textToDraw.GetCStr(), Font,
        x + 1 + get_fixed_pixel_size(1), x + _width - 1, y + 1 + get_fixed_pixel_size(1),
        reverse ? kAlignTopRight : kAlignTopLeft);
    wouttext_outline(ds, tpos.X1, tpos.Y1, Font, text_color, _textToDraw.GetCStr());

    if (IsGUIEnabled(this))
    {
        // draw a cursor
        const int cursor_width = get_fixed_pixel_size(5);
        int draw_at_x = reverse ? tpos.X1 - 3 - cursor_width : tpos.X2 + 3;
        int draw_at_y = y + 1 + get_font_height(Font);
        ds->DrawRect(Rect(draw_at_x, draw_at_y, draw_at_x + cursor_width, draw_at_y + (get_fixed_pixel_size(1) - 1)), text_color);
    }
}

void GUIListBox::PrepareTextToDraw(const String &text)
{
     _textToDraw = GUI::TransformTextForDrawing(text, (Flags & kGUICtrl_Translated) != 0,
         (loaded_game_file_version >= kGameVersion_361));
}

void GUIButton::PrepareTextToDraw()
{
    _textToDraw = GUI::TransformTextForDrawing(_text, (Flags & kGUICtrl_Translated) != 0,
        (loaded_game_file_version >= kGameVersion_361));
}

} // namespace Common
} // namespace AGS
