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
//
// GUITextBasedControl is a parent class for controls that have any kind of
// text. It controls the generic text appearance.
// GUITextFieldControl is a parent class for controls which text is represented
// by a single text property (label, button, and alike).
//
//=============================================================================
#ifndef __AGS_CN_GUI__GUITEXTBASEDCONTROL_H
#define __AGS_CN_GUI__GUITEXTBASEDCONTROL_H

#include "gui/guiobject.h"
#include "util/string.h"

namespace AGS
{
namespace Common
{

class GUITextBasedControl : public GUIObject
{
public:
    GUITextBasedControl() = default;
    virtual ~GUITextBasedControl() = default;

    // Properties
    int  GetFont() const { return _font; }
    void SetFont(int font);
    int  GetTextColor() const { return _textColor; }
    void SetTextColor(int color);
    FrameAlignment GetTextAlignment() const { return _textAlignment; }
    void SetTextAlignment(FrameAlignment align);

protected:
    // Reports that the text color has changed
    virtual void OnTextColorChanged() { /* do nothing */ };
    // Reports that the text font has changed
    virtual void OnTextFontChanged() { /* do nothing */ };

    int     _font = 0;
    color_t _textColor = 0;
    FrameAlignment _textAlignment = kAlignTopLeft;

    // prepared text buffer/cache
    // TODO: cache split lines instead?
    String _textToDraw;
};

class GUITextFieldControl : public GUITextBasedControl
{
public:
    GUITextFieldControl() = default;

    // Gets control's text property
    const String &GetText() const { return _text; }
    void SetText(const String &text);

protected:
    // Reports that the new text is set to let child classes react to this change
    virtual void OnTextChanged() { /* do nothing */ };

    // Text property set by user
    String  _text;
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_GUI__GUITEXTBASEDCONTROL_H
