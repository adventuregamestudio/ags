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
#ifndef __AC_GUIINV_H
#define __AC_GUIINV_H

#include <vector>
#include "gui/guiobject.h"

namespace AGS
{
namespace Common
{

class GUIInvWindow : public GUIObject
{
public:
    GUIInvWindow();

    // Properties
    int  GetItemWidth() const { return _itemWidth; }
    int  GetItemHeight() const { return _itemHeight; }
    void SetItemWidth(int itemw) { SetItemDimensions(itemw, _itemHeight); }
    void SetItemHeight(int itemh) { SetItemDimensions(_itemWidth, itemh); }
    void SetItemDimensions(int itemw, int itemh);
    int  GetRowCount() const { return _rowCount; }
    int  GetColCount() const { return _colCount; }
    int  GetTopItem() const { return _topItem; }
    void SetTopItem(int item);
    bool HasAlphaChannel() const override;
    // This function has distinct implementations in Engine and Editor
    int  GetCharacterID() const;
    void SetCharacterID(int charid);

    // Operations
    // This function has distinct implementations in Engine and Editor
    void Draw(Bitmap *ds, int x = 0, int y = 0) override;

    // Events
    void OnMouseEnter() override;
    void OnMouseLeave() override;
    void OnMouseUp() override;
    void OnResized() override;

    // Serialization
    void ReadFromFile(Stream *in, GuiVersion gui_version) override;
    void ReadFromFile_Ext363(Stream *in, GuiVersion gui_version) override;
    void WriteToFile(Stream *out) const override;
    void ReadFromSavegame(Common::Stream *in, GuiSvgVersion svg_ver) override;
    void WriteToSavegame(Common::Stream *out) const override;

private:
    // Internal control's region (content region) was resized
    void OnContentRectChanged() override;

    void CalculateNumCells();

    static const int DefaultItemWidth = 40;
    static const int DefaultItemHeight = 22;

    //static const int EventCount = 0;
    //static String EventNames[EventCount];
    //static String EventArgs[EventCount];

    bool    _isMouseOver = false;
    int     _charID = -1; // whose inventory (-1 = current player)
    int     _itemWidth = DefaultItemWidth;
    int     _itemHeight = DefaultItemHeight;
    int     _colCount = 0;
    int     _rowCount = 0;
    int     _topItem = 0;
};

} // namespace Common
} // namespace AGS

#endif // __AC_GUIINV_H
