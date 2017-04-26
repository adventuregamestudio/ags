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
#include "ac/common.h"
#include "gui/guiinv.h"
#include "gui/guimain.h"
#include "util/stream.h"

std::vector<AGS::Common::GUIInvWindow> guiinv;
int numguiinv = 0;

namespace AGS
{
namespace Common
{

GUIInvWindow::GUIInvWindow()
{
    IsMouseOver = false;
    CharId = -1;
    ItemWidth = 40;
    ItemHeight = 22;
    ColCount = 0;
    RowCount = 0;
    TopItem = 0;
    CalculateNumCells();

    _scEventCount = 0;
}

void GUIInvWindow::OnMouseEnter()
{
    IsMouseOver = true;
}

void GUIInvWindow::OnMouseLeave()
{
    IsMouseOver = false;
}

void GUIInvWindow::OnMouseUp()
{
    if (IsMouseOver)
        IsActivated = true;
}

void GUIInvWindow::OnResized()
{
    CalculateNumCells();
}

void GUIInvWindow::WriteToFile(Stream *out)
{
	GUIObject::WriteToFile(out);
    out->WriteInt32(CharId);
    out->WriteInt32(ItemWidth);
    out->WriteInt32(ItemHeight);
    out->WriteInt32(TopItem);
}

void GUIInvWindow::ReadFromFile(Stream *in, GuiVersion gui_version)
{
	GUIObject::ReadFromFile(in, gui_version);
    if (gui_version >= kGuiVersion_unkn_109)
    {
        CharId = in->ReadInt32();
        ItemWidth = in->ReadInt32();
        ItemHeight = in->ReadInt32();
        TopItem = in->ReadInt32();
	}
    else
    {
        CharId = -1;
        ItemWidth = 40;
        ItemHeight = 22;
        TopItem = 0;
	}

	// ensure that some items are visible
        if (ItemWidth > Width)
            ItemWidth = Width;
        if (ItemHeight > Height)
            ItemHeight = Height;


	CalculateNumCells();
}

void GUIInvWindow::CalculateNumCells()
{
        ColCount = Width / ItemWidth;
        RowCount = Height / ItemHeight;
}

} // namespace Common
} // namespace AGS
