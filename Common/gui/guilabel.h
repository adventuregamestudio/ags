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
#ifndef __AC_GUILABEL_H
#define __AC_GUILABEL_H

#include <vector>
#include "gui/guitextbasedcontrol.h"

namespace AGS
{
namespace Common
{

class GUILabel : public GUITextFieldControl
{
public:
    GUILabel();

    // Properties
    bool HasAlphaChannel() const override;
    // Gets which macro are contained within label's text
    GUILabelMacro GetTextMacros() const { return _textMacro; }

    // Operations
    Rect CalcGraphicRect(bool clipped) override;
    void Draw(Bitmap *ds, int x = 0, int y = 0) override;

    // Serialization
    void ReadFromFile(Stream *in, GuiVersion gui_version) override;
    void ReadFromFile_Ext363(Stream *in, GuiVersion gui_version) override;
    void WriteToFile(Stream *out) const override;
    void ReadFromSavegame(Common::Stream *in, GuiSvgVersion svg_ver) override;
    void WriteToSavegame(Common::Stream *out) const override;

private:
    // Reports that the new text is set
    void OnTextChanged() override;

    // Transforms the Text property to a drawn text, applies translation,
    // replaces macros, splits and wraps, etc; returns number of lines.
    int PrepareTextToDraw();

    //static const int EventCount = 0;
    //static String EventNames[EventCount];
    //static String EventArgs[EventCount];

    // Information on macros contained within Text field
    GUILabelMacro _textMacro;
};

} // namespace Common
} // namespace AGS

#endif // __AC_GUILABEL_H
