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

#include "font/agsfontrenderer.h"
#include "font/fonts.h"
#include "gui/guimain.h"
#include "gui/guitextbox.h"

namespace AGS
{
namespace Common
{

GuiTextBox::GuiTextBox()
{
    TextFont = 0;
    TextColor = 0;
    TextBoxFlags = 0;

    SupportedEventCount = 1;
    EventNames[0] = "Activate";
    EventArgs[0] = "GUIControl *control";
}

void GuiTextBox::Draw(Bitmap *ds)
{
    check_font(&TextFont);
    color_t text_color = ds->GetCompatibleColor(TextColor);
    color_t draw_color = ds->GetCompatibleColor(TextColor);
    if ((TextBoxFlags & kGuiTextBox_NoBorder) == 0)
    {
        ds->DrawRect(Rect(Frame.Left, Frame.Top, Frame.Left + Frame.GetWidth() - 1, Frame.Top + Frame.GetHeight() - 1), draw_color);
        if (get_fixed_pixel_size(1) > 1)
        {
            ds->DrawRect(Rect(Frame.Left + 1, Frame.Top + 1, Frame.Left + Frame.GetWidth() - get_fixed_pixel_size(1), Frame.Top + Frame.GetHeight() - get_fixed_pixel_size(1)), draw_color);
        }
    }
    DrawTextBoxContents(ds, text_color);
}

void GuiTextBox::OnKeyPress(int kp)
{
    guis_need_update = 1;
    // backspace, remove character
    if (kp == 8)
    {
        Text.ClipRight(1);
        return;
    }
    // other key, continue
    if (kp >= 128 && !fontRenderers[TextFont]->SupportsExtendedCharacters(TextFont))
    {
        return;
    }

    if (kp == 13)
    {
        IsActivated = true;
        return;
    }

    Text.AppendChar(kp);
    // if the new string is too long, remove the new character
    if (wgettextwidth(Text, TextFont) > (Frame.GetWidth() - (6 + get_fixed_pixel_size(5))))
    {
        Text.ClipRight(1);
    }
}

void GuiTextBox::WriteToFile(Stream *out)
{
    GuiObject::WriteToFile(out);
    out->WriteInt32(TextBoxFlags);
    out->WriteInt32(TextFont);
    out->WriteInt32(TextColor);
    Text.Write(out);
}

void GuiTextBox::ReadFromFile(Stream *in, GuiVersion gui_version)
{
    GuiObject::ReadFromFile(in, gui_version);
    if (gui_version < kGuiVersion_340_alpha)
    {
        Text.ReadCount(in, 200);
        TextFont = in->ReadInt32();
        TextColor = in->ReadInt32();
        TextBoxFlags = in->ReadInt32();
    }
    else
    {
        TextBoxFlags = in->ReadInt32();
        TextFont = in->ReadInt32();
        TextColor = in->ReadInt32();
        Text.Read(in);
    }

    if (TextColor == 0)
    {
        TextColor = 16;
    }
}

void GuiTextBox::WriteToSavedGame(Stream *out)
{
    GuiObject::WriteToSavedGame(out);
    out->WriteInt32(TextBoxFlags);
    out->WriteInt32(TextFont);
    out->WriteInt32(TextColor);
    Text.Write(out);
}

void GuiTextBox::ReadFromSavedGame(Common::Stream *in, RuntimeGuiVersion gui_version)
{
    GuiObject::ReadFromSavedGame(in, gui_version);
    TextBoxFlags = in->ReadInt32();
    TextFont = in->ReadInt32();
    TextColor = in->ReadInt32();
    Text.Read(in);
}

} // namespace Common
} // namespace AGS

AGS::Common::ObjectArray<AGS::Common::GuiTextBox> guitext;
int numguitext = 0;
