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

#include "ac/common.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/keycode.h"
#include "ac/mouse.h"
#include "ac/sys_events.h"
#include "device/mousew32.h"
#include "platform/base/agsplatformdriver.h"
#include "ac/timer.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern GameSetupStruct game;
extern GameState play;

extern volatile unsigned long globalTimerCounter;
extern int pluginSimulatedClick;
extern int displayed_room;
extern char check_dynamic_sprites_at_exit;

extern void domouse(int str);
extern int mgetbutton();
extern int misbuttondown(int buno);

int mouse_z_was = 0;

int ags_kbhit () {
    int result = keypressed();
    if ((result) && (AGS_Clock::now() < play.ignore_user_input_until_time))
    {
        // ignoring user input
        ags_getch();
        result = 0;
    }
    return result;  
}

int ags_iskeypressed (int keycode) {
    return key[keycode];
}

int ags_misbuttondown (int but) {
    return misbuttondown(but);
}

int ags_mgetbutton() {
    int result;

    if (pluginSimulatedClick > NONE) {
        result = pluginSimulatedClick;
        pluginSimulatedClick = NONE;
    }
    else {
        result = mgetbutton();
    }

    if ((result >= 0) && (AGS_Clock::now() < play.ignore_user_input_until_time))
    {
        // ignoring user input
        result = NONE;
    }

    return result;
}

void ags_domouse (int what) {
    // do mouse is "update the mouse x,y and also the cursor position", unless DOMOUSE_NOCURSOR is set.
    if (what == DOMOUSE_NOCURSOR)
        mgetgraphpos();
    else
        domouse(what);
}

int ags_check_mouse_wheel () {
    int result = 0;
    if ((mouse_z != mouse_z_was) && (game.options[OPT_MOUSEWHEEL] != 0)) {
        if (mouse_z > mouse_z_was)
            result = 1;
        else
            result = -1;
        mouse_z_was = mouse_z;
    }
    return result;
}

int ags_getch() {
    int gott=readkey();
    int scancode = ((gott >> 8) & 0x00ff);

    if (gott == READKEY_CODE_ALT_TAB)
    {
        // Alt+Tab, it gets stuck down unless we do this
        return AGS_KEYCODE_ALT_TAB;
    }

    /*  char message[200];
    sprintf(message, "Scancode: %04X", gott);
    Debug::Printf(message);*/

    /*if ((scancode >= KEY_0_PAD) && (scancode <= KEY_9_PAD)) {
    // fix numeric pad keys if numlock is off (allegro 4.2 changed this behaviour)
    if ((key_shifts & KB_NUMLOCK_FLAG) == 0)
    gott = (gott & 0xff00) | EXTENDED_KEY_CODE;
    }*/

    if ((gott & 0x00ff) == EXTENDED_KEY_CODE) {
        gott = scancode + AGS_EXT_KEY_SHIFT;

        // convert Allegro KEY_* numbers to scan codes
        // (for backwards compatibility we can't just use the
        // KEY_* constants now, it's too late)
        if ((gott>=347) & (gott<=356)) gott+=12;
        // F11-F12
        else if ((gott==357) || (gott==358)) gott+=76;
        // insert / numpad insert
        else if ((scancode == KEY_0_PAD) || (scancode == KEY_INSERT))
            gott = AGS_KEYCODE_INSERT;
        // delete / numpad delete
        else if ((scancode == KEY_DEL_PAD) || (scancode == KEY_DEL))
            gott = AGS_KEYCODE_DELETE;
        // Home
        else if (gott == 378) gott = 371;
        // End
        else if (gott == 379) gott = 379;
        // PgUp
        else if (gott == 380) gott = 373;
        // PgDn
        else if (gott == 381) gott = 381;
        // left arrow
        else if (gott==382) gott=375;
        // right arrow
        else if (gott==383) gott=377;
        // up arrow
        else if (gott==384) gott=372;
        // down arrow
        else if (gott==385) gott=380;
        // numeric keypad
        else if (gott==338) gott=379;
        else if (gott==339) gott=380;
        else if (gott==340) gott=381;
        else if (gott==341) gott=375;
        else if (gott==342) gott=376;
        else if (gott==343) gott=377;
        else if (gott==344) gott=371;
        else if (gott==345) gott=372;
        else if (gott==346) gott=373;
    }
    else
    {
      gott = gott & 0x00ff;
#if defined(MAC_VERSION)
      if (scancode==KEY_BACKSPACE) {
        gott = 8; //j backspace on mac
      }
#endif
    }

    // Alt+X, abort (but only once game is loaded)
    if ((gott == play.abort_key) && (displayed_room >= 0)) {
        check_dynamic_sprites_at_exit = 0;
        quit("!|");
    }

    //sprintf(message, "Keypress: %d", gott);
    //Debug::Printf(message);

    return gott;
}

void ags_clear_input_buffer()
{
    while (ags_kbhit()) ags_getch();
    while (mgetbutton() != NONE);
}

void ags_wait_until_keypress()
{
    while (!ags_kbhit()) {
        platform->YieldCPU();
    }
    ags_getch();
}
