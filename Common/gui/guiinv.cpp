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

    numSupportedEvents = 0;
}

void GUIInvWindow::MouseOver()
{
    IsMouseOver = true;
}

void GUIInvWindow::MouseLeave()
{
    IsMouseOver = false;
}

void GUIInvWindow::MouseUp()
{
    if (IsMouseOver)
        activated = 1;
}

void GUIInvWindow::Resized()
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

    if (loaded_game_file_version >= kGameVersion_270)
    {
        // ensure that some items are visible
        if (ItemWidth > wid)
            ItemWidth = wid;
        if (ItemHeight > hit)
            ItemHeight = hit;
    }

    CalculateNumCells();
}

void GUIInvWindow::CalculateNumCells()
{
    if (loaded_game_file_version >= kGameVersion_270)
    {
        ColCount = wid / multiply_up_coordinate(ItemWidth);
        RowCount = hit / multiply_up_coordinate(ItemHeight);
    }
    else
    {
        ColCount = floor((float)wid / (float)multiply_up_coordinate(ItemWidth) + 0.5f);
        RowCount = floor((float)hit / (float)multiply_up_coordinate(ItemHeight) + 0.5f);
    }
}

} // namespace Common
} // namespace AGS
