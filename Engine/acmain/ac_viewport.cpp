
#include "acmain/ac_maindefines.h"
#include "acmain/ac_viewport.h"

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


void SetViewport(int offsx,int offsy) {
    DEBUG_CONSOLE("Viewport locked to %d,%d", offsx, offsy);
    offsetx = multiply_up_coordinate(offsx);
    offsety = multiply_up_coordinate(offsy);
    check_viewport_coords();
    play.offsets_locked = 1;
}
void ReleaseViewport() {
    play.offsets_locked = 0;
    DEBUG_CONSOLE("Viewport released back to engine control");
}
int GetViewportX () {
    return divide_down_coordinate(offsetx);
}
int GetViewportY () {
    return divide_down_coordinate(offsety);
}

