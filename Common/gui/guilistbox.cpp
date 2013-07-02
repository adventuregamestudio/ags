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

namespace AGS
{
namespace Common
{

GuiListBox::GuiListBox()
{
    ItemCount = 0;
    SelectedItem = 0;
    TopItem = 0;
    RowHeight = 0;
    VisibleItemCount = 0;
    TextFont = 0;
    TextColor = 0;
    BackgroundColor = 7;
    ListBoxFlags = 0;
    SelectedBkgColor = 16;
    TextAlignment = kAlignNone;

    SupportedEventCount = 1;
    EventNames[0] = "SelectionChanged";
    EventArgs[0] = "GUIControl *control";
}

int GuiListBox::GetItemAt(int x, int y)
{
    if (IsInRightMargin(x))
    {
        return -1;
    }
    int index = y / RowHeight + TopItem;
    if (index < 0 || index >= ItemCount)
    {
        return -1;
    }
    return index;
}

bool GuiListBox::IsInRightMargin(int x)
{
    if (x >= (Frame.GetWidth() - get_fixed_pixel_size(6)) &&
        (ListBoxFlags & kGuiListBox_NoBorder) == 0 && (ListBoxFlags & kGuiListBox_NoArrows) == 0)
    {
        return true;
    }
    return false;
}

int GuiListBox::AddItem(const String &text)
{
    guis_need_update = 1;
    Items.Append(text);
    SavedGameIndex.Append(-1);
    ItemCount++;
    return ItemCount - 1;
}

void GuiListBox::Clear()
{
    Items.Empty();
    ItemCount = 0;
    SelectedItem = 0;
    TopItem = 0;
    guis_need_update = 1;
}

void GuiListBox::Draw(Common::Bitmap *ds)
{
    Rect frame = RectWH(Frame.Left, Frame.Top, Frame.GetWidth() - 1, Frame.GetHeight() - 1);
    int pixel_size = get_fixed_pixel_size(1);

    check_font(&TextFont);
    color_t text_color = ds->GetCompatibleColor(TextColor);
    color_t draw_color = ds->GetCompatibleColor(TextColor);
    if ((ListBoxFlags & kGuiListBox_NoBorder) == 0)
    {
        ds->DrawRect(Rect(frame.Left, frame.Top, frame.Left + frame.GetWidth() + (pixel_size - 1), frame.Top + frame.GetHeight() + (pixel_size - 1)), draw_color);
        if (pixel_size > 1)
        {
            ds->DrawRect(Rect(frame.Left + 1, frame.Top + 1, frame.Left + frame.GetWidth(), frame.Top + frame.GetHeight()), draw_color);
        }
    }

    int right_hand_edge = (frame.Left + frame.GetWidth()) - pixel_size - 1;
    // use SetFont to update the RowHeight and VisibleItemCount
    SetFont(TextFont);
    // draw the scroll bar in if necessary
    if (ItemCount > VisibleItemCount && (ListBoxFlags & kGuiListBox_NoBorder) == 0 &&
        (ListBoxFlags & kGuiListBox_NoArrows) == 0)
    {
        int xstrt, ystrt;
        ds->DrawRect(Rect(frame.Left + frame.GetWidth() - get_fixed_pixel_size(7), frame.Top, (frame.Left + (pixel_size - 1) + frame.GetWidth()) - get_fixed_pixel_size(7), frame.Top + frame.GetHeight()), draw_color);
        ds->DrawRect(Rect(frame.Left + frame.GetWidth() - get_fixed_pixel_size(7), frame.Top + frame.GetHeight() / 2, frame.Left + frame.GetWidth(), frame.Top + frame.GetHeight() / 2 + (pixel_size - 1)), draw_color);

        xstrt = (frame.Left + frame.GetWidth() - get_fixed_pixel_size(6)) + (pixel_size - 1);
        ystrt = (frame.Top + frame.GetHeight() - 3) - get_fixed_pixel_size(5);

        draw_color = ds->GetCompatibleColor(TextColor);
        ds->DrawTriangle(Triangle(xstrt, ystrt, xstrt + get_fixed_pixel_size(4), ystrt, 
            xstrt + get_fixed_pixel_size(2),
            ystrt + get_fixed_pixel_size(5)), draw_color);

        ystrt = frame.Top + 3;
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
        {
            break;
        }

        int at_y = frame.Top + pixel_size + item * RowHeight;
        if (item + TopItem == SelectedItem) {
            int stretch_to = (frame.Left + frame.GetWidth()) - pixel_size;

            text_color = ds->GetCompatibleColor(BackgroundColor);

            if (SelectedBkgColor > 0)
            {
                // draw the SelectedItem item bar (if colour not transparent)
                draw_color = ds->GetCompatibleColor(SelectedBkgColor);
                if (VisibleItemCount < ItemCount && (ListBoxFlags & kGuiListBox_NoBorder) == 0 &&
                    (ListBoxFlags & kGuiListBox_NoArrows) == 0)
                {
                    stretch_to -= get_fixed_pixel_size(7);
                }
                ds->FillRect(Rect(frame.Left + pixel_size, at_y, stretch_to, at_y + RowHeight - pixel_size), draw_color);
            }
        }
        else
        {
            text_color = ds->GetCompatibleColor(TextColor);
        }

        int item_index = item + TopItem;
        PrepareTextToDraw(Items[item_index]);

        if (TextAlignment & kAlignLeft)
        {
            wouttext_outline(ds, frame.Left + 1 + pixel_size, at_y + 1, TextFont, text_color, TextToDraw);
        }
        else
        {
            int textWidth = wgettextwidth(TextToDraw, TextFont);
            if (TextAlignment & kAlignRight)
            {
                wouttext_outline(ds, right_hand_edge - textWidth, at_y + 1, TextFont, text_color, TextToDraw);
            }
            else
            {
                wouttext_outline(ds, ((right_hand_edge - frame.Left) / 2) + frame.Left - (textWidth / 2), at_y + 1, TextFont, text_color, TextToDraw);
            }
        }
    }
    DrawItemsUnfix();
}

int GuiListBox::InsertItem(int index, const String &text)
{
    if (index < 0 ||
        LoadedGuiVersion < kGuiVersion_340_alpha && index > ItemCount)
    {
        return -1;
    }
    if (index > ItemCount)
    {
        Items.Append(text);
        SavedGameIndex.Append(-1);
    }
    else
    {
        Items.Insert(index, text);
        SavedGameIndex.Insert(index, -1);
        if (SelectedItem >= index)
        {
            SelectedItem++;
        }
    }
    ItemCount++;
    guis_need_update = 1;
    return ItemCount - 1;
}

void GuiListBox::RemoveItem(int index)
{
    if (index < 0 || index >= ItemCount)
    {
        return;
    }

    Items.Remove(index);
    SavedGameIndex.Remove(index);
    ItemCount--;

    if (SelectedItem > index)
    {
        SelectedItem--;
    }
    if (SelectedItem >= ItemCount)
    {
        SelectedItem = -1;
    }
    guis_need_update = 1;
}

void GuiListBox::SetItemText(int index, const String &text)
{
    if (index < 0 || index >= ItemCount)
    {
        return;
    }

    guis_need_update = 1;
    Items[index] = text;
}

void GuiListBox::SetFont(int font)
{
    TextFont = font;
    RowHeight = wgettextheight("YpyjIHgMNWQ", TextFont) + get_fixed_pixel_size(2);
    VisibleItemCount = Frame.GetHeight() / RowHeight;
}

bool GuiListBox::OnMouseDown()
{
    if (IsInRightMargin(MousePos.X))
    {
        if ((MousePos.Y < Frame.GetHeight() / 2) && TopItem > 0)
        {
            TopItem--;
        }
        if ((MousePos.Y >= Frame.GetHeight() / 2) && (ItemCount > TopItem + VisibleItemCount))
        {
            TopItem++;
        }
        return false;
    }
    int new_selection = GetItemAt(MousePos.X, MousePos.Y);
    if (new_selection < 0)
    {
        return false;
    }
    SelectedItem = new_selection;
    IsActivated = true;
    return false;
}

void GuiListBox::OnMouseMove(int x, int y)
{
    MousePos.X = x - Frame.Left;
    MousePos.Y = y - Frame.Top;
}

void GuiListBox::OnResized() 
{
	if (RowHeight == 0)
	{
        check_font(&TextFont);
        SetFont(TextFont);
	}
	if (RowHeight > 0)
    {
        VisibleItemCount = Frame.GetHeight() / RowHeight;
    }
}

void GuiListBox::WriteToFile(Stream *out)
{
    GuiObject::WriteToFile(out);
    out->WriteInt32(ListBoxFlags);
    out->WriteInt32(TextFont);
    out->WriteInt32(TextColor);
    out->WriteInt32(TextAlignment);
    out->WriteInt32(BackgroundColor);
    out->WriteInt32(SelectedBkgColor);
    out->WriteInt32(ItemCount);
    for (int i = 0; i < ItemCount; ++i)
    {
        Items[i].Write(out);
    }
    if (ListBoxFlags & kGuiListBox_SavedGameIndexValid)
    {
        SavedGameIndex.WriteRaw(out);
    }
}

void GuiListBox::ReadFromFile(Stream *in, GuiVersion gui_version)
{
    GuiObject::ReadFromFile(in, gui_version);
    if (gui_version < kGuiVersion_340_alpha)
    {
        ItemCount = in->ReadInt32();
        SelectedItem = in->ReadInt32();
        TopItem = in->ReadInt32();
        MousePos.X = in->ReadInt32();
        MousePos.Y = in->ReadInt32();
        RowHeight = in->ReadInt32();
        VisibleItemCount = in->ReadInt32();
        TextFont = in->ReadInt32();
        TextColor = in->ReadInt32();
        BackgroundColor = in->ReadInt32();
        ListBoxFlags = in->ReadInt32();
        LegacyGuiAlignment legacy_align;
        if (gui_version >= kGuiVersion_272b)
        {
            legacy_align = (LegacyGuiAlignment)in->ReadInt32();
            in->ReadInt32(); // reserved1
        }
        else
        {
            legacy_align = kLegacyGuiAlign_Left;
        }
        TextAlignment = ConvertLegacyAlignment(legacy_align);
        if (gui_version >= kGuiVersion_unkn_107)
        {
            SelectedBkgColor = in->ReadInt32();
        }
        else
        {
            SelectedBkgColor = TextColor;
            if (SelectedBkgColor == 0)
            {
                SelectedBkgColor = 16;
            }
        }
    }
    else
    {
        ListBoxFlags = in->ReadInt32();
        TextFont = in->ReadInt32();
        TextColor = in->ReadInt32();
        TextAlignment = (Alignment)in->ReadInt32();
        BackgroundColor = in->ReadInt32();
        SelectedBkgColor = in->ReadInt32();
        ItemCount = in->ReadInt32();
    }

    Items.SetLength(ItemCount);
    SavedGameIndex.SetLength(ItemCount);
    for (int i = 0; i < ItemCount; ++i)
    {
        Items[i].Read(in);
        SavedGameIndex[i] = -1;
    }
    if (gui_version >= kGuiVersion_272d && ListBoxFlags & kGuiListBox_SavedGameIndexValid)
    {
        SavedGameIndex.ReadRaw(in, ItemCount);
    }

    if (TextColor == 0)
    {
        TextColor = 16;
    }
}

void GuiListBox::WriteToSavedGame(Stream *out)
{
    GuiObject::WriteToSavedGame(out);
    out->WriteInt32(ListBoxFlags);
    out->WriteInt32(TextFont);
    out->WriteInt32(TextColor);
    out->WriteInt32(TextAlignment);
    out->WriteInt32(BackgroundColor);
    out->WriteInt32(SelectedBkgColor);
    out->WriteInt32(ItemCount);
    out->WriteInt32(TopItem);
    out->WriteInt32(SelectedItem);
    for (int i = 0; i < ItemCount; ++i)
    {
        Items[i].Write(out);
    }
    if (ListBoxFlags & kGuiListBox_SavedGameIndexValid)
    {
        SavedGameIndex.WriteRaw(out);
    }
}

void GuiListBox::ReadFromSavedGame(Common::Stream *in, RuntimeGuiVersion gui_version)
{
    GuiObject::ReadFromSavedGame(in, gui_version);
    ListBoxFlags = in->ReadInt32();
    TextFont = in->ReadInt32();
    TextColor = in->ReadInt32();
    TextAlignment = (Alignment)in->ReadInt32();
    BackgroundColor = in->ReadInt32();
    SelectedBkgColor = in->ReadInt32();
    ItemCount = in->ReadInt32();
    TopItem = in->ReadInt32();
    SelectedItem = in->ReadInt32();
    
    Items.SetLength(ItemCount);
    SavedGameIndex.SetLength(ItemCount);
    for (int i = 0; i < ItemCount; ++i)
    {
        Items[i].Read(in);
        SavedGameIndex[i] = -1;
    }
    if (ListBoxFlags & kGuiListBox_SavedGameIndexValid)
    {
        SavedGameIndex.ReadRaw(in, ItemCount);
    }
}

} // namespace Common
} // namespace AGS

AGS::Common::ObjectArray<AGS::Common::GuiListBox> guilist;
int numguilist = 0;
