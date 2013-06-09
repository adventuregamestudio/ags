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
#include "ac/characterinfo.h"
#include "game/game_objects.h"

int offsetx = 0, offsety = 0;

extern int scrnwid,scrnhit;
extern CharacterInfo*playerchar;

void check_viewport_coords() 
{
    if (offsetx<0) offsetx=0;
    if (offsety<0) offsety=0;

    int roomWidth = multiply_up_coordinate(thisroom.Width);
    int roomHeight = multiply_up_coordinate(thisroom.Height);
    if (offsetx + scrnwid > roomWidth)
        offsetx = roomWidth - scrnwid;
    if (offsety + scrnhit > roomHeight)
        offsety = roomHeight - scrnhit;
}


void update_viewport()
{
    if ((thisroom.Width > BASEWIDTH) || (thisroom.Height > BASEHEIGHT)) {
        if (play.ViewportLocked == 0) {
            offsetx = multiply_up_coordinate(playerchar->x) - scrnwid/2;
            offsety = multiply_up_coordinate(playerchar->y) - scrnhit/2;
        }
        check_viewport_coords();
    }
    else {
        offsetx=0;
        offsety=0;
    }
}
