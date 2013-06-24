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

#include <math.h>
#include "ac/game_version.h"
#include "gui/guiinv.h"
#include "gui/guimain.h"

namespace AGS
{
namespace Common
{

GuiInvWindow::GuiInvWindow()
{
    IsMouseOver = false;
    CharacterId = -1;
    ItemWidth = 40;
    ItemHeight = 22;
    ColumnCount = 0;
    RowCount = 0;
    TopItem = 0;
    CalculateNumCells();

    SupportedEventCount = 0;
}

void GuiInvWindow::OnMouseLeave()
{
    IsMouseOver = false;
}

void GuiInvWindow::OnMouseOver()
{
    IsMouseOver = true;
}

void GuiInvWindow::OnMouseUp()
{
    IsActivated = IsMouseOver;
}

void GuiInvWindow::OnResized()
{
    CalculateNumCells();
}

void GuiInvWindow::WriteToFile(Stream *out)
{
	GuiObject::WriteToFile(out);
	out->WriteInt32(ItemWidth);
	out->WriteInt32(ItemHeight);
    out->WriteInt32(CharacterId);
}

void GuiInvWindow::ReadFromFile(Stream *in, GuiVersion gui_version)
{
    GuiObject::ReadFromFile(in, gui_version);
	if (gui_version >= kGuiVersion_unkn_109)
    {
        if (gui_version < kGuiVersion_340_alpha)
        {
            CharacterId = in->ReadInt32();
            ItemWidth = in->ReadInt32();
            ItemHeight = in->ReadInt32();
            TopItem = in->ReadInt32();
        }
        else
        {
            ItemWidth = in->ReadInt32();
            ItemHeight = in->ReadInt32();
            CharacterId = in->ReadInt32();
        }
	}
	else
    {
        CharacterId = -1;
        ItemWidth = 40;
        ItemHeight = 22;
        TopItem = 0;
	}

	if (loaded_game_file_version >= kGameVersion_270)
	{
	  // ensure that some items are visible
        if (ItemWidth > Frame.GetWidth())
        {
            ItemWidth = Frame.GetWidth();
        }
        if (ItemHeight > Frame.GetHeight())
        {
            ItemHeight = Frame.GetHeight();
        }
	}

	CalculateNumCells();
}

void GuiInvWindow::WriteToSavedGame(Stream *out)
{
    GuiObject::WriteToSavedGame(out);
    out->WriteInt32(ItemWidth);
    out->WriteInt32(ItemHeight);
    out->WriteInt32(CharacterId);
    out->WriteInt32(TopItem);
}

void GuiInvWindow::ReadFromSavedGame(Common::Stream *in, RuntimeGuiVersion gui_version)
{
    GuiObject::ReadFromSavedGame(in, gui_version);
    ItemWidth = in->ReadInt32();
    ItemHeight = in->ReadInt32();
    CharacterId = in->ReadInt32();
    TopItem = in->ReadInt32();
    CalculateNumCells();
}

void GuiInvWindow::CalculateNumCells()
{
    if (loaded_game_file_version >= kGameVersion_270)
    {
        ColumnCount = Frame.GetWidth() / multiply_up_coordinate(ItemWidth);
        RowCount = Frame.GetHeight() / multiply_up_coordinate(ItemHeight);
    }
    else
    {
        // CHECKME: must be a way to not use float math...
        ColumnCount = floor((float)Frame.GetWidth() / (float)multiply_up_coordinate(ItemWidth) + 0.5f);
        RowCount = floor((float)Frame.GetHeight() / (float)multiply_up_coordinate(ItemHeight) + 0.5f);
    }
}

} // namespace Common
} // namespace AGS

AGS::Common::ObjectArray<AGS::Common::GuiInvWindow> guiinv;
int numguiinv = 0;
