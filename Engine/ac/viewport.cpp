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

#include "ac/viewport.h"
#include "ac/draw.h"
#include "ac/roomstruct.h"
#include "ac/characterinfo.h"
#include "ac/gamestate.h"
#include "ac/gamesetup.h"

extern int offsetx, offsety;
extern GameState play;
extern roomstruct thisroom;
extern GameSetup usetup;
extern CharacterInfo*playerchar;

void check_viewport_coords() 
{
    if (offsetx<0) offsetx=0;
    if (offsety<0) offsety=0;

    int roomWidth = multiply_up_coordinate(thisroom.width);
    int roomHeight = multiply_up_coordinate(thisroom.height);
    if (offsetx + play.viewport.GetWidth() > roomWidth)
        offsetx = roomWidth - play.viewport.GetWidth();
    if (offsety + play.viewport.GetHeight() > roomHeight)
        offsety = roomHeight - play.viewport.GetHeight();
}


void update_viewport()
{
    if ((thisroom.width > BASEWIDTH) || (thisroom.height > BASEHEIGHT)) {
        if (play.offsets_locked == 0) {
            offsetx = multiply_up_coordinate(playerchar->x) - play.viewport.GetWidth()/2;
            offsety = multiply_up_coordinate(playerchar->y) - play.viewport.GetHeight()/2;
        }
        check_viewport_coords();
    }
    else {
        offsetx=0;
        offsety=0;
    }
}
