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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "ac/common.h"
#include "gui/guiinv.h"
#include "gui/guimain.h"
#include "util/stream.h"

using AGS::Common::Stream;

DynamicArray<GUIInv> guiinv;
int numguiinv = 0;

void GUIInv::WriteToFile(Stream *out)
{
    GUIObject::WriteToFile(out);
    out->WriteInt32(charId);
    out->WriteInt32(itemWidth);
    out->WriteInt32(itemHeight);
    out->WriteInt32(topIndex);
}

void GUIInv::ReadFromFile(Stream *in, GuiVersion gui_version)
{
    GUIObject::ReadFromFile(in, gui_version);
    if (gui_version >= kGuiVersion_unkn_109) {
        charId = in->ReadInt32();
        itemWidth = in->ReadInt32();
        itemHeight = in->ReadInt32();
        topIndex = in->ReadInt32();
    }
    else {
        charId = -1;
        itemWidth = 40;
        itemHeight = 22;
        topIndex = 0;
    }

    if (loaded_game_file_version >= kGameVersion_270)
    {
        // ensure that some items are visible
        if (itemWidth > wid)
            itemWidth = wid;
        if (itemHeight > hit)
            itemHeight = hit;
    }

    CalculateNumCells();
}

void GUIInv::CalculateNumCells() {
    if (loaded_game_file_version >= kGameVersion_270)
    {
        itemsPerLine = wid / multiply_up_coordinate(itemWidth);
        numLines = hit / multiply_up_coordinate(itemHeight);
    }
    else
    {
        itemsPerLine = floor((float)wid / (float)multiply_up_coordinate(itemWidth) + 0.5f);
        numLines = floor((float)hit / (float)multiply_up_coordinate(itemHeight) + 0.5f);
    }
}
