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

#include "ac/global_viewport.h"
#include "ac/draw.h"
#include "ac/viewport.h"
#include "debug/debug_log.h"

extern int offsetx, offsety;

void SetViewport(int offsx,int offsy) {
    debug_script_log("Viewport locked to %d,%d", offsx, offsy);
    offsetx = multiply_up_coordinate(offsx);
    offsety = multiply_up_coordinate(offsy);
    check_viewport_coords();
    play.offsets_locked = 1;
}
void ReleaseViewport() {
    play.offsets_locked = 0;
    debug_script_log("Viewport released back to engine control");
}
int GetViewportX () {
    return divide_down_coordinate(offsetx);
}
int GetViewportY () {
    return divide_down_coordinate(offsety);
}
