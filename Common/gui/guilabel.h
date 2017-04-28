//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
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

class GUILabel:public GUIObject
{
public:
    GUILabel();
    
    String       GetText() const;

    // Operations
    virtual void Draw(Bitmap *ds) override;
    void         SetText(const String &text);

    // Serialization
    virtual void WriteToFile(Stream *out) override;
    virtual void ReadFromFile(Stream *in, GuiVersion gui_version) override;

// TODO: these members are currently public; hide them later
public:
    String  Text;
    int32_t Font;
    color_t TextColor;
    int32_t TextAlignment;

private:
    void DrawAlignedText(Bitmap *g, int at_y, color_t text_color, const char *text);
    void PrepareTextToDraw();
    int  SplitLinesForDrawing();

    // prepared text buffer/cache
    String _textToDraw;
};

} // namespace Common
} // namespace AGS

extern std::vector<AGS::Common::GUILabel> guilabels;
extern int numguilabels;

#endif // __AC_GUILABEL_H
