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
#include <algorithm>
#include "ac/game_version.h"
#include "font/fonts.h"
#include "gui/guilabel.h"
#include "gui/guimain.h"
#include "util/stream.h"
#include "util/string_utils.h"

std::vector<AGS::Common::GUILabel> guilabels;

namespace AGS
{
namespace Common
{

GUILabel::GUILabel()
{
    Font = 0;
    TextColor = 0;
    TextAlignment = kHAlignLeft;

    _scEventCount = 0;
}

String GUILabel::GetText() const
{
    return Text;
}

GUILabelMacro GUILabel::GetTextMacros() const
{
    return _textMacro;
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
    const int linespacing = get_font_linespacing(Font);
    const bool limit_by_label_frame = true;
    int at_y = 0;
    Line max_line;
    for (size_t i = 0;
        i < Lines.Count() && (!limit_by_label_frame || at_y <= _height);
        ++i, at_y += linespacing)
    {
        Line lpos = GUI::CalcTextPositionHor(Lines[i].GetCStr(), Font, 0, 0 + _width - 1, at_y,
            (FrameAlignment)TextAlignment);
        max_line.X2 = std::max(max_line.X2, lpos.X2);
    }
    return SumRects(rc, RectWH(0, 0, max_line.X2 - max_line.X1 + 1, at_y - linespacing + get_font_surface_height(Font)));
}

void GUILabel::Draw(Bitmap *ds, int x, int y)
{
    // TODO: need to find a way to cache text prior to drawing;
    // but that will require to update all gui controls when translation is changed in game
    if (PrepareTextToDraw() == 0)
        return;

    color_t text_color = ds->GetCompatibleColor(TextColor);
    const int linespacing = get_font_linespacing(Font);
    const bool limit_by_label_frame = true;
    int at_y = y;
    for (size_t i = 0;
        i < Lines.Count() && (!limit_by_label_frame || at_y <= y + _height);
        ++i, at_y += linespacing)
    {
        GUI::DrawTextAlignedHor(ds, Lines[i].GetCStr(), Font, text_color, x, x + _width - 1, at_y,
            (FrameAlignment)TextAlignment);
    }
}

void GUILabel::SetText(const String &text)
{
    if (text == Text)
        return;
    Text = text;
    // Check for macros within text
    _textMacro = GUI::FindLabelMacros(Text);
    MarkChanged();
}

// TODO: replace string serialization with StrUtil::ReadString and WriteString
// methods in the future, to keep this organized.
void GUILabel::WriteToFile(Stream *out) const
{
    GUIObject::WriteToFile(out);
    StrUtil::WriteString(Text, out);
    out->WriteInt32(Font);
    out->WriteInt32(TextColor);
    out->WriteInt32(TextAlignment);
}

void GUILabel::ReadFromFile(Stream *in, GuiVersion gui_version)
{
    GUIObject::ReadFromFile(in, gui_version);

    Text = StrUtil::ReadString(in);
    Font = in->ReadInt32();
    TextColor = in->ReadInt32();
    TextAlignment = (HorAlignment)in->ReadInt32();

    if (TextColor == 0)
        TextColor = 16;

    _textMacro = GUI::FindLabelMacros(Text);
}

void GUILabel::ReadFromSavegame(Stream *in, GuiSvgVersion svg_ver)
{
    GUIObject::ReadFromSavegame(in, svg_ver);
    Font = in->ReadInt32();
    TextColor = in->ReadInt32();
    Text = StrUtil::ReadString(in);
    if (svg_ver >= kGuiSvgVersion_350)
        TextAlignment = (HorAlignment)in->ReadInt32();

    _textMacro = GUI::FindLabelMacros(Text);
}

void GUILabel::WriteToSavegame(Stream *out) const
{
    GUIObject::WriteToSavegame(out);
    out->WriteInt32(Font);
    out->WriteInt32(TextColor);
    StrUtil::WriteString(Text, out);
    out->WriteInt32(TextAlignment);
}

} // namespace Common
} // namespace AGS
