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

#include "gui/newcontrol.h"
#include "gui/guidialoginternaldefs.h"

extern int topwindowhandle;
extern int mousex, mousey;

extern Common::Graphics *GetVirtualScreenGraphics();

NewControl::NewControl(int xx, int yy, int wi, int hi)
{
    x = xx;
    y = yy;
    wid = wi;
    hit = hi;
    state = 0;
    visible = 1;
    enabled = 1;
    needredraw = 1;
};
NewControl::NewControl() {
    visible = 1;
    enabled = 1;
}
int NewControl::mouseisinarea()
{
    if (topwindowhandle != wlevel)
        return 0;

    if ((mousex > x) & (mousex < x + wid) & (mousey > y) & (mousey < y + hit))
        return 1;

    return 0;
}
void NewControl::drawifneeded()
{
    if (topwindowhandle != wlevel)
        return;
    if (needredraw) {
        needredraw = 0;
        draw(GetVirtualScreenGraphics());
    }
}
void NewControl::drawandmouse()
{
    //    domouse(2);
    draw(GetVirtualScreenGraphics());
    //  domouse(1);
}
