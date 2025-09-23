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
#include "ac/keycode.h"
#include "font/fonts.h"
#include "gui/guimain.h"
#include "gui/guitextbox.h"
#include "util/stream.h"
#include "util/string_utils.h"
#include "util/utf8.h"

namespace AGS
{
namespace Common
{

/* static */ String GUITextBox::EventNames[GUITextBox::EventCount] =
    { "Activate" };
/* static */ String GUITextBox::EventArgs[GUITextBox::EventCount] =
    { "GUIControl *control" };

GUITextBox::GUITextBox()
{
}

void GUITextBox::SetFont(int font)
{
    if (_font != font)
    {
        _font = font;
        MarkChanged();
    }
}

void GUITextBox::SetTextColor(int color)
{
    if (_textColor != color)
    {
        _textColor = color;
        MarkChanged();
    }
}

void GUITextBox::SetText(const String &text)
{
    if (_text != text)
    {
        _text = text;
        MarkChanged();
    }
}

bool GUITextBox::IsBorderShown() const
{
    return (_textBoxFlags & kTextBox_ShowBorder) != 0;
}

uint32_t GUITextBox::GetEventCount() const
{
    return EventCount;
}

String GUITextBox::GetEventName(uint32_t event) const
{
    if (event >= EventCount)
        return "";
    return EventNames[event];
}

String GUITextBox::GetEventArgs(uint32_t event) const
{
    if (event >= EventCount)
        return "";
    return EventArgs[event];
}

Rect GUITextBox::CalcGraphicRect(bool clipped)
{
    if (clipped)
        return RectWH(0, 0, _width, _height);

    // TODO: need to find a way to cache text position, or there'll be some repetition
    Rect rc = RectWH(0, 0, _width, _height);
    Point text_at(1 + 1, 1 + 1);
    Rect text_rc = GUI::CalcTextGraphicalRect(_text, _font, text_at);
    if (GUI::IsGUIEnabled(this))
    {
        // add a cursor
        Rect cur_rc = RectWH(
            text_rc.Right + 3,
            1 + get_font_height(_font),
            5,
            1);
        text_rc = SumRects(text_rc, cur_rc);
    }
    return SumRects(rc, text_rc);
}

void GUITextBox::Draw(Bitmap *ds, int x, int y)
{
    color_t text_color = ds->GetCompatibleColor(_textColor);
    color_t draw_color = ds->GetCompatibleColor(_textColor);
    if (IsBorderShown())
        ds->DrawRect(RectWH(x, y, _width, _height), draw_color);
    DrawTextBoxContents(ds, x, y, text_color);
}

// TODO: a shared utility function
static void Backspace(String &text)
{
    if (get_uformat() == U_UTF8)
    {// Find where the last utf8 char begins
        const char *ptr_end = text.GetCStr() + text.GetLength();
        const char *ptr_prev = Utf8::BackOneChar(ptr_end, text.GetCStr());
        text.ClipRight(ptr_end - ptr_prev);
    }
    else
    {
        text.ClipRight(1);
    }
}

bool GUITextBox::OnKeyPress(const KeyInput &ki)
{
    switch (ki.Key)
    {
    case eAGSKeyCodeReturn:
        _isActivated = true;
        return true;
    case eAGSKeyCodeBackspace:
        Backspace(_text);
        MarkChanged();
        return true;
    default: break;
    }

    if (ki.UChar == 0)
        return false; // not a textual event, don't handle

    if (get_uformat() == U_UTF8)
        _text.Append(ki.Text); // proper unicode char
    else if (ki.UChar < 256)
        _text.AppendChar(static_cast<uint8_t>(ki.UChar)); // ascii/ansi-range char in ascii mode
    else
        return true; // char from an unsupported range, don't print but still report as handled
    // if the new string is too long, remove the new character
    if (get_text_width(_text.GetCStr(), _font) > (_width - (6 + 5)))
        Backspace(_text);
    MarkChanged();
    return true;
}

void GUITextBox::SetShowBorder(bool on)
{
    if (on)
        _textBoxFlags |= kTextBox_ShowBorder;
    else
        _textBoxFlags &= ~kTextBox_ShowBorder;
}

// TODO: replace string serialization with StrUtil::ReadString and WriteString
// methods in the future, to keep this organized.
void GUITextBox::WriteToFile(Stream *out) const
{
    GUIControl::WriteToFile(out);
    StrUtil::WriteString(_text, out);
    out->WriteInt32(_font);
    out->WriteInt32(_textColor);
    out->WriteInt32(_textBoxFlags);
}

void GUITextBox::ReadFromFile(Stream *in, GuiVersion gui_version)
{
    GUIControl::ReadFromFile(in, gui_version);
    _text = StrUtil::ReadString(in);
    _font = in->ReadInt32();
    _textColor = in->ReadInt32();
    _textBoxFlags = in->ReadInt32();

    if (_textColor == 0)
        _textColor = 16; // FIXME: adjust this using GetStandardColor where is safe to access GuiContext
}

void GUITextBox::ReadFromSavegame(Stream *in, GuiSvgVersion svg_ver)
{
    GUIControl::ReadFromSavegame(in, svg_ver);
    _font = in->ReadInt32();
    _textColor = in->ReadInt32();
    _text = StrUtil::ReadString(in);
    if (svg_ver >= kGuiSvgVersion_350)
        _textBoxFlags = in->ReadInt32();
}

void GUITextBox::WriteToSavegame(Stream *out) const
{
    GUIControl::WriteToSavegame(out);
    out->WriteInt32(_font);
    out->WriteInt32(_textColor);
    StrUtil::WriteString(_text, out);
    out->WriteInt32(_textBoxFlags);
}

} // namespace Common
} // namespace AGS
