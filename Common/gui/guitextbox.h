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

    bool HasAlphaChannel() const override;
    bool IsBorderShown() const;

    // Operations
    Rect CalcGraphicRect(bool clipped) override;
    void Draw(Bitmap *ds, int x = 0, int y = 0) override;
    void SetShowBorder(bool on);
 
    // Events
    void OnKeyPress(const KeyInput &ki) override;
 
    // Serialization
    void ReadFromFile(Stream *in, GuiVersion gui_version) override;
    void WriteToFile(Stream *out) const override;
    void ReadFromSavegame(Stream *in, GuiSvgVersion svg_ver) override;
    void WriteToSavegame(Stream *out) const override;
 
// TODO: these members are currently public; hide them later
public:
    int32_t Font;
    String  Text;
    color_t TextColor;

private:
    int32_t TextBoxFlags;
    String  _textToDraw;

    void DrawTextBoxContents(Bitmap *ds, int x, int y, color_t text_color);
};

} // namespace Common
} // namespace AGS

extern std::vector<AGS::Common::GUITextBox> guitext;

#endif // __AC_GUITEXTBOX_H
