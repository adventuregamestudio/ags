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

#include "font/fonts.h"
#include "gui/guilabel.h"
#include "gui/guimain.h"

namespace AGS
{
namespace Common
{

GuiLabel::GuiLabel()
{
    TextFont = 0;
    TextColor = 0;
    TextAlignment = kAlignLeft;

    SupportedEventCount = 0;
}

void GuiLabel::SetText(const String &text)
{
    Text = text;  
}

String GuiLabel::GetText() const
{
    return Text;
}

void GuiLabel::Draw(Common::Bitmap *ds)
{
    check_font(&TextFont);

    // TODO: need to find a way to cache text prior to drawing;
    // but that will require to update all gui controls when translation is changed in game
    PrepareTextToDraw();
    int line_count = SplitLinesForDrawing();

    color_t text_color = ds->GetCompatibleColor(TextColor);
    int text_height = wgettextheight("ZhypjIHQFb", TextFont) + 1;
    for (int i = 0, at_y = Frame.Top; i < line_count; ++i)
    {
        DrawAlignedText(ds, at_y, text_color, lines[i]);
        at_y += text_height;
        if (at_y > Frame.Bottom)
        {
            break;
        }
    }
}

void GuiLabel::DrawAlignedText(Common::Bitmap *ds, int at_y, color_t text_color, const char *text)
{
    int at_x = Frame.Left;
    if (TextAlignment & kAlignHCenter)
    {
        at_x += Frame.GetWidth() / 2 - wgettextwidth(text, TextFont) / 2;
    }
    else if (TextAlignment & kAlignRight)
    {
        at_x += Frame.GetWidth() - wgettextwidth(text, TextFont);
    }
    wouttext_outline(ds, at_x, at_y, TextFont, text_color, text);
}

void GuiLabel::WriteToFile(Stream *out)
{
    GuiObject::WriteToFile(out);
    out->WriteInt32(TextFont);
    out->WriteInt32(TextColor);
    out->WriteInt32(TextAlignment);
    Text.Write(out);
}

void GuiLabel::ReadFromFile(Stream *in, GuiVersion gui_version)
{
    GuiObject::ReadFromFile(in, gui_version);
    if (gui_version < kGuiVersion_272c)
    {
        Text.ReadCount(in, 200);
    }
    else if (gui_version < kGuiVersion_340_alpha)
    {
        size_t text_length = in->ReadInt32();
        Text.ReadCount(in, text_length);
    }
    TextFont = in->ReadInt32();
    TextColor = in->ReadInt32();

    if (gui_version < kGuiVersion_340_alpha)
    {
        LegacyGuiAlignment legacy_align = (LegacyGuiAlignment)in->ReadInt32();
        TextAlignment = ConvertLegacyAlignment(legacy_align);
    }
    else
    {
        TextAlignment = (Alignment)in->ReadInt32();
        Text.Read(in);
    }

    if (TextColor == 0)
    {
        TextColor = 16;
    }
    // All labels are translated at the moment
    Flags |= kGuiCtrl_Translated;
}

void GuiLabel::WriteToSavedGame(Stream *out)
{
    GuiObject::WriteToSavedGame(out);
    out->WriteInt32(TextFont);
    out->WriteInt32(TextColor);
    out->WriteInt32(TextAlignment);
    Text.Write(out);
}

void GuiLabel::ReadFromSavedGame(Common::Stream *in, RuntimeGuiVersion gui_version)
{
    GuiObject::ReadFromSavedGame(in, gui_version);
    TextFont = in->ReadInt32();
    TextColor = in->ReadInt32();
    TextAlignment = (Alignment)in->ReadInt32();
    Text.Read(in);
}

} // namespace Common
} // namespace AGS

AGS::Common::ObjectArray<AGS::Common::GuiLabel> guilabels;
int numguilabels = 0;
