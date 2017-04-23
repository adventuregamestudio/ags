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
#include "gui/guitextbox.h"
#include "gui/guimain.h"
#include "util/stream.h"

#define GUITEXTBOX_TEXT_LENGTH 200

std::vector<AGS::Common::GUITextBox> guitext;
int numguitext = 0;

namespace AGS
{
namespace Common
{

GUITextBox::GUITextBox()
{
    Font = 0;
    TextColor = 0;
    TextBoxFlags = 0;

    _scEventCount = 1;
    _scEventNames[0] = "Activate";
    _scEventArgs[0] = "GUIControl *control";
}

void GUITextBox::Draw(Bitmap *ds)
{
    check_font(&Font);
    color_t text_color = ds->GetCompatibleColor(TextColor);
    color_t draw_color = ds->GetCompatibleColor(TextColor);
    if ((TextBoxFlags & kTextBox_NoBorder) == 0)
    {
        ds->DrawRect(RectWH(X, Y, Width, Height), draw_color);
        if (1 > 1)
        {
            ds->DrawRect(Rect(X + 1, Y + 1, X + Width - 1, Y + Height - 1), draw_color);
        }
    }
    DrawTextBoxContents(ds, text_color);
}

void GUITextBox::OnKeyPress(int keycode)
{
    guis_need_update = 1;
    // TODO: use keycode constants
    // backspace, remove character
    if (keycode == 8)
    {
        Text.ClipRight(1);
        return;
    }
    // other key, continue
    if ((keycode >= 128) && (!font_supports_extended_characters(Font)))
        return;
    // return/enter
    if (keycode == 13)
    {
        IsActivated = true;
        return;
    }

    Text.AppendChar(keycode);
    // if the new string is too long, remove the new character
    if (wgettextwidth(Text, Font) > (Width - (6 + 5)))
        Text.ClipRight(1);
}

// TODO: replace string serialization with StrUtil::ReadString and WriteString
// methods in the future, to keep this organized.
void GUITextBox::WriteToFile(Stream *out)
{
    GUIObject::WriteToFile(out);
    Text.WriteCount(out, GUITEXTBOX_TEXT_LENGTH);
    out->WriteInt32(Font);
    out->WriteInt32(TextColor);
    out->WriteInt32(TextBoxFlags);
}

void GUITextBox::ReadFromFile(Stream *in, GuiVersion gui_version)
{
    GUIObject::ReadFromFile(in, gui_version);
    Text.ReadCount(in, GUITEXTBOX_TEXT_LENGTH);
    Font = in->ReadInt32();
    TextColor = in->ReadInt32();
    TextBoxFlags = in->ReadInt32();

    if (TextColor == 0)
        TextColor = 16;
}

} // namespace Common
} // namespace AGS
