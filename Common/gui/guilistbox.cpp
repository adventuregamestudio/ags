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
    _flags |= kGUICtrl_ShowBorder;
    UpdateControlRect();
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
            if ((index < _topItem) || (_visibleItemCount == 0))
                _topItem = index;
            else if ((static_cast<uint32_t>(index) >= _topItem + _visibleItemCount))
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

bool GUIListBox::HasAlphaChannel() const
{
    return is_font_antialiased(_font);
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
    if (_rowHeight <= 0 || IsPosOnScrollbar(x))
        return -1;

    uint32_t index = y / _rowHeight + _topItem;
    if (index < 0 || index >= _items.size())
        return -1;
    return index;
}

bool GUIListBox::ShouldShowScrollArrows() const
{
    return (_listBoxFlags & kListBox_ShowArrows) != 0;
}

bool GUIListBox::AreArrowsShown() const
{
    return IsShowBorder() && ShouldShowScrollArrows() &&
        _visibleItemCount < _items.size();
}

bool GUIListBox::IsPosOnScrollbar(int x) const
{
    if (AreArrowsShown() && (x >= (_width - get_fixed_pixel_size(6))))
        return 1;
    return 0;
}

bool GUIListBox::IsSvgIndex() const
{
    return (_listBoxFlags & kListBox_SvgIndex) != 0;
}

Rect GUIListBox::CalcGraphicRect(bool clipped)
{
    if (clipped)
        return RectWH(0, 0, _width, _height);

    // TODO: need to find a way to text position, or there'll be some repetition
    // have to precache text and size on some events:
    // - translation change
    // - macro value change (score, overhotspot etc)
    Rect rc = RectWH(0, 0, _width, _height);
    UpdateMetrics();
    Line max_line;
    for (uint32_t item = 0; (item < _visibleItemCount) && (item + _topItem < _items.size()); ++item)
    {
        int at_y = _itemsRect.Top + item * _rowHeight;
        uint32_t item_index = item + _topItem;
        PrepareTextToDraw(_items[item_index]);
        Line lpos = GUI::CalcTextPositionHor(_textToDraw, _font, _itemsRect.Left, _itemsRect.Right, at_y + _itemTextPaddingY,
            _textAlignment);
        max_line.X2 = std::max(max_line.X2, lpos.X2);
    }
    int last_line_y = _itemsRect.Top + _itemTextPaddingY + (_visibleItemCount - 1) * _rowHeight;
    // Include font fixes for the first and last text line,
    // in case graphical height is different, and there's a VerticalOffset
    int h_ext = GUI::CalcFontGraphicalHExtent(_font);
    Line vextent = GUI::CalcFontGraphicalVExtent(_font);
    Rect text_rc = RectWH(h_ext, vextent.Y1, max_line.X2 - max_line.X1 + 1 + (-h_ext * 2),
                          last_line_y + (vextent.Y2 - vextent.Y1 + 1));
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
    _selectedItem = (GUI::DataVersion >= kGameVersion_363) ? -1 : 0;
    MarkChanged();
}

void GUIListBox::Draw(Bitmap *ds, int x, int y)
{
    const int pixel_size = get_fixed_pixel_size(1);

    DrawControlFrame(ds, x, y);

    // update the _rowHeight and _visibleItemCount
    // FIXME: find a way to update this whenever relevant things change in the engine
    UpdateMetrics();

    // draw the scroll bar in if necessary
    const bool scrollbar = AreArrowsShown();
    if (scrollbar)
    {
        const color_t draw_color = ds->GetCompatibleColor(_borderColor);
        ds->DrawRect(Rect(x + _scrollbarRect.Left, y + _scrollbarRect.Top, x + _scrollbarRect.Right, y + _scrollbarRect.Top + _scrollbarRect.GetHeight() / 2 - 1), draw_color);
        ds->DrawRect(Rect(x + _scrollbarRect.Left, y + _scrollbarRect.Top + _scrollbarRect.GetHeight() / 2 - 1, x + _scrollbarRect.Right, y + _scrollbarRect.Bottom), draw_color);

        const int xstrt = x + _scrollbarRect.Left + 1;
        // Up arrow
        int ystrt = y + _scrollbarRect.Top + 3;
        ds->DrawTriangle(Triangle(xstrt, ystrt + get_fixed_pixel_size(5),
                xstrt + get_fixed_pixel_size(4),
                ystrt + get_fixed_pixel_size(5),
                xstrt + get_fixed_pixel_size(2), ystrt), draw_color);
        // Down arrow
        ystrt = (y + _scrollbarRect.Bottom - 3) - get_fixed_pixel_size(5);
        ds->DrawTriangle(Triangle(xstrt, ystrt, xstrt + get_fixed_pixel_size(4), ystrt, 
                 xstrt + get_fixed_pixel_size(2),
                 ystrt + get_fixed_pixel_size(5)), draw_color);
    }

    const Rect items_rc = Rect::MoveBy(_itemsRect, x, y);
    const Rect old_clip = ds->GetClip();
    if (GUI::Options.ClipControls)
        ds->SetClip(items_rc);
    for (uint32_t item = 0; (item < _visibleItemCount) && (item + _topItem < _items.size()); ++item)
    {
        color_t text_color;
        const int at_x = items_rc.Left;
        const int right_x = items_rc.Right;
        int at_y = items_rc.Top + item * _rowHeight;
        if (item + _topItem == _selectedItem)
        {
            text_color = ds->GetCompatibleColor(_selectedTextColor);
            if (_selectedBgColor > 0)
            {
                // draw the _selectedItem item bar (if colour not transparent)
                const color_t draw_color = ds->GetCompatibleColor(_selectedBgColor);
                ds->FillRect(Rect(at_x, at_y, right_x, at_y + _rowHeight - 1), draw_color);
            }
        }
        else
        {
            text_color = ds->GetCompatibleColor(_textColor);
        }
        const color_t outline_color = ds->GetCompatibleColor(_textOutlineColor);

        int item_index = item + _topItem;
        PrepareTextToDraw(_items[item_index]);

        GUI::DrawTextAlignedHor(ds, _textToDraw, _font, text_color, outline_color,
            at_x + _itemTextPaddingX, right_x - _itemTextPaddingX,
            at_y + _itemTextPaddingY, _textAlignment);
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

void GUIListBox::UpdateVisualState()
{
    MarkPositionChanged(true);
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
    if (IsPosOnScrollbar(_mousePos.X))
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

void GUIListBox::OnMouseMove(int x_, int y_)
{
    _mousePos.X = x_ - _x;
    _mousePos.Y = y_ - _y;
}

void GUIListBox::OnResized()
{
    GUIObject::OnResized();
    UpdateMetrics();
}

void GUIListBox::OnContentRectChanged()
{
    UpdateMetrics();
    MarkChanged();
}

void GUIListBox::OnTextFontChanged()
{
    UpdateMetrics();
}

void GUIListBox::UpdateMetrics()
{
    if (GUI::GameGuiVersion < kGuiVersion_363_03)
    {
        // NOTE: we do this here, because calling get_fixed_pixel_size()
        // may not be safe in constructor
        _borderWidth = get_fixed_pixel_size(1);
    }

    int font_height = (GUI::DataVersion < kGameVersion_360_21) ?
        get_font_height(_font) : get_font_height_outlined(_font);
    _rowHeight = font_height + get_fixed_pixel_size(2); // +1 top/bottom margin
    _itemTextPaddingX = get_fixed_pixel_size(1);
    _itemTextPaddingY = get_fixed_pixel_size(1);
    const int items_height = _innerRect.GetHeight();
    _visibleItemCount = items_height / _rowHeight;
    if (_items.size() <= _visibleItemCount)
        _topItem = 0; // reset scroll if all items are visible

    int items_right = _innerRect.Right;
    if (AreArrowsShown())
    {
        // Scrollbar rect includes 1-thin border around it (for convenience of calc & draw),
        // so we subtract that from the outer border
        const int scroll_width = get_fixed_pixel_size(7);
        const int scroll_border = _borderWidth > 0 ? 1 : 0;
        const int ext_border = _borderWidth - scroll_border;
        _scrollbarRect = RectWH(_width - ext_border - scroll_width,
            ext_border, scroll_width, _height - ext_border * 2);
        items_right = _scrollbarRect.Left - 1;
    }

    _itemsRect = Rect(_innerRect.Left, _innerRect.Top, items_right, _innerRect.Bottom);
}

// TODO: replace string serialization with StrUtil::ReadString and WriteString
// methods in the future, to keep this organized.
void GUIListBox::WriteToFile(Stream *out) const
{
    GUIObject::WriteToFile(out);
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

    GUIObject::ReadFromFile(in, gui_version);
    const uint32_t item_count = in->ReadInt32();
    if (gui_version < kGuiVersion_350)
    {
        // old data format have these dynamic values written as well,
        // even though that did not have any purpose
        in->ReadInt32(); // _selectedItem
        in->ReadInt32(); // _topItem
        in->ReadInt32(); // _mousePos.X
        in->ReadInt32(); // _mousePos.Y
        in->ReadInt32(); // _rowHeight
        in->ReadInt32(); // _visibleItemCount
    }
    _font = in->ReadInt32();
    _textColor = in->ReadInt32();
    _selectedTextColor = in->ReadInt32();
    _listBoxFlags = in->ReadInt32();
    // reverse particular flags from older format
    if (gui_version < kGuiVersion_350)
        _listBoxFlags ^= kListBox_OldFmtXorMask;

    if (gui_version >= kGuiVersion_272b)
    {
        if (gui_version < kGuiVersion_350)
        {
            _textAlignment = static_cast<FrameAlignment>(ConvertLegacyGUIAlignment((LegacyGUIAlignment)in->ReadInt32()));
            in->ReadInt32(); // reserved1
        }
        else
        {
            _textAlignment = static_cast<FrameAlignment>(in->ReadInt32());
        }
    }
    else
    {
        _textAlignment = kAlignTopLeft;
    }

    if (gui_version >= kGuiVersion_unkn_107)
    {
        _selectedBgColor = in->ReadInt32();
    }
    else
    {
        _selectedBgColor = _textColor;
        if (_selectedBgColor == 0)
            _selectedBgColor = 16;
    }

    // NOTE: we leave items in game data format as a potential support for defining
    // ListBox contents at design-time, although Editor does not support it as of 3.5.0.
    _items.resize(item_count);
    _savedGameIndex.resize(item_count, -1);
    for (uint32_t i = 0; i < item_count; ++i)
    {
        _items[i].Read(in);
    }

    if (gui_version >= kGuiVersion_272d && gui_version < kGuiVersion_350 &&
        (_listBoxFlags & kListBox_SvgIndex))
    { // NOTE: reading into actual variables only for old savegame support
        for (uint32_t i = 0; i < item_count; ++i)
            _savedGameIndex[i] = in->ReadInt16();
    }

    if (_textColor == 0)
        _textColor = 16;

    // Reset dynamic values
    _rowHeight = 0;
    _itemTextPaddingX = 0;
    _itemTextPaddingY = 0;
    _visibleItemCount = 0;
    _topItem = 0;
    // NOTE: backwards compatible behavior is to keep selection at index 0,
    // so that the first item appears selected when added
    _selectedItem = (GUI::DataVersion >= kGameVersion_363) ? -1 : 0;
}

void GUIListBox::ReadFromFile_Ext363(Stream *in, GuiVersion gui_version)
{
    GUIObject::ReadFromFile_Ext363(in, gui_version);

    _textOutlineColor = in->ReadInt32();
    in->ReadInt32(); // reserved
    in->ReadInt32();
    in->ReadInt32();
}

void GUIListBox::ReadFromSavegame(Stream *in, GuiSvgVersion svg_ver)
{
    GUIObject::ReadFromSavegame(in, svg_ver);
    // Properties
    _listBoxFlags = in->ReadInt32();
    _font = in->ReadInt32();
    if (svg_ver < kGuiSvgVersion_350)
    {
        // reverse particular flags from older format
        _listBoxFlags ^= kListBox_OldFmtXorMask;
    }
    else
    {
        _selectedBgColor = in->ReadInt32();
        _selectedTextColor = in->ReadInt32();
        _textAlignment = static_cast<FrameAlignment>(in->ReadInt32());
        _textColor = in->ReadInt32();
    }

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

    if (svg_ver >= kGuiSvgVersion_36308)
    {
        _textOutlineColor = in->ReadInt32();
        in->ReadInt32(); // reserved
        in->ReadInt32();
        in->ReadInt32();
    }

    if (svg_ver < kGuiSvgVersion_36304)
    {
        SetDefaultLooksFor363();
    }

    // Reset dynamic values
    _rowHeight = 0;
    _itemTextPaddingX = 0;
    _itemTextPaddingY = 0;
    _visibleItemCount = 0;
    _topItem = 0;
}

void GUIListBox::WriteToSavegame(Stream *out) const
{
    GUIObject::WriteToSavegame(out);
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

    // kGuiSvgVersion_36308
    out->WriteInt32(_textOutlineColor);
    out->WriteInt32(0); // reserved
    out->WriteInt32(0);
    out->WriteInt32(0);
}

void GUIListBox::SetDefaultLooksFor363()
{
    if ((_listBoxFlags & kListBox_ShowBorder) != 0)
        _flags |= kGUICtrl_ShowBorder;
    _borderColor = _textColor;
    _borderWidth = get_fixed_pixel_size(1);
    _textOutlineColor = 16;
    UpdateControlRect();
}

} // namespace Common
} // namespace AGS
