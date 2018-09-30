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

    bool         IsInRightMargin(int x) const;
    int          GetItemAt(int x, int y) const;

    // Operations
    int          AddItem(const String &text);
    void         Clear();
    virtual void Draw(Bitmap *ds) override;
    int          InsertItem(int index, const String &text);
    void         RemoveItem(int index);
    void         SetFont(int font);
    void         SetItemText(int index, const String &textt);

    // Events
    virtual bool OnMouseDown() override;
    virtual void OnMouseMove(int x, int y) override;
    virtual void OnResized() override;

    // Serialization
    virtual void WriteToFile(Stream *out) override;
    virtual void ReadFromFile(Stream *in, GuiVersion gui_version) override;
    virtual void ReadFromSavegame(Common::Stream *in, GuiSvgVersion svg_ver);
    virtual void WriteToSavegame(Common::Stream *out) const;

// TODO: these members are currently public; hide them later
public:
    int32_t               ListBoxFlags;
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
    // A temporary solution for special drawing in the Editor
    void DrawItemsFix();
    void DrawItemsUnfix();
    void PrepareTextToDraw(const String &text);

    // prepared text buffer/cache
    String _textToDraw;
};

} // namespace Common
} // namespace AGS

extern std::vector<AGS::Common::GUIListBox> guilist;
extern int numguilist;

#endif // __AC_GUILISTBOX_H
