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
#include "ac/keycode.h"
#include "font/fonts.h"
#include "gui/guimain.h"
#include "gui/guitextbox.h"
#include "util/stream.h"
#include "util/string_utils.h"

#define GUITEXTBOX_LEGACY_TEXTLEN 200

std::vector<AGS::Common::GUITextBox> guitext;

namespace AGS
{
namespace Common
{

GUITextBox::GUITextBox()
{
    Font = 0;
    TextColor = 0;
    TextBoxFlags = kTextBox_DefFlags;

    _scEventCount = 1;
    _scEventNames[0] = "Activate";
    _scEventArgs[0] = "GUIControl *control";
}

bool GUITextBox::HasAlphaChannel() const
{
    return is_font_antialiased(Font);
}

bool GUITextBox::IsBorderShown() const
{
    return (TextBoxFlags & kTextBox_ShowBorder) != 0;
}

Rect GUITextBox::CalcGraphicRect(bool clipped)
{
    if (clipped)
        return RectWH(0, 0, _width, _height);

    // TODO: need to find a way to cache text position, or there'll be some repetition
    Rect rc = RectWH(0, 0, _width, _height);
    Point text_at(1 + get_fixed_pixel_size(1), 1 + get_fixed_pixel_size(1));
    Rect text_rc = GUI::CalcTextGraphicalRect(Text.GetCStr(), Font, text_at);
    if (IsGUIEnabled(this))
    {
        // add a cursor
        Rect cur_rc = RectWH(
            text_rc.Right + 3,
            1 + get_font_height(Font),
            get_fixed_pixel_size(5),
            get_fixed_pixel_size(1) - 1);
        text_rc = SumRects(text_rc, cur_rc);
    }
    return SumRects(rc, text_rc);
}

void GUITextBox::Draw(Bitmap *ds, int x, int y)
{
    color_t text_color = ds->GetCompatibleColor(TextColor);
    color_t draw_color = ds->GetCompatibleColor(TextColor);
    if (IsBorderShown())
    {
        ds->DrawRect(RectWH(x, y, _width, _height), draw_color);
        if (get_fixed_pixel_size(1) > 1)
        {
            ds->DrawRect(Rect(x + 1, y + 1, x + _width - get_fixed_pixel_size(1), y + _height - get_fixed_pixel_size(1)), draw_color);
        }
    }
    DrawTextBoxContents(ds, x, y, text_color);
}

// TODO: a shared utility function
static void Backspace(String &text)
{
    if (get_uformat() == U_UTF8)
    {// Find where the last utf8 char begins
        const char *ptr_end = text.GetCStr() + text.GetLength();
        const char *ptr = ptr_end - 1;
        for (; ptr > text.GetCStr() && ((*ptr & 0xC0) == 0x80); --ptr);
        text.ClipRight(ptr_end - ptr);
    }
    else
    {
        text.ClipRight(1);
    }
}

void GUITextBox::OnKeyPress(const KeyInput &ki)
{
    switch (ki.Key)
    {
    case eAGSKeyCodeReturn:
        IsActivated = true;
        return;
    case eAGSKeyCodeBackspace:
        Backspace(Text);
        MarkChanged();
        return;
    default: break;
    }

    if (ki.UChar == 0)
        return; // not a textual event
    if ((ki.UChar >= 128) && (!font_supports_extended_characters(Font)))
        return; // unsupported letter

    (get_uformat() == U_UTF8) ?
        Text.Append(ki.Text) :
        Text.AppendChar(ki.UChar);
    // if the new string is too long, remove the new character
    if (get_text_width(Text.GetCStr(), Font) > (_width - (6 + get_fixed_pixel_size(5))))
        Backspace(Text);
    MarkChanged();
}

void GUITextBox::SetShowBorder(bool on)
{
    if (on)
        TextBoxFlags |= kTextBox_ShowBorder;
    else
        TextBoxFlags &= ~kTextBox_ShowBorder;
}

// TODO: replace string serialization with StrUtil::ReadString and WriteString
// methods in the future, to keep this organized.
void GUITextBox::WriteToFile(Stream *out) const
{
    GUIObject::WriteToFile(out);
    StrUtil::WriteString(Text, out);
    out->WriteInt32(Font);
    out->WriteInt32(TextColor);
    out->WriteInt32(TextBoxFlags);
}

void GUITextBox::ReadFromFile(Stream *in, GuiVersion gui_version)
{
    GUIObject::ReadFromFile(in, gui_version);
    if (gui_version < kGuiVersion_350)
        Text.ReadCount(in, GUITEXTBOX_LEGACY_TEXTLEN);
    else
        Text = StrUtil::ReadString(in);
    Font = in->ReadInt32();
    TextColor = in->ReadInt32();
    TextBoxFlags = in->ReadInt32();
    // reverse particular flags from older format
    if (gui_version < kGuiVersion_350)
        TextBoxFlags ^= kTextBox_OldFmtXorMask;

    if (TextColor == 0)
        TextColor = 16;
}

void GUITextBox::ReadFromSavegame(Stream *in, GuiSvgVersion svg_ver)
{
    GUIObject::ReadFromSavegame(in, svg_ver);
    Font = in->ReadInt32();
    TextColor = in->ReadInt32();
    Text = StrUtil::ReadString(in);
    if (svg_ver >= kGuiSvgVersion_350)
        TextBoxFlags = in->ReadInt32();
}

void GUITextBox::WriteToSavegame(Stream *out) const
{
    GUIObject::WriteToSavegame(out);
    out->WriteInt32(Font);
    out->WriteInt32(TextColor);
    StrUtil::WriteString(Text, out);
    out->WriteInt32(TextBoxFlags);
}

} // namespace Common
} // namespace AGS
