
#include <string.h>
#include "util/wgt2allg.h"
#include "gfx/ali3d.h"
#include "gui/mytextbox.h"
#include "gui/guidialoginternaldefs.h"
#include "gfx/bitmap.h"

using AGS::Common::Bitmap;

extern GameSetup usetup;

extern int windowbackgroundcolor;
extern int cbuttfont;

MyTextBox::MyTextBox(int xx, int yy, int wii, char *tee)
{
    x = xx;
    y = yy;
    wid = wii;
    if (tee != NULL)
        strcpy(text, tee);
    else
        text[0] = 0;

    hit = TEXT_HT + 1;
}

void MyTextBox::draw()
{
    wsetcolor(windowbackgroundcolor);
    abuf->FillRect(Rect(x, y, x + wid, y + hit), currentcolor);
    wsetcolor(0);
    abuf->DrawRect(Rect(x, y, x + wid, y + hit), currentcolor);
    wtextcolor(0);
    wouttextxy(x + 2, y + 1, cbuttfont, text);

    char tbu[2] = "_";
    wouttextxy(x + 2 + wgettextwidth(text, cbuttfont), y + 1, cbuttfont, tbu);
}

int MyTextBox::pressedon()
{
    return 0;
}

int MyTextBox::processmessage(int mcode, int wParam, long lParam)
{
    if (mcode == CTB_SETTEXT) {
        strcpy(text, (char *)lParam);
        needredraw = 1;
    } else if (mcode == CTB_GETTEXT)
        strcpy((char *)lParam, text);
    else if (mcode == CTB_KEYPRESS) {
        if (wParam == 8) {
            if (text[0] != 0)
                text[strlen(text) - 1] = 0;

            drawandmouse();
        } else if (strlen(text) >= TEXTBOX_MAXLEN - 1)
            ;
        else if (wgettextwidth(text, cbuttfont) >= wid - 5)
            ;
        else if (wParam > 127)
            ;  // font only has 128 chars
        else {
            text[strlen(text) + 1] = 0;
            text[strlen(text)] = wParam;
            drawandmouse();
        }
    } else
        return -1;

    return 0;
}
