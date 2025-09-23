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
#include <algorithm>
#include "gui/guilistbox.h"
#include "ac/game_version.h"
#include "font/fonts.h"
#include "gui/guimain.h"
#include "util/stream.h"
#include "util/string_utils.h"

namespace AGS
{
namespace Common
{

/* static */ String GUIListBox::EventNames[GUIListBox::EventCount] =
    { "SelectionChanged" };
/* static */ String GUIListBox::EventArgs[GUIListBox::EventCount] =
    { "GUIControl *control" };

GUIListBox::GUIListBox()
{
}

void GUIListBox::SetFont(int font)
{
    if (_font != font)
    {
        _font = font;
        UpdateMetrics();
        UpdateGraphicSpace();
        MarkChanged();
    }
}

void GUIListBox::SetTextColor(int color)
{
    if (_textColor != color)
    {
        _textColor = color;
        MarkChanged();
    }
}

void GUIListBox::SetSelectedBgColor(int color)
{
    if (_selectedBgColor != color)
    {
        _selectedBgColor = color;
        MarkChanged();
    }
}

void GUIListBox::SetSelectedTextColor(int color)
{
    if (_selectedTextColor != color)
    {
        _selectedTextColor = color;
        MarkChanged();
    }
}

void GUIListBox::SetTextAlignment(HorAlignment align)
{
    if (_textAlignment != align)
    {
        _textAlignment = align;
        MarkChanged();
    }
}

void GUIListBox::SetSelectedItem(int index)
{
    if (index < 0 || static_cast<uint32_t>(index) >= _items.size())
        index = -1;

    if (_selectedItem != index)
    {
        _selectedItem = index;
        // Automatically update top item to keep selection visible
        if (index >= 0)
        {
            if (index < _topItem)
                _topItem = index;
            else if (static_cast<uint32_t>(index) >= _topItem + _visibleItemCount)
                _topItem = (index - _visibleItemCount) + 1;
        }
        MarkChanged();
    }
}

void GUIListBox::SetTopItem(int index)
{
    if (_topItem != index)
    {
        _topItem = index;
        MarkChanged();
    }
}

uint32_t GUIListBox::GetEventCount() const
{
    return EventCount;
}

String GUIListBox::GetEventName(uint32_t event) const
{
    if (event >= EventCount)
        return "";
    return EventNames[event];
}

String GUIListBox::GetEventArgs(uint32_t event) const
{
    if (event >= EventCount)
        return "";
    return EventArgs[event];
}

String GUIListBox::GetItem(int index) const
{
    if (index < 0 || static_cast<uint32_t>(index) >= _items.size())
        return String();
    return _items[index];
}

int GUIListBox::GetSavedGameIndex(int index) const
{
    if (index < 0 || static_cast<uint32_t>(index) >= _items.size())
        return -1;
    return _savedGameIndex[index];
}

int GUIListBox::GetItemAt(int x, int y) const
{
    if (_rowHeight <= 0 || IsInRightMargin(x))
        return -1;

    uint32_t index = y / _rowHeight + _topItem;
    if (index < 0 || index >= _items.size())
        return -1;
    return index;
}

bool GUIListBox::AreArrowsShown() const
{
    return (_listBoxFlags & kListBox_ShowArrows) != 0;
}

bool GUIListBox::IsBorderShown() const
{
    return (_listBoxFlags & kListBox_ShowBorder) != 0;
}

bool GUIListBox::IsSvgIndex() const
{
    return (_listBoxFlags & kListBox_SvgIndex) != 0;
}

bool GUIListBox::IsInRightMargin(int x) const
{
    if (x >= (_width - 6) && IsBorderShown() && AreArrowsShown())
        return 1;
    return 0;
}

Rect GUIListBox::CalcGraphicRect(bool clipped)
{
    if (clipped)
        return RectWH(0, 0, _width, _height);

    // TODO: need to find a way to text position, or there'll be some repetition
    // have to precache text and size on some events:
    // - translation change
    // - macro value change (overhotspot etc)
    Rect rc = RectWH(0, 0, _width, _height);
    UpdateMetrics();
    const int width = _width - 1;
    const int pixel_size = 1;
    int right_hand_edge = width - pixel_size - 1;
    // calculate the scroll bar's width if necessary
    if (_items.size() > _visibleItemCount && IsBorderShown() && AreArrowsShown())
        right_hand_edge -= 7;
    Line max_line;
    for (uint32_t item = 0; (item < _visibleItemCount) && (item + _topItem < _items.size()); ++item)
    {
        int at_y = pixel_size + item * _rowHeight;
        uint32_t item_index = item + _topItem;
        PrepareTextToDraw(_items[item_index]);
        Line lpos = GUI::CalcTextPositionHor(_textToDraw, _font, 1 + pixel_size, right_hand_edge, at_y + 1,
            (FrameAlignment)_textAlignment);
        max_line.X2 = std::max(max_line.X2, lpos.X2);
    }
    int last_line_y = pixel_size + 1 + (_visibleItemCount - 1) * _rowHeight;
    // Include font fixes for the first and last text line,
    // in case graphical height is different, and there's a VerticalOffset
    Line vextent = GUI::CalcFontGraphicalVExtent(_font);
    Rect text_rc = RectWH(0, vextent.Y1, max_line.X2 - max_line.X1 + 1, last_line_y + (vextent.Y2 - vextent.Y1 + 1));
    return SumRects(rc, text_rc);
}

int GUIListBox::AddItem(const String &text)
{
    return AddItem(text, -1);
}

int GUIListBox::AddItem(const String &text, int save)
{
    _items.push_back(text);
    _savedGameIndex.push_back(save);
    MarkChanged();
    return _items.size() - 1;
}

void GUIListBox::Clear()
{
    if (_items.empty())
        return;
    _items.clear();
    _savedGameIndex.clear();
    _topItem = 0;
    // NOTE: backwards compatible behavior is to keep selection at index 0,
    // so that the first item appears selected when added
    _selectedItem = (loaded_game_file_version >= kGameVersion_363) ? -1 : 0;
    MarkChanged();
}

void GUIListBox::Draw(Bitmap *ds, int x, int y)
{
    const int width  = _width - 1;
    const int height = _height - 1;
    const int pixel_size = 1;

    color_t text_color = ds->GetCompatibleColor(_textColor);
    color_t draw_color = ds->GetCompatibleColor(_textColor);
    if (IsBorderShown())
    {
        ds->DrawRect(Rect(x, y, x + width, y + height), draw_color);
        if (pixel_size > 1)
            ds->DrawRect(Rect(x + 1, y + 1, x + width - 1, y + height - 1), draw_color);
    }

    int right_hand_edge = (x + width) - pixel_size - 1;

    // update the _rowHeight and _visibleItemCount
    // FIXME: find a way to update this whenever relevant things change in the engine
    UpdateMetrics();

    // draw the scroll bar in if necessary
    bool scrollbar = (_items.size() > _visibleItemCount) && IsBorderShown() && AreArrowsShown();
    if (scrollbar)
    {
        int xstrt, ystrt;
        ds->DrawRect(Rect(x + width - 7, y, (x + (pixel_size - 1) + width) - 7, y + height), draw_color);
        ds->DrawRect(Rect(x + width - 7, y + height / 2, x + width, y + height / 2 + (pixel_size - 1)), draw_color);

        xstrt = (x + width - 6) + (pixel_size - 1);
        ystrt = (y + height - 3) - 5;

        draw_color = ds->GetCompatibleColor(_textColor);
        ds->DrawTriangle(Triangle(xstrt, ystrt, xstrt + 4, ystrt, 
                 xstrt + 2,
                 ystrt + 5), draw_color);

        ystrt = y + 3;
        ds->DrawTriangle(Triangle(xstrt, ystrt + 5, 
                 xstrt + 4, 
                 ystrt + 5,
                 xstrt + 2, ystrt), draw_color);

        right_hand_edge -= 7;
    }

    Rect old_clip = ds->GetClip();
    if (scrollbar && GUI::Options.ClipControls)
        ds->SetClip(Rect(x, y, right_hand_edge + 1, y + _height - 1));
    for (uint32_t item = 0; (item < _visibleItemCount) && (item + _topItem < _items.size()); ++item)
    {
        int at_y = y + pixel_size + item * _rowHeight;
        if (item + _topItem == _selectedItem)
        {
            text_color = ds->GetCompatibleColor(_selectedTextColor);
            if (_selectedBgColor != 0)
            {
                int stretch_to = (x + width) - pixel_size;
                // draw the _selectedItem item bar (if colour not transparent)
                draw_color = ds->GetCompatibleColor(_selectedBgColor);
                if ((_visibleItemCount < _items.size()) && IsBorderShown() && AreArrowsShown())
                    stretch_to -= 7;

                ds->FillRect(Rect(x + pixel_size, at_y, stretch_to, at_y + _rowHeight - pixel_size), draw_color);
            }
        }
        else
        {
            text_color = ds->GetCompatibleColor(_textColor);
        }

        int item_index = item + _topItem;
        PrepareTextToDraw(_items[item_index]);

        GUI::DrawTextAlignedHor(ds, _textToDraw, _font, text_color, x + 1 + pixel_size, right_hand_edge, at_y + 1,
            (FrameAlignment)_textAlignment);
    }
    ds->SetClip(old_clip);
}

int GUIListBox::InsertItem(int index, const String &text)
{
    if (index < 0 || static_cast<uint32_t>(index) > _items.size())
        return -1;

    _items.insert(_items.begin() + index, text);
    _savedGameIndex.insert(_savedGameIndex.begin() + index, -1);
    if (_selectedItem >= index)
        _selectedItem++;

    MarkChanged();
    return _items.size() - 1;
}

void GUIListBox::RemoveItem(int index)
{
    if (index < 0 || static_cast<uint32_t>(index) >= _items.size())
        return;

    _items.erase(_items.begin() + index);
    _savedGameIndex.erase(_savedGameIndex.begin() + index);

    if (_selectedItem > index)
        _selectedItem--;
    if (_selectedItem >= _items.size())
        _selectedItem = -1;
    MarkChanged();
}

void GUIListBox::SetShowArrows(bool on)
{
    if (on != ((_listBoxFlags & kListBox_ShowArrows) != 0))
        MarkChanged();
    if (on)
        _listBoxFlags |= kListBox_ShowArrows;
    else
        _listBoxFlags &= ~kListBox_ShowArrows;
}

void GUIListBox::SetShowBorder(bool on)
{
    if (on != ((_listBoxFlags & kListBox_ShowBorder) != 0))
        MarkChanged();
    if (on)
        _listBoxFlags |= kListBox_ShowBorder;
    else
        _listBoxFlags &= ~kListBox_ShowBorder;
}

void GUIListBox::SetSvgIndex(bool on)
{
    if (on)
        _listBoxFlags |= kListBox_SvgIndex;
    else
        _listBoxFlags &= ~kListBox_SvgIndex;
}

void GUIListBox::SetItemText(int index, const String &text)
{
    if ((index >= 0) && (static_cast<uint32_t>(index) < _items.size()) && (text != _items[index]))
    {
        _items[index] = text;
        MarkChanged();
    }
}

bool GUIListBox::OnMouseDown()
{
    if (IsInRightMargin(_mousePos.X))
    {
        int top_item = _topItem;
        if (_mousePos.Y < _height / 2 && _topItem > 0)
            top_item = _topItem - 1;
        if (_mousePos.Y >= _height / 2 && _items.size() > _topItem + _visibleItemCount)
            top_item = _topItem + 1;
        if (_topItem != top_item)
        {
            _topItem = top_item;
            MarkChanged();
        }
        return false;
    }

    int sel = GetItemAt(_mousePos.X, _mousePos.Y);
    // We do not cancel current selection if clicked on empty spot, leave it as is
    if (sel < 0)
        return false;
    if (sel != _selectedItem)
    {
        _selectedItem = sel;
        MarkChanged();
    }
    _isActivated = true;
    return false;
}

void GUIListBox::OnMouseMove(int mx, int my)
{
    _mousePos = _gs.WorldToLocal(mx, my);
}

void GUIListBox::OnResized()
{
    UpdateMetrics();
    UpdateGraphicSpace();
    MarkChanged();
}

void GUIListBox::UpdateMetrics()
{
    int font_height = get_font_height_outlined(_font);
    _rowHeight = font_height + 2; // +1 top/bottom margin
    _visibleItemCount = _height / _rowHeight;
    if (_items.size() <= _visibleItemCount)
        _topItem = 0; // reset scroll if all items are visible
}

// TODO: replace string serialization with StrUtil::ReadString and WriteString
// methods in the future, to keep this organized.
void GUIListBox::WriteToFile(Stream *out) const
{
    GUIControl::WriteToFile(out);
    out->WriteInt32(_items.size());
    out->WriteInt32(_font);
    out->WriteInt32(_textColor);
    out->WriteInt32(_selectedTextColor);
    out->WriteInt32(_listBoxFlags);
    out->WriteInt32(_textAlignment);
    out->WriteInt32(_selectedBgColor);
    for (uint32_t i = 0; i < _items.size(); ++i)
        _items[i].Write(out);
}

void GUIListBox::ReadFromFile(Stream *in, GuiVersion gui_version)
{
    Clear();

    GUIControl::ReadFromFile(in, gui_version);
    const uint32_t item_count = in->ReadInt32();
    _font = in->ReadInt32();
    _textColor = in->ReadInt32();
    _selectedTextColor = in->ReadInt32();
    _listBoxFlags = in->ReadInt32();
    _textAlignment = (HorAlignment)in->ReadInt32();
    _selectedBgColor = in->ReadInt32();

    // NOTE: we leave items in game data format as a potential support for defining
    // ListBox contents at design-time, although Editor does not support it as of 3.5.0.
    _items.resize(item_count);
    _savedGameIndex.resize(item_count, -1);
    for (uint32_t i = 0; i < item_count; ++i)
    {
        _items[i].Read(in);
    }

    if (_textColor == 0)
        _textColor = 16; // FIXME: adjust this using GetStandardColor where is safe to access GuiContext

    // Reset dynamic values
    _rowHeight = 0;
    _visibleItemCount = 0;
    _topItem = 0;
    // NOTE: backwards compatible behavior is to keep selection at index 0,
    // so that the first item appears selected when added
    _selectedItem = (loaded_game_file_version >= kGameVersion_363) ? -1 : 0;
}

void GUIListBox::ReadFromSavegame(Stream *in, GuiSvgVersion svg_ver)
{
    GUIControl::ReadFromSavegame(in, svg_ver);
    // Properties
    _listBoxFlags = in->ReadInt32();
    _font = in->ReadInt32();
    _selectedBgColor = in->ReadInt32();
    _selectedTextColor = in->ReadInt32();
    _textAlignment = (HorAlignment)in->ReadInt32();
    _textColor = in->ReadInt32();

    // _items
    const uint32_t item_count = in->ReadInt32();
    _items.resize(item_count);
    _savedGameIndex.resize(item_count);
    for (uint32_t i = 0; i < item_count; ++i)
        _items[i] = StrUtil::ReadString(in);
    // TODO: investigate this, it might be unreasonable to save and read
    // savegame index like that because list of savegames may easily change
    // in between writing and restoring the game. Perhaps clearing and forcing
    // this list to update on load somehow may make more sense.
    if (_listBoxFlags & kListBox_SvgIndex)
        for (uint32_t i = 0; i < item_count; ++i)
            _savedGameIndex[i] = in->ReadInt16();
    _topItem = in->ReadInt32();
    _selectedItem = in->ReadInt32();

    // Reset dynamic values
    _rowHeight = 0;
    _visibleItemCount = 0;
    _topItem = 0;
}

void GUIListBox::WriteToSavegame(Stream *out) const
{
    GUIControl::WriteToSavegame(out);
    // Properties
    out->WriteInt32(_listBoxFlags);
    out->WriteInt32(_font);
    out->WriteInt32(_selectedBgColor);
    out->WriteInt32(_selectedTextColor);
    out->WriteInt32(_textAlignment);
    out->WriteInt32(_textColor);

    // _items
    const uint32_t item_count = _items.size();
    out->WriteInt32(item_count);
    for (uint32_t i = 0; i < item_count; ++i)
        StrUtil::WriteString(_items[i], out);
    if (_listBoxFlags & kListBox_SvgIndex)
        for (uint32_t i = 0; i < item_count; ++i)
            out->WriteInt16(_savedGameIndex[i]);
    out->WriteInt32(_topItem);
    out->WriteInt32(_selectedItem);
}

} // namespace Common
} // namespace AGS
