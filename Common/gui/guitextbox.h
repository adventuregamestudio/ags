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
#ifndef __AC_GUITEXTBOX_H
#define __AC_GUITEXTBOX_H

#include <vector>
#include "gui/guiobject.h"
#include "util/string.h"

namespace AGS
{
namespace Common
{

class GUITextBox : public GUIObject
{
public:
    GUITextBox();

    // Properties
    int  GetFont() const { return _font; }
    void SetFont(int font);
    int  GetTextColor() const { return _textColor; }
    void SetTextColor(int color);
    const String &GetText() const { return _text; }
    void SetText(const String &text);
    bool HasAlphaChannel() const override;
    bool IsBorderShown() const;

    // Operations
    Rect CalcGraphicRect(bool clipped) override;
    void Draw(Bitmap *ds, int x = 0, int y = 0) override;
    void SetShowBorder(bool on);
 
    // Events
    bool OnKeyPress(const KeyInput &ki) override;
 
    // Serialization
    void ReadFromFile(Stream *in, GuiVersion gui_version) override;
    void WriteToFile(Stream *out) const override;
    void ReadFromSavegame(Stream *in, GuiSvgVersion svg_ver) override;
    void WriteToSavegame(Stream *out) const override;

private:
    int     _font = 0;
    String  _text;
    color_t _textColor = 0;
    int     _textBoxFlags = kTextBox_DefFlags;
    String  _textToDraw;

    void DrawTextBoxContents(Bitmap *ds, int x, int y, color_t text_color);
};

} // namespace Common
} // namespace AGS

#endif // __AC_GUITEXTBOX_H
