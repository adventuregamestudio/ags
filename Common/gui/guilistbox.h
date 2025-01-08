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
#include "gui/guiobject.h"
#include "util/string.h"

namespace AGS
{
namespace Common
{

class GUIListBox : public GUIObject
{
public:
    GUIListBox();

    bool AreArrowsShown() const;
    bool IsBorderShown() const;
    bool IsSvgIndex() const;
    bool IsInRightMargin(int x) const;
    int  GetItemAt(int x, int y) const;

    // Operations
    int  AddItem(const String &text);
    void Clear();
    Rect CalcGraphicRect(bool clipped) override;
    void Draw(Bitmap *ds, int x = 0, int y = 0) override;
    int  InsertItem(int index, const String &text);
    void RemoveItem(int index);
    void SetShowArrows(bool on);
    void SetShowBorder(bool on);
    void SetSvgIndex(bool on); // TODO: work around this
    void SetFont(int font);
    void SetItemText(int index, const String &textt);

    // Events
    bool OnMouseDown() override;
    void OnMouseMove(int x, int y) override;
    void OnResized() override;

    // Serialization
    void ReadFromFile(Stream *in, GuiVersion gui_version) override;
    void WriteToFile(Stream *out) const override;
    void ReadFromSavegame(Common::Stream *in, GuiSvgVersion svg_ver) override;
    void WriteToSavegame(Common::Stream *out) const override;

// TODO: these members are currently public; hide them later
public:
    int32_t               Font;
    color_t               TextColor;
    HorAlignment          TextAlignment;
    color_t               SelectedBgColor;
    color_t               SelectedTextColor;
    int32_t               RowHeight;
    int32_t               VisibleItemCount;
    
    std::vector<String>   Items;
    std::vector<int16_t>  SavedGameIndex;
    int32_t               SelectedItem;
    int32_t               TopItem;
    Point                 MousePos;

    // TODO: remove these later
    int32_t               ItemCount;

private:
    int32_t               ListBoxFlags;

    // Updates dynamic metrics such as row height and others
    void UpdateMetrics();
    // Applies translation
    void PrepareTextToDraw(const String &text);

    // prepared text buffer/cache
    String _textToDraw;
};

} // namespace Common
} // namespace AGS

#endif // __AC_GUILISTBOX_H
