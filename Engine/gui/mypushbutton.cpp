
#include <string.h>
#include "wgt2allg.h"
#include "ali3d.h"
#include "acmain/ac_mouse.h"
#include "acmain/ac_record.h"
#include "gui/mypushbutton.h"
#include "gui/guidialog.h"
#include "gui/guidialoginternaldefines.h"
#include "main/game_run.h"
#include "media/audio/audio.h"

extern volatile int timerloop;

extern int windowbackgroundcolor, pushbuttondarkcolor;
extern int pushbuttonlightcolor;
extern int cbuttfont;

MyPushButton::MyPushButton(int xx, int yy, int wi, int hi, char *tex)
{                             //wlevel=2;
    x = xx;
    y = yy;
    wid = wi;
    hit = hi + 1;               //hit=hi;
    state = 0;
    strncpy(text, tex, 50);
    text[49] = 0;
};

void MyPushButton::draw()
{
    wtextcolor(0);
    wsetcolor(COL254);
    wbar(x, y, x + wid, y + hit);
    if (state == 0)
        wsetcolor(pushbuttondarkcolor);
    else
        wsetcolor(pushbuttonlightcolor);

    wrectangle(x, y, x + wid, y + hit);
    if (state == 0)
        wsetcolor(pushbuttonlightcolor);
    else
        wsetcolor(pushbuttondarkcolor);

    wline(x, y, x + wid - 1, y);
    wline(x, y, x, y + hit - 1);
    wouttextxy(x + (wid / 2 - wgettextwidth(text, cbuttfont) / 2), y + 2, cbuttfont, text);
    if (typeandflags & CNF_DEFAULT)
        wsetcolor(0);
    else
        wsetcolor(windowbackgroundcolor);

    wrectangle(x - 1, y - 1, x + wid + 1, y + hit + 1);
}

//extern const int LEFT;  // in mousew32

int MyPushButton::pressedon()
{
    int wasstat;
    while (mbutrelease(LEFT) == 0) {
        timerloop = 0;
        wasstat = state;
        next_iteration();
        state = mouseisinarea();
        // stop mp3 skipping if button held down
        update_polled_stuff_if_runtime();
        if (wasstat != state) {
            //        domouse(2);
            draw();
            //domouse(1);
        }

        //      domouse(0);

        refresh_screen();

        while (timerloop == 0) ;
    }
    wasstat = state;
    state = 0;
    //    domouse(2);
    draw();
    //  domouse(1);
    return wasstat;
}

int MyPushButton::processmessage(int mcode, int wParam, long lParam)
{
    return -1;                  // doesn't support messages
}