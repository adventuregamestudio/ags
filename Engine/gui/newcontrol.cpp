
#include "util/wgt2allg.h"
#include "gui/newcontrol.h"
#include "gui/guidialoginternaldefines.h"

extern int topwindowhandle;
extern int mousex, mousey;

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
        draw();
    }
}
void NewControl::drawandmouse()
{
    //    domouse(2);
    draw();
    //  domouse(1);
}
