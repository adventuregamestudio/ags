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
#ifndef __AC_GUITEXTBOX_H
#define __AC_GUITEXTBOX_H

#include <vector>
#include "gui/guitextbasedcontrol.h"

namespace AGS
{
namespace Common
{

class GUITextBox : public GUITextFieldControl
{
public:
    GUITextBox();

    // Properties
    int  GetTextBoxFlags() const { return _textBoxFlags; }
    bool HasAlphaChannel() const override;

    // Script Events
    uint32_t GetEventCount() const override;
    String GetEventArgs(uint32_t event) const override;
    String GetEventName(uint32_t event) const override;

    // Operations
    Rect CalcGraphicRect(bool clipped) override;
    void Draw(Bitmap *ds, int x = 0, int y = 0) override;
 
    // Events
    bool OnKeyPress(const KeyInput &ki) override;
 
    // Serialization
    void ReadFromFile(Stream *in, GuiVersion gui_version) override;
    void ReadFromFile_Ext363(Stream *in, GuiVersion gui_version) override;
    void WriteToFile(Stream *out) const override;
    void ReadFromSavegame(Stream *in, GuiSvgVersion svg_ver) override;
    void WriteToSavegame(Stream *out) const override;

    // Upgrades the GUI control to default looks for 3.6.3
    void SetDefaultLooksFor363() override;

private:
    void DrawTextBoxContents(Bitmap *ds, int x, int y);

    static const uint32_t EventCount = 1;
    static String EventNames[EventCount];
    static String EventArgs[EventCount];

    int     _textBoxFlags = kTextBox_DefFlags;
};

} // namespace Common
} // namespace AGS

#endif // __AC_GUITEXTBOX_H
