
#include "ac/viewport.h"
#include "wgt2allg.h"
#include "ac/draw.h"
#include "ac/roomstruct.h"
#include "ac/characterinfo.h"
#include "ac/gamestate.h"
#include "ac/gamesetup.h"

extern int offsetx, offsety;
extern GameState play;
extern roomstruct thisroom;
extern int scrnwid,scrnhit;
extern GameSetup usetup;
extern CharacterInfo*playerchar;

void check_viewport_coords() 
{
    if (offsetx<0) offsetx=0;
    if (offsety<0) offsety=0;

    int roomWidth = multiply_up_coordinate(thisroom.width);
    int roomHeight = multiply_up_coordinate(thisroom.height);
    if (offsetx + scrnwid > roomWidth)
        offsetx = roomWidth - scrnwid;
    if (offsety + scrnhit > roomHeight)
        offsety = roomHeight - scrnhit;
}


void update_viewport()
{
    if ((thisroom.width > BASEWIDTH) || (thisroom.height > BASEHEIGHT)) {
        if (play.offsets_locked == 0) {
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
