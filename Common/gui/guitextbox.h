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
#ifndef __AGS_CN_GUI__GUITEXTBOX_H
#define __AGS_CN_GUI__GUITEXTBOX_H
 
#include "gui/guiobject.h"
#include "util/array.h"
 
namespace AGS
{
namespace Common
{

enum GuiTextBoxFlags
{
    kGuiTextBox_NoBorder = 1
};

class GuiTextBox : public GuiObject
{
public:
    GuiTextBox();

    virtual void Draw(Common::Bitmap *ds);
 
    virtual void OnKeyPress(int);
 
    virtual void WriteToFile(Common::Stream *out);
    virtual void ReadFromFile(Common::Stream *in, GuiVersion gui_version);
    virtual void WriteToSavedGame(Common::Stream *out);
    virtual void ReadFromSavedGame(Common::Stream *in, RuntimeGuiVersion gui_version);
 
// TODO: these members are currently public; hide them later
public:
    int32_t TextFont;
    color_t TextColor;
    int32_t TextBoxFlags;
    String  Text;

private:
    void         DrawTextBoxContents(Bitmap *ds, color_t text_color);
};

} // namespace Common
} // namespace AGS
 
extern AGS::Common::ObjectArray<AGS::Common::GuiTextBox> guitext;
extern int numguitext;
 
#endif // __AGS_CN_GUI__GUITEXTBOX_H
