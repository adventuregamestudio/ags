//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "gui/cscidialog.h"
#include <cctype>
#include "ac/common.h"
#include "ac/draw.h"
#include "ac/gamesetup.h"
#include "ac/gamestate.h"
#include "ac/gui.h"
#include "ac/keycode.h"
#include "ac/mouse.h"
#include "ac/sys_events.h"
#include "ac/runtime_defines.h"
#include "font/fonts.h"
#include "gui/guidialog.h"
#include "gui/guimain.h"
#include "gui/mycontrols.h"
#include "main/game_run.h"
#include "gfx/graphicsdriver.h"
#include "gfx/bitmap.h"
#include "media/audio/audio_system.h"
#include "platform/base/agsplatformdriver.h"
#include "ac/timer.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern IGraphicsDriver *gfxDriver;


//-----------------------------------------------------------------------------
// DIALOG SYSTEM STUFF below

int windowbackgroundcolor = COL254, pushbuttondarkcolor = COL255;
int pushbuttonlightcolor = COL253;
int topwindowhandle = -1;
int cbuttfont;

int acdialog_font;

int smcode = 0;

#define MAXCONTROLS 20
#define MAXSCREENWINDOWS 5
NewControl *vobjs[MAXCONTROLS];
OnScreenWindow oswi[MAXSCREENWINDOWS];

int controlid = 0;


//-----------------------------------------------------------------------------

void __my_wbutt(Bitmap *ds, int x1, int y1, int x2, int y2)
{
    color_t draw_color = GUI::GetStandardColorForBitmap(COL254);
    ds->FillRect(Rect(x1, y1, x2, y2), draw_color);
    draw_color = GUI::GetStandardColorForBitmap(0);
    ds->DrawRect(Rect(x1, y1, x2, y2), draw_color);
}

//-----------------------------------------------------------------------------

OnScreenWindow::OnScreenWindow()
{
    x = y = 0;
    handle = -1;
    oldtop = -1;
}

int CSCIGetVersion()
{
    return 0x0100;
}

int windowcount = 0, curswas = 0;
int win_x = 0, win_y = 0, win_width = 0, win_height = 0;
// CLNUP dialogs are based on 320x200 coords, will need to at least center them
int CSCIDrawWindow(int xx, int yy, int wid, int hit)
{
    ignore_bounds++;
    int drawit = -1;
    for (int aa = 0; aa < MAXSCREENWINDOWS; aa++) {
        if (oswi[aa].handle < 0) {
            drawit = aa;
            break;
        }
    }

    if (drawit < 0)
        quit("Too many windows created.");

    windowcount++;
    xx -= 2;
    yy -= 2;
    wid += 4;
    hit += 4;
    Bitmap *ds = prepare_gui_screen(xx, yy, wid, hit, true);
    oswi[drawit].x = xx;
    oswi[drawit].y = yy;
    __my_wbutt(ds, 0, 0, wid - 1, hit - 1);    // wbutt goes outside its area
    oswi[drawit].oldtop = topwindowhandle;
    topwindowhandle = drawit;
    oswi[drawit].handle = topwindowhandle;
    win_x = xx;
    win_y = yy;
    win_width = wid;
    win_height = hit;
    return drawit;
}

void CSCIEraseWindow(int handl)
{
    ignore_bounds--;
    topwindowhandle = oswi[handl].oldtop;
    oswi[handl].handle = -1;
    windowcount--;
    clear_gui_screen();
}

int CSCIWaitMessage(CSCIMessage * cscim)
{
    for (int uu = 0; uu < MAXCONTROLS; uu++) {
        if (vobjs[uu] != nullptr) {
            vobjs[uu]->drawifneeded();
        }
    }

    prepare_gui_screen(win_x, win_y, win_width, win_height, true);

    while (1) {
        sys_evt_process_pending();

        update_audio_system_on_game_loop();
        refresh_gui_screen();

        cscim->id = -1;
        cscim->code = 0;
        smcode = 0;
        // NOTE: CSCIWaitMessage is supposed to report only single message,
        // therefore we cannot process all buffered key presses here
        // (unless the whole dialog system is rewritten).
        KeyInput ki;
        if (run_service_key_controls(ki) && !play.IsIgnoringInput()) {
            int keywas = ki.Key;
            int uchar = ki.UChar;
            if (keywas == eAGSKeyCodeReturn) {
                cscim->id = finddefaultcontrol(CNF_DEFAULT);
                cscim->code = CM_COMMAND;
            } else if (keywas == eAGSKeyCodeEscape) {
                cscim->id = finddefaultcontrol(CNF_CANCEL);
                cscim->code = CM_COMMAND;
            } else if ((uchar == 0) && (keywas < eAGSKeyCodeSpace) && (keywas != eAGSKeyCodeBackspace)) ;
            else if ((keywas >= eAGSKeyCodeUpArrow) & (keywas <= eAGSKeyCodePageDown) & (finddefaultcontrol(CNT_LISTBOX) >= 0))
                vobjs[finddefaultcontrol(CNT_LISTBOX)]->processmessage(CTB_KEYPRESS, keywas, 0);
            else if (finddefaultcontrol(CNT_TEXTBOX) >= 0)
                vobjs[finddefaultcontrol(CNT_TEXTBOX)]->processmessage(CTB_KEYPRESS, keywas, uchar);

            if (cscim->id < 0) {
                cscim->code = CM_KEYPRESS;
                cscim->wParam = keywas;
            }
        }

        eAGSMouseButton mbut;
        if (run_service_mb_controls(mbut) && (mbut > kMouseNone) && !play.IsIgnoringInput()) {
            if (checkcontrols()) {
                cscim->id = controlid;
                cscim->code = CM_COMMAND;
            }
        }

        if (smcode) {
            cscim->code = smcode;
            cscim->id = controlid;
        }

        if (cscim->code > 0)
            break;

        WaitForNextFrame();
    }

    return 0;
}

int CSCICreateControl(int typeandflags, int xx, int yy, int wii, int hii, const char *title)
{
    int usec = -1;
    for (int hh = 1; hh < MAXCONTROLS; hh++) {
        if (vobjs[hh] == nullptr) {
            usec = hh;
            break;
        }
    }

    if (usec < 0)
        quit("Too many controls created");

    int type = typeandflags & 0x00ff;     // 256 control types
    if (type == CNT_PUSHBUTTON) {
        if (wii == -1)
            wii = get_text_width(title, cbuttfont) + 20;

        vobjs[usec] = new MyPushButton(xx, yy, wii, hii, title);

    } else if (type == CNT_LISTBOX) {
        vobjs[usec] = new MyListBox(xx, yy, wii, hii, play.std_gui_textheight);
    } else if (type == CNT_LABEL) {
        vobjs[usec] = new MyLabel(xx, yy, wii, title, play.std_gui_textheight);
    } else if (type == CNT_TEXTBOX) {
        vobjs[usec] = new MyTextBox(xx, yy, wii, title, play.std_gui_textheight);
    } else
        quit("Unknown control type requested");

    vobjs[usec]->typeandflags = typeandflags;
    vobjs[usec]->wlevel = topwindowhandle;
    vobjs[usec]->draw( get_gui_screen() );
    return usec;
}

void CSCIDeleteControl(int haa)
{
    delete vobjs[haa];
    vobjs[haa] = nullptr;
}

int CSCISendControlMessage(int haa, int mess, int wPar, intptr_t ipPar)
{
    if (vobjs[haa] == nullptr)
        return -1;
    return vobjs[haa]->processmessage(mess, wPar, ipPar);
}

// TODO: this is silly, make a uniform formula
int checkcontrols()
{
    // NOTE: this is because old code was working with full game screen
    const int mx = ::mousex - win_x;
    const int my = ::mousey - win_y;

    smcode = 0;
    for (int kk = 0; kk < MAXCONTROLS; kk++) {
        if (vobjs[kk] != nullptr) {
            if (vobjs[kk]->mouseisinarea(mx, my)) {
                controlid = kk;
                return vobjs[kk]->pressedon(mx, my);
            }
        }
    }
    return 0;
}

int finddefaultcontrol(int flagmask)
{
    for (int ff = 0; ff < MAXCONTROLS; ff++) {
        if (vobjs[ff] == nullptr)
            continue;

        if (vobjs[ff]->wlevel != topwindowhandle)
            continue;

        if (vobjs[ff]->typeandflags & flagmask)
            return ff;
    }

    return -1;
}

int GetBaseWidth () {
    return play.GetUIViewport().GetWidth();
}
