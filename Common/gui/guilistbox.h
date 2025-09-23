//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#ifndef __AC_GUILISTBOX_H
#define __AC_GUILISTBOX_H

#include <vector>
#include "gui/guicontrol.h"
#include "util/string.h"

namespace AGS
{
namespace Common
{

class GUIListBox : public GUIControl
{
public:
    GUIListBox();

    // Properties
    int  GetFont() const { return _font; }
    void SetFont(int font);
    int  GetTextColor() const { return _textColor; }
    void SetTextColor(int color);
    int  GetSelectedBgColor() const { return _selectedBgColor; }
    void SetSelectedBgColor(int color);
    int  GetSelectedTextColor() const { return _selectedTextColor; }
    void SetSelectedTextColor(int color);
    HorAlignment GetTextAlignment() const { return _textAlignment; }
    void SetTextAlignment(HorAlignment align);
    bool AreArrowsShown() const;
    bool IsBorderShown() const;
    bool IsSvgIndex() const;
    bool IsInRightMargin(int x) const;
    uint32_t GetItemCount() const { return _items.size(); }
    String GetItem(int index) const;
    int  GetSavedGameIndex(int index) const;
    int  GetItemAt(int x, int y) const;
    // NOTE: GetSelectedItem accounts for backwards-compatible behavior,
    // when the "selection" is kept at index 0 even when there's no items
    int  GetSelectedItem() const { return _items.size() > 0 ? _selectedItem : -1; }
    void SetSelectedItem(int index);
    int  GetTopItem() const { return _topItem; }
    void SetTopItem(int index);
    uint32_t GetVisibleItemCount() const { return _visibleItemCount; }
    void SetShowArrows(bool on);
    void SetShowBorder(bool on);
    void SetSvgIndex(bool on); // TODO: work around this
    void SetItemText(int index, const String &text);

    // Script Events
    uint32_t GetEventCount() const override;
    String GetEventArgs(uint32_t event) const override;
    String GetEventName(uint32_t event) const override;

    // Operations
    int AddItem(const String &text);
    int AddItem(const String &text, int save_slot);
    void Clear();
    Rect CalcGraphicRect(bool clipped) override;
    void Draw(Bitmap *ds, int x = 0, int y = 0) override;
    int InsertItem(int index, const String &text);
    void RemoveItem(int index);

    // Events
    bool OnMouseDown() override;
    void OnMouseMove(int mx, int my) override;
    void OnResized() override;

    // Serialization
    void ReadFromFile(Stream *in, GuiVersion gui_version) override;
    void WriteToFile(Stream *out) const override;
    void ReadFromSavegame(Common::Stream *in, GuiSvgVersion svg_ver) override;
    void WriteToSavegame(Common::Stream *out) const override;

private:
    static const color_t DefaultTextColor = 0;
    static const color_t DefaultSelectFgColor = 7;
    static const color_t DefaultSelectBgColor = 16;

    int                     _listBoxFlags = kListBox_DefFlags;
    int                     _font = 0;
    color_t                 _textColor = DefaultTextColor;
    HorAlignment            _textAlignment = kHAlignLeft;
    // FIXME: adjust these colors using GetStandardColor where is safe to access GuiContext
    color_t                 _selectedBgColor = DefaultSelectBgColor;
    color_t                 _selectedTextColor = DefaultSelectFgColor;
    int                     _rowHeight = 0;
    uint32_t                _visibleItemCount = 0u;

    std::vector<String>     _items;
    // CHECKME: why int16?
    std::vector<int16_t>    _savedGameIndex;
    int                     _selectedItem = -1;
    int                     _topItem = 0;
    Point                   _mousePos;

    // Updates dynamic metrics such as row height and others
    void UpdateMetrics();
    // Applies translation
    void PrepareTextToDraw(const String &text);

    static const int EventCount = 1;
    static String EventNames[EventCount];
    static String EventArgs[EventCount];

    // prepared text buffer/cache
    String _textToDraw;
};

} // namespace Common
} // namespace AGS

#endif // __AC_GUILISTBOX_H
