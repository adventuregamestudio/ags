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

/* static */ ScriptEventSchema GUITextBox::_eventSchema = {{
        { "OnActivate", kTextBoxEvent_OnActivate }
    }};

GUITextBox::GUITextBox()
    : GUIControl(&GUITextBox::_eventSchema)
{
    _flags |= kGUICtrl_ShowBorder;
    _paddingX = 1;
    _paddingY = 1;
    UpdateControlRect();
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

void GUITextBox::SetTextAlignment(FrameAlignment align)
{
    if (_textAlignment != align)
    {
        _textAlignment = align;
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
    DrawControlFrame(ds, x, y);
    DrawTextBoxContents(ds, x, y);
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

void GUITextBox::ReadFromFile_Ext363(Stream *in, GuiVersion gui_version)
{
    GUIControl::ReadFromFile_Ext363(in, gui_version);
    _textAlignment = static_cast<FrameAlignment>(in->ReadInt32());
    in->ReadInt32(); // reserved
    in->ReadInt32();
    in->ReadInt32();
}

void GUITextBox::ReadFromSavegame(Stream *in, GuiSvgVersion svg_ver)
{
    GUIControl::ReadFromSavegame(in, svg_ver);
    _font = in->ReadInt32();
    _textColor = in->ReadInt32();
    _text = StrUtil::ReadString(in);
    if (svg_ver >= kGuiSvgVersion_350)
        _textBoxFlags = in->ReadInt32();

    if (svg_ver >= kGuiSvgVersion_36304)
    {
        _textAlignment = static_cast<FrameAlignment>(in->ReadInt32());
        in->ReadInt32(); // reserved
        in->ReadInt32();
        in->ReadInt32();
    }
    else
    {
        SetDefaultLooksFor363();
    }
}

void GUITextBox::WriteToSavegame(Stream *out) const
{
    GUIControl::WriteToSavegame(out);
    out->WriteInt32(_font);
    out->WriteInt32(_textColor);
    StrUtil::WriteString(_text, out);
    out->WriteInt32(_textBoxFlags);
    // kGuiSvgVersion_36304
    out->WriteInt32(_textAlignment);
    out->WriteInt32(0); // reserved
    out->WriteInt32(0);
    out->WriteInt32(0);
}

void GUITextBox::SetDefaultLooksFor363()
{
    if ((_textBoxFlags & kTextBox_ShowBorder) != 0)
        _flags |= kGUICtrl_ShowBorder;
    _borderColor = _textColor;
    _borderWidth = 1;
    _paddingX = 1;
    _paddingY = 1;
    UpdateControlRect();
}

} // namespace Common
} // namespace AGS
