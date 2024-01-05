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
#ifndef __AC_GUILABEL_H
#define __AC_GUILABEL_H

#include <vector>
#include "gui/guiobject.h"
#include "util/string.h"

namespace AGS
{
namespace Common
{

class GUILabel : public GUIObject
{
public:
    GUILabel();

    bool HasAlphaChannel() const override;
    // Gets label's text property in original set form (with macros etc)
    String       GetText() const;
    // Gets which macro are contained within label's text
    GUILabelMacro GetTextMacros() const;

    // Operations
    Rect CalcGraphicRect(bool clipped) override;
    void Draw(Bitmap *ds, int x = 0, int y = 0) override;
    void SetText(const String &text);

    // Serialization
    void ReadFromFile(Stream *in, GuiVersion gui_version) override;
    void WriteToFile(Stream *out) const override;
    void ReadFromSavegame(Common::Stream *in, GuiSvgVersion svg_ver) override;
    void WriteToSavegame(Common::Stream *out) const override;

// TODO: these members are currently public; hide them later
public:
    String  Text;
    int32_t Font;
    color_t TextColor;
    HorAlignment TextAlignment;

private:
    // Transforms the Text property to a drawn text, applies translation,
    // replaces macros, splits and wraps, etc; returns number of lines.
    int PrepareTextToDraw();

    // Information on macros contained within Text field
    GUILabelMacro _textMacro;
    // prepared text buffer/cache
    // TODO: cache split lines instead?
    String _textToDraw;
};

} // namespace Common
} // namespace AGS

extern std::vector<AGS::Common::GUILabel> guilabels;

#endif // __AC_GUILABEL_H
