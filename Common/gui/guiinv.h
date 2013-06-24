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
#ifndef __AGS_CN_GUI__GUIINVWINDOW_H
#define __AGS_CN_GUI__GUIINVWINDOW_H

#include "gui/guiobject.h"
#include "util/array.h"

namespace AGS
{
namespace Common
{

class GuiInvWindow : public GuiObject
{
public:
    GuiInvWindow();

    int          GetCharacterId();

    // This function has distinct implementations in Engine and Editor
    virtual void Draw(Common::Bitmap *ds);

    virtual void OnMouseLeave();
    virtual void OnMouseOver();
    virtual void OnMouseUp();
    virtual void OnResized();

    virtual void WriteToFile(Common::Stream *out);
    virtual void ReadFromFile(Common::Stream *in, GuiVersion gui_version);
    virtual void WriteToSavedGame(Common::Stream *out);
    virtual void ReadFromSavedGame(Common::Stream *in, RuntimeGuiVersion gui_version);

private:
    void         CalculateNumCells();

// TODO: these members are currently public; hide them later
public:
    bool    IsMouseOver;
    int32_t CharacterId; // whose inventory (-1 = current player)
    int32_t ItemWidth;
    int32_t ItemHeight;
    int32_t ColumnCount;
    int32_t RowCount;
    int32_t TopItem;
};

} // namespace Common
} // namespace AGS

extern AGS::Common::ObjectArray<AGS::Common::GuiInvWindow> guiinv;
extern int numguiinv;

#endif // __AGS_CN_GUI__GUIINVWINDOW_H
