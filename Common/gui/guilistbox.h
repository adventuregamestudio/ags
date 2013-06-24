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
#ifndef __AGS_CN_GUI__GUILISTBOX_H
#define __AGS_CN_GUI__GUILISTBOX_H

#include "gui/guiobject.h"
#include "util/array.h"

#define LEGACY_MAX_LISTBOX_ITEMS    200

namespace AGS
{
namespace Common
{

enum GuiListBoxFlags
{
    kGuiListBox_NoBorder            = 0x01,
    kGuiListBox_NoArrows            = 0x02,
    kGuiListBox_SavedGameIndexValid = 0x04,
};

class GuiListBox : public GuiObject
{
public:
    GuiListBox();

    bool         IsInRightMargin(int x);
    int          GetItemAt(int x, int y);

    int          AddItem(const String &text);
    void         Clear();
    virtual void Draw(Common::Bitmap *ds);
    int          InsertItem(int index, const String &text);
    void         RemoveItem(int index);
    void         SetFont(int font);
    void         SetItemText(int index, const String &textt);    
    
    virtual bool OnMouseDown();
    virtual void OnMouseMove(int x, int y);
    virtual void OnResized();

    virtual void WriteToFile(Common::Stream *out);
    virtual void ReadFromFile(Common::Stream *in, GuiVersion gui_version);
    virtual void WriteToSavedGame(Common::Stream *out);
    virtual void ReadFromSavedGame(Common::Stream *in, RuntimeGuiVersion gui_version);

// TODO: these members are currently public; hide them later
public:
    int32_t               ListBoxFlags;
    int32_t               TextFont;
    color_t               TextColor;
    Alignment             TextAlignment;
    color_t               BackgroundColor;
    color_t               SelectedBkgColor;
    int32_t               ItemCount;
    ObjectArray<String>   Items;
    Array<int16_t>        SavedGameIndex;
    int32_t               SelectedItem;
    int32_t               TopItem;
    int32_t               RowHeight;
    int32_t               VisibleItemCount;  
    Point                 MousePos;

private:
    // A temporary solution for special drawing in the Editor
    void DrawItemsFix();
    void DrawItemsUnfix();
    void PrepareTextToDraw(const String &text);

    // prepared text buffer/cache
    String  TextToDraw;
};

} // namespace Common
} // namespace AGS

extern AGS::Common::ObjectArray<AGS::Common::GuiListBox> guilist;
extern int numguilist;

#endif // __AGS_CN_GUI__GUILISTBOX_H
