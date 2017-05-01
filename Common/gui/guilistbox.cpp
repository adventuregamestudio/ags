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

#include "font/fonts.h"
#include "gui/guilistbox.h"
#include "gui/guimain.h"
#include "util/stream.h"

std::vector<AGS::Common::GUIListBox> guilist;
int numguilist = 0;

namespace AGS
{
namespace Common
{

GUIListBox::GUIListBox()
{
    ItemCount = 0;
    SelectedItem = 0;
    TopItem = 0;
    RowHeight = 0;
    VisibleItemCount = 0;
    Font = 0;
    TextColor = 0;
    BgColor = 7;
    ListBoxFlags = 0;
    SelectedBgColor = 16;
    TextAlignment = 0;

    _scEventCount = 1;
    _scEventNames[0] = "SelectionChanged";
    _scEventArgs[0] = "GUIControl *control";
}

int GUIListBox::GetItemAt(int x, int y) const
{
    if (RowHeight <= 0 || IsInRightMargin(x))
        return -1;

    int index = y / RowHeight + TopItem;
    if (index < 0 || index >= ItemCount)
        return -1;
    return index;
}

bool GUIListBox::IsInRightMargin(int x) const
{
    if (x >= (Width - get_fixed_pixel_size(6)) && (ListBoxFlags & kListBox_NoBorder) == 0 && (ListBoxFlags & kListBox_NoArrows) == 0)
        return 1;
    return 0;
}

int GUIListBox::AddItem(const String &text)
{
    guis_need_update = 1;
    Items.push_back(text);
    SavedGameIndex.push_back(-1);
    ItemCount++;
    return ItemCount - 1;
}

void GUIListBox::Clear()
{
    Items.clear();
    SavedGameIndex.clear();
    ItemCount = 0;
    SelectedItem = 0;
    TopItem = 0;
    guis_need_update = 1;
}

void GUIListBox::Draw(Common::Bitmap *ds)
{
    const int width  = Width - 1;
    const int height = Height - 1;
    const int pixel_size = get_fixed_pixel_size(1);

    check_font(&Font);
    color_t text_color = ds->GetCompatibleColor(TextColor);
    color_t draw_color = ds->GetCompatibleColor(TextColor);
    if ((ListBoxFlags & kListBox_NoBorder) == 0) {
        ds->DrawRect(Rect(X, Y, X + width + (pixel_size - 1), Y + height + (pixel_size - 1)), draw_color);
        if (pixel_size > 1)
            ds->DrawRect(Rect(X + 1, Y + 1, X + width, Y + height), draw_color);
    }

    int right_hand_edge = (X + width) - pixel_size - 1;

    // use SetFont to update the RowHeight and VisibleItemCount
    SetFont(Font);

    // draw the scroll bar in if necessary
    if (ItemCount > VisibleItemCount && (ListBoxFlags & kListBox_NoBorder) == 0 && (ListBoxFlags & kListBox_NoArrows) == 0)
    {
        int xstrt, ystrt;
        ds->DrawRect(Rect(X + width - get_fixed_pixel_size(7), Y, (X + (pixel_size - 1) + width) - get_fixed_pixel_size(7), Y + height), draw_color);
        ds->DrawRect(Rect(X + width - get_fixed_pixel_size(7), Y + height / 2, X + width, Y + height / 2 + (pixel_size - 1)), draw_color);

        xstrt = (X + width - get_fixed_pixel_size(6)) + (pixel_size - 1);
        ystrt = (Y + height - 3) - get_fixed_pixel_size(5);

        draw_color = ds->GetCompatibleColor(TextColor);
        ds->DrawTriangle(Triangle(xstrt, ystrt, xstrt + get_fixed_pixel_size(4), ystrt, 
                 xstrt + get_fixed_pixel_size(2),
                 ystrt + get_fixed_pixel_size(5)), draw_color);

        ystrt = Y + 3;
        ds->DrawTriangle(Triangle(xstrt, ystrt + get_fixed_pixel_size(5), 
                 xstrt + get_fixed_pixel_size(4), 
                 ystrt + get_fixed_pixel_size(5),
                 xstrt + get_fixed_pixel_size(2), ystrt), draw_color);

        right_hand_edge -= get_fixed_pixel_size(7);
    }

    DrawItemsFix();

    for (int item = 0; item < VisibleItemCount; ++item)
    {
        if (item + TopItem >= ItemCount)
            break;

        int at_y = Y + pixel_size + item * RowHeight;
        if (item + TopItem == SelectedItem)
        {
            text_color = ds->GetCompatibleColor(BgColor);
            if (SelectedBgColor > 0)
            {
                int stretch_to = (X + width) - pixel_size;
                // draw the SelectedItem item bar (if colour not transparent)
                draw_color = ds->GetCompatibleColor(SelectedBgColor);
                if ((VisibleItemCount < ItemCount) && ((ListBoxFlags & kListBox_NoBorder) == 0) && ((ListBoxFlags & kListBox_NoArrows) == 0))
                    stretch_to -= get_fixed_pixel_size(7);

                ds->FillRect(Rect(X + pixel_size, at_y, stretch_to, at_y + RowHeight - pixel_size), draw_color);
            }
        }
        else
            text_color = ds->GetCompatibleColor(TextColor);

        int item_index = item + TopItem;
        PrepareTextToDraw(Items[item_index]);

        GUI::DrawTextAlignedHor(ds, _textToDraw, Font, text_color, X + 1 + pixel_size, right_hand_edge, at_y + 1,
            ConvertLegacyGUIAlignment(TextAlignment));
    }

    DrawItemsUnfix();
}

int GUIListBox::InsertItem(int index, const String &text)
{
    if (index < 0 || index > ItemCount)
        return -1;

    Items.insert(Items.begin() + index, text);
    SavedGameIndex.insert(SavedGameIndex.begin() + index, -1);
    if (SelectedItem >= index)
        SelectedItem++;

    ItemCount++;
    guis_need_update = 1;
    return ItemCount - 1;
}

void GUIListBox::RemoveItem(int index)
{
    if (index < 0 || index >= ItemCount)
        return;

    Items.erase(Items.begin() + index);
    SavedGameIndex.erase(SavedGameIndex.begin() + index);
    ItemCount--;

    if (SelectedItem > index)
        SelectedItem--;
    if (SelectedItem >= ItemCount)
        SelectedItem = -1;
    guis_need_update = 1;
}

void GUIListBox::SetFont(int Font)
{
    Font = Font;
    RowHeight = getfontheight(Font) + get_fixed_pixel_size(2);
    VisibleItemCount = Height / RowHeight;
}

void GUIListBox::SetItemText(int index, const String &text)
{
    if (index >= 0 && index < ItemCount)
    {
        guis_need_update = 1;
        Items[index] = text;
    }
}

bool GUIListBox::OnMouseDown()
{
    if (IsInRightMargin(MousePos.X))
    {
        if (MousePos.Y < Height / 2 && TopItem > 0)
            TopItem--;
        if (MousePos.Y >= Height / 2 && ItemCount > TopItem + VisibleItemCount)
            TopItem++;
        return false;
    }

    int sel = GetItemAt(MousePos.X, MousePos.Y);
    if (sel < 0)
        return false;
    SelectedItem = sel;
    IsActivated = true;
    return false;
}

void GUIListBox::OnMouseMove(int x_, int y_)
{
    MousePos.X = x_ - X;
    MousePos.Y = y_ - Y;
}

void GUIListBox::OnResized() 
{
    if (RowHeight == 0)
    {
        check_font(&Font);
        SetFont(Font);
    }
    if (RowHeight > 0)
        VisibleItemCount = Height / RowHeight;
}

// TODO: replace string serialization with StrUtil::ReadString and WriteString
// methods in the future, to keep this organized.
void GUIListBox::WriteToFile(Stream *out)
{
    GUIObject::WriteToFile(out);
    out->WriteInt32(ItemCount);
    out->WriteInt32(SelectedItem);
    out->WriteInt32(TopItem);
    out->WriteInt32(MousePos.X);
    out->WriteInt32(MousePos.Y);
    out->WriteInt32(RowHeight);
    out->WriteInt32(VisibleItemCount);
    out->WriteInt32(Font);
    out->WriteInt32(TextColor);
    out->WriteInt32(BgColor);
    out->WriteInt32(ListBoxFlags);
    out->WriteInt32(TextAlignment);
    out->WriteInt32(0); // reserved1
    out->WriteInt32(SelectedBgColor);
    for (int i = 0; i < ItemCount; ++i)
    {
        Items[i].Write(out);
    }

    if (ListBoxFlags & kListBox_SvgIndex)
    {
        for (int i = 0; i < ItemCount; ++i)
            out->WriteInt16(SavedGameIndex[i]);
    }
}

void GUIListBox::ReadFromFile(Stream *in, GuiVersion gui_version)
{
    Clear();

    GUIObject::ReadFromFile(in, gui_version);
    ItemCount = in->ReadInt32();
    SelectedItem = in->ReadInt32();
    TopItem = in->ReadInt32();
    MousePos.X = in->ReadInt32();
    MousePos.Y = in->ReadInt32();
    RowHeight = in->ReadInt32();
    VisibleItemCount = in->ReadInt32();
    Font = in->ReadInt32();
    TextColor = in->ReadInt32();
    BgColor = in->ReadInt32();
    ListBoxFlags = in->ReadInt32();

    if (gui_version >= kGuiVersion_272b)
    {
        TextAlignment = in->ReadInt32();
        in->ReadInt32(); // reserved1
    }
    else
    {
        TextAlignment = kLegacyGUIAlign_Left;
    }

    if (gui_version >= kGuiVersion_unkn_107)
    {
        SelectedBgColor = in->ReadInt32();
    }
    else
    {
        SelectedBgColor = TextColor;
        if (SelectedBgColor == 0)
            SelectedBgColor = 16;
    }

    Items.resize(ItemCount);
    SavedGameIndex.resize(ItemCount, -1);
    for (int i = 0; i < ItemCount; ++i)
    {
        Items[i].Read(in);
    }

    if (gui_version >= kGuiVersion_272d && (ListBoxFlags & kListBox_SvgIndex))
    {
        for (int i = 0; i < ItemCount; ++i)
            SavedGameIndex[i] = in->ReadInt16();
    }

    if (TextColor == 0)
        TextColor = 16;
}

} // namespace Common
} // namespace AGS
