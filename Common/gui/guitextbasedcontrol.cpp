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
#include "gui/guitextbasedcontrol.h"

namespace AGS
{
namespace Common
{

void GUITextBasedControl::SetFont(int font)
{
    if (_font != font)
    {
        _font = font;
        OnTextFontChanged();
        MarkChanged();
    }
}

void GUITextBasedControl::SetTextColor(int color)
{
    if (_textColor != color)
    {
        _textColor = color;
        OnTextColorChanged();
        MarkChanged();
    }
}

void GUITextBasedControl::SetTextAlignment(FrameAlignment align)
{
    if (_textAlignment != align)
    {
        _textAlignment = align;
        MarkChanged();
    }
}

void GUITextFieldControl::SetText(const String &text)
{
    if (text == _text)
        return;
    _text = text;
    OnTextChanged();
    MarkChanged();
}

} // namespace Common
} // namespace AGS
