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
#include <math.h>
#include "ac/game_version.h"
#include "gui/guiinv.h"
#include "gui/guimain.h"
#include "util/stream.h"

namespace AGS
{
namespace Common
{

GUIInvWindow::GUIInvWindow()
    : GUIControl(&ScriptEventTable::DefaultSchema())
{
    CalculateNumCells();
}

void GUIInvWindow::SetItemDimensions(int itemw, int itemh)
{
    if (_itemWidth != itemw || _itemHeight != itemh)
    {
        _itemWidth = itemw;
        _itemHeight = itemh;
        OnResized();
    }
}

void GUIInvWindow::SetCharacterID(int charid)
{
    if (_charID != charid)
    {
        _charID = charid;
        MarkChanged();
    }
}

void GUIInvWindow::OnMouseEnter()
{
    _isMouseOver = true;
}

void GUIInvWindow::OnMouseLeave()
{
    _isMouseOver = false;
}

void GUIInvWindow::OnMouseUp()
{
    if (_isMouseOver)
        _isActivated = true;
}

void GUIInvWindow::OnResized()
{
    CalculateNumCells();
    UpdateGraphicSpace();
    MarkChanged();
}

void GUIInvWindow::WriteToFile(Stream *out) const
{
    GUIControl::WriteToFile(out);
    out->WriteInt32(_charID);
    out->WriteInt32(_itemWidth);
    out->WriteInt32(_itemHeight);
}

void GUIInvWindow::ReadFromFile(Stream *in, GuiVersion gui_version)
{
    GUIControl::ReadFromFile(in, gui_version);
    _charID = in->ReadInt32();
    _itemWidth = in->ReadInt32();
    _itemHeight = in->ReadInt32();

    // ensure that some items are visible
    if (_itemWidth > _width)
        _itemWidth = _width;
    if (_itemHeight > _height)
        _itemHeight = _height;

    CalculateNumCells();
}

void GUIInvWindow::ReadFromSavegame(Stream *in, GuiSvgVersion svg_ver)
{
    GUIControl::ReadFromSavegame(in, svg_ver);
    _itemWidth = in->ReadInt32();
    _itemHeight = in->ReadInt32();
    _charID = in->ReadInt32();
    _topItem = in->ReadInt32();
    CalculateNumCells();
}

void GUIInvWindow::WriteToSavegame(Stream *out) const
{
    GUIControl::WriteToSavegame(out);
    out->WriteInt32(_itemWidth);
    out->WriteInt32(_itemHeight);
    out->WriteInt32(_charID);
    out->WriteInt32(_topItem);
}

void GUIInvWindow::CalculateNumCells()
{
    if (_itemWidth <= 0 || _itemHeight <= 0)
    {
        _colCount = 0;
        _rowCount = 0;
    }
    else
    {
        _colCount = _width / _itemWidth;
        _rowCount = _height / _itemHeight;
    }
}

} // namespace Common
} // namespace AGS
