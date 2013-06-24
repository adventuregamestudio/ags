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
//
// 
//
//=============================================================================
#ifndef __AGS_CN_GUI__GUILABEL_H
#define __AGS_CN_GUI__GUILABEL_H

#include "gui/guiobject.h"
#include "util/array.h"

#define LEGACY_MAX_GUILABEL_TEXT_LEN    2048

namespace AGS
{
namespace Common
{

class GuiLabel:public GuiObject
{
public:
    GuiLabel();
    
    String       GetText() const;

    virtual void Draw(Common::Bitmap *ds);
    void         SetText(const String &text);

    virtual void WriteToFile(Common::Stream *out);
    virtual void ReadFromFile(Common::Stream *in, GuiVersion gui_version);
    virtual void WriteToSavedGame(Common::Stream *out);
    virtual void ReadFromSavedGame(Common::Stream *in, RuntimeGuiVersion gui_version);

// TODO: these members are currently public; hide them later
public:
    String      Text;
    int32_t     TextFont;
    color_t     TextColor;
    Alignment   TextAlignment;

  
private:
    void DrawAlignedText(Common::Bitmap *g, int yy, color_t text_color, const char *text);
    void PrepareTextToDraw();
    int  SplitLinesForDrawing();

    // prepared text buffer/cache
    String      TextToDraw;
};

} // namespace Common
} // namespace AGS

extern AGS::Common::ObjectArray<AGS::Common::GuiLabel> guilabels;
extern int numguilabels;

#endif // __AGS_CN_GUI__GUILABEL_H
