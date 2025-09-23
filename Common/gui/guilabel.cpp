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
#include <algorithm>
#include "ac/game_version.h"
#include "font/fonts.h"
#include "gui/guilabel.h"
#include "gui/guimain.h"
#include "util/stream.h"
#include "util/string_utils.h"

namespace AGS
{
namespace Common
{

GUILabel::GUILabel()
{
}

void GUILabel::SetFont(int font)
{
    if (_font != font)
    {
        _font = font;
        MarkChanged();
    }
}

void GUILabel::SetTextColor(int color)
{
    if (_textColor != color)
    {
        _textColor = color;
        MarkChanged();
    }
}

void GUILabel::SetTextAlignment(FrameAlignment align)
{
    if (_textAlignment != align)
    {
        _textAlignment = align;
        MarkChanged();
    }
}

Rect GUILabel::CalcGraphicRect(bool clipped)
{
    if (clipped)
        return RectWH(0, 0, _width, _height);

    // TODO: need to find a way to text position, or there'll be some repetition
    // have to precache text and size on some events:
    // - translation change
    // - macro value change (overhotspot etc)
    Rect rc = RectWH(0, 0, _width, _height);
    if (PrepareTextToDraw() == 0)
        return rc;
    const int linespacing = get_font_linespacing(_font);
    const bool limit_by_label_frame = true;

    Rect text_rc = GUI::CalcTextGraphicalRect(Lines.GetVector(), Lines.Count(), _font, linespacing,
        RectWH(0, 0, _width, _height), (FrameAlignment)_textAlignment, limit_by_label_frame);
    return SumRects(rc, text_rc);
}

void GUILabel::Draw(Bitmap *ds, int x, int y)
{
    // TODO: need to find a way to cache text prior to drawing;
    // but that will require to update all gui controls when translation is changed in game
    if (PrepareTextToDraw() == 0)
        return;

    color_t text_color = ds->GetCompatibleColor(_textColor);
    const int linespacing = get_font_linespacing(_font);
    const bool limit_by_label_frame = true;
    GUI::DrawTextLinesAligned(ds, Lines.GetVector(), Lines.Count(), _font, linespacing, text_color,
        RectWH(x, y, _width, _height), (FrameAlignment)_textAlignment, limit_by_label_frame);
}

void GUILabel::SetText(const String &text)
{
    if (text == _text)
        return;
    _text = text;
    // Check for macros within text
    _textMacro = GUI::FindLabelMacros(_text);
    MarkChanged();
}

// TODO: replace string serialization with StrUtil::ReadString and WriteString
// methods in the future, to keep this organized.
void GUILabel::WriteToFile(Stream *out) const
{
    GUIControl::WriteToFile(out);
    StrUtil::WriteString(_text, out);
    out->WriteInt32(_font);
    out->WriteInt32(_textColor);
    out->WriteInt32(_textAlignment);
}

void GUILabel::ReadFromFile(Stream *in, GuiVersion gui_version)
{
    GUIControl::ReadFromFile(in, gui_version);

    _text = StrUtil::ReadString(in);
    _font = in->ReadInt32();
    _textColor = in->ReadInt32();
    _textAlignment = (FrameAlignment)in->ReadInt32();

    if (_textColor == 0)
        _textColor = 16; // FIXME: adjust this using GetStandardColor where is safe to access GuiContext

    _textMacro = GUI::FindLabelMacros(_text);
}

void GUILabel::ReadFromSavegame(Stream *in, GuiSvgVersion svg_ver)
{
    GUIControl::ReadFromSavegame(in, svg_ver);
    _font = in->ReadInt32();
    _textColor = in->ReadInt32();
    _text = StrUtil::ReadString(in);
    if (svg_ver >= kGuiSvgVersion_350)
        _textAlignment = (FrameAlignment)in->ReadInt32();

    _textMacro = GUI::FindLabelMacros(_text);
}

void GUILabel::WriteToSavegame(Stream *out) const
{
    GUIControl::WriteToSavegame(out);
    out->WriteInt32(_font);
    out->WriteInt32(_textColor);
    StrUtil::WriteString(_text, out);
    out->WriteInt32(_textAlignment);
}

} // namespace Common
} // namespace AGS
