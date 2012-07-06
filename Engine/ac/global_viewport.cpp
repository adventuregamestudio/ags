
#include "ac/global_viewport.h"
#include "wgt2allg.h"
#include "ac/viewport.h"
#include "acmain/ac_draw.h"
#include "debug/debug.h"

extern int offsetx, offsety;

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
