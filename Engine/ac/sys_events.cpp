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
#include "ac/sys_events.h"
#include <SDL.h>
#include "core/platform.h"
#include "ac/common.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/keycode.h"
#include "ac/mouse.h"
#include "device/mousew32.h"
#include "platform/base/agsplatformdriver.h"
#include "ac/timer.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern GameSetupStruct game;
extern GameState play;

extern int displayed_room;
extern char check_dynamic_sprites_at_exit;


int ags_kbhit () {
    return keypressed();
}

int ags_iskeypressed (int keycode) {
    if (keycode >= 0 && keycode < __allegro_KEY_MAX)
    {
        return key[keycode] != 0;
    }
    return 0;
}

int ags_getch() {
    const int read_key_value = readkey();
    int gott = read_key_value;
    const int scancode = ((gott >> 8) & 0x00ff);
    const int ascii = (gott & 0x00ff);

    bool is_extended = (ascii == EXTENDED_KEY_CODE);
    // On macos, the extended keycode is the ascii character '?' or '\0' if alt-key
    // so check it's not actually the character '?'
    #if AGS_PLATFORM_OS_MACOS && ! AGS_PLATFORM_OS_IOS
    is_extended = is_extended || ((ascii == EXTENDED_KEY_CODE_MACOS) && (scancode != __allegro_KEY_SLASH));
    #endif

    /*  char message[200];
    sprintf(message, "Scancode: %04X", gott);
    Debug::Printf(message);*/

    /*if ((scancode >= KEY_0_PAD) && (scancode <= KEY_9_PAD)) {
    // fix numeric pad keys if numlock is off (allegro 4.2 changed this behaviour)
    if ((key_shifts & KB_NUMLOCK_FLAG) == 0)
    gott = (gott & 0xff00) | EXTENDED_KEY_CODE;
    }*/

    if (gott == READKEY_CODE_ALT_TAB)
    {
        // Alt+Tab, it gets stuck down unless we do this
        gott = eAGSKeyCodeAltTab;
    }
    #if AGS_PLATFORM_OS_MACOS
    else if (scancode == __allegro_KEY_BACKSPACE) 
    {
        gott = eAGSKeyCodeBackspace;
    }
    #endif
    else if (is_extended) 
    {

        // I believe we rely on a lot of keys being converted to ASCII, which is why
        // the complete scan code list is not here.

        switch(scancode) 
        {
            case __allegro_KEY_F1 : gott = eAGSKeyCodeF1 ; break;
            case __allegro_KEY_F2 : gott = eAGSKeyCodeF2 ; break;
            case __allegro_KEY_F3 : gott = eAGSKeyCodeF3 ; break;
            case __allegro_KEY_F4 : gott = eAGSKeyCodeF4 ; break;
            case __allegro_KEY_F5 : gott = eAGSKeyCodeF5 ; break;
            case __allegro_KEY_F6 : gott = eAGSKeyCodeF6 ; break;
            case __allegro_KEY_F7 : gott = eAGSKeyCodeF7 ; break;
            case __allegro_KEY_F8 : gott = eAGSKeyCodeF8 ; break;
            case __allegro_KEY_F9 : gott = eAGSKeyCodeF9 ; break;
            case __allegro_KEY_F10 : gott = eAGSKeyCodeF10 ; break;
            case __allegro_KEY_F11 : gott = eAGSKeyCodeF11 ; break;
            case __allegro_KEY_F12 : gott = eAGSKeyCodeF12 ; break;

            case __allegro_KEY_INSERT : gott = eAGSKeyCodeInsert ; break;
            case __allegro_KEY_DEL : gott = eAGSKeyCodeDelete ; break;
            case __allegro_KEY_HOME : gott = eAGSKeyCodeHome ; break;
            case __allegro_KEY_END : gott = eAGSKeyCodeEnd ; break;
            case __allegro_KEY_PGUP : gott = eAGSKeyCodePageUp ; break;
            case __allegro_KEY_PGDN : gott = eAGSKeyCodePageDown ; break;
            case __allegro_KEY_LEFT : gott = eAGSKeyCodeLeftArrow ; break;
            case __allegro_KEY_RIGHT : gott = eAGSKeyCodeRightArrow ; break;
            case __allegro_KEY_UP : gott = eAGSKeyCodeUpArrow ; break;
            case __allegro_KEY_DOWN : gott = eAGSKeyCodeDownArrow ; break;

            case __allegro_KEY_0_PAD : gott = eAGSKeyCodeInsert ; break;
            case __allegro_KEY_1_PAD : gott = eAGSKeyCodeEnd ; break;
            case __allegro_KEY_2_PAD : gott = eAGSKeyCodeDownArrow ; break;
            case __allegro_KEY_3_PAD : gott = eAGSKeyCodePageDown ; break;
            case __allegro_KEY_4_PAD : gott = eAGSKeyCodeLeftArrow ; break;
            case __allegro_KEY_5_PAD : gott = eAGSKeyCodeNumPad5 ; break;
            case __allegro_KEY_6_PAD : gott = eAGSKeyCodeRightArrow ; break;
            case __allegro_KEY_7_PAD : gott = eAGSKeyCodeHome ; break;
            case __allegro_KEY_8_PAD : gott = eAGSKeyCodeUpArrow ; break;
            case __allegro_KEY_9_PAD : gott = eAGSKeyCodePageUp ; break;
            case __allegro_KEY_DEL_PAD : gott = eAGSKeyCodeDelete ; break;

            default: 
                // no meaningful mappings
                // this is how we accidentally got the alt-key mappings
                gott = scancode + AGS_EXT_KEY_SHIFT;
        }
    }
    else
    {
        // this includes ascii characters and ctrl-A-Z
        gott = ascii;
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


// ----------------------------------------------------------------------------
// MOUSE INPUT
// ----------------------------------------------------------------------------
volatile int sys_mouse_x = 0; // mouse x position
volatile int sys_mouse_y = 0; // mouse y position
volatile int sys_mouse_z = 0; // mouse wheel position


// TODO: check later, if this may be useful in other places (then move to header)
enum eAGSMouseButtonMask
{
    MouseBitLeft     = 0x01,
    MouseBitRight    = 0x02,
    MouseBitMiddle   = 0x04,
    MouseBitX1       = 0x08,
    MouseBitX2       = 0x10
};

static int sdl_button_to_mask(int button)
{
    switch (button) {
    case SDL_BUTTON_LEFT: return MouseBitLeft;
    case SDL_BUTTON_RIGHT: return MouseBitRight;
    case SDL_BUTTON_MIDDLE: return MouseBitMiddle;
    case SDL_BUTTON_X1: return MouseBitX1;
    case SDL_BUTTON_X2: return MouseBitX2;
    }
    return 0;
}

/* [sonneveld]
Button tracking:
On OSX, some tap to click up/down events happen too quickly to be detected on the polled mouse_b global variable.
Instead we accumulate button presses over a couple of timer loops.
// TODO: check again when/if we replace polling with different event handling.
*/
static int mouse_button_state = 0;
static int mouse_accum_button_state = 0;
static auto mouse_clear_at_time = AGS_Clock::now();
static int mouse_accum_relx = 0, mouse_accum_rely = 0;

// Returns accumulated mouse button state and clears internal cache by timer
static int mouse_button_poll()
{
    auto now = AGS_Clock::now();
    int result = mouse_button_state | mouse_accum_button_state;
    if (now >= mouse_clear_at_time) {
        mouse_accum_button_state = 0;
        mouse_clear_at_time = now + std::chrono::milliseconds(50);
    }
    return result;
}

static void on_sdl_mouse_motion(const SDL_MouseMotionEvent &event) {
    sys_mouse_x = event.x;
    sys_mouse_y = event.y;
    mouse_accum_relx += event.xrel;
    mouse_accum_rely += event.yrel;
}

static void on_sdl_mouse_button(const SDL_MouseButtonEvent &event)
{
    sys_mouse_x = event.x;
    sys_mouse_y = event.y;

    if (event.type == SDL_MOUSEBUTTONDOWN) {
        mouse_button_state |= sdl_button_to_mask(event.button);
        mouse_accum_button_state |= sdl_button_to_mask(event.button);
    }
    else {
        mouse_button_state &= ~sdl_button_to_mask(event.button);
    }
}

static void on_sdl_mouse_wheel(const SDL_MouseWheelEvent &event)
{
    sys_mouse_z += event.y;
}

int butwas = 0;
int mgetbutton()
{
    int toret = MouseNone;
    int butis = mouse_button_poll();

    if ((butis > 0) & (butwas > 0))
        return MouseNone;  // don't allow holding button down

    if (butis & MouseBitLeft)
        toret = MouseLeft;
    else if (butis & MouseBitRight)
        toret = MouseRight;
    else if (butis & MouseMiddle)
        toret = MouseMiddle;

    butwas = butis;
    return toret;

    // TODO: presumably this was a hack for 1-button Mac mouse;
    // is this still necessary?
    // find an elegant way to reimplement this; e.g. allow to configure key->mouse mappings?!
#define AGS_SIMULATE_RIGHT_CLICK (AGS_PLATFORM_OS_MACOS)
#if AGS_SIMULATE_RIGHT_CLICK__FIXME
        // j Ctrl-left click should be right-click
        if (ags_iskeypressed(__allegro_KEY_LCONTROL) || ags_iskeypressed(__allegro_KEY_RCONTROL))
        {
            toret = RIGHT;
        }
#endif
}

extern int pluginSimulatedClick;
extern void domouse(int str);
int mouse_z_was = 0;
const int MB_ARRAY[3] = { MouseBitLeft, MouseBitRight, MouseBitMiddle };

bool ags_misbuttondown(int but)
{
    return mouse_button_poll() & MB_ARRAY[but];
}

int ags_mgetbutton() {
    int result;

    if (pluginSimulatedClick > MouseNone) {
        result = pluginSimulatedClick;
        pluginSimulatedClick = MouseNone;
    }
    else {
        result = mgetbutton();
    }
    return result;
}

void ags_mouse_get_relxy(int &x, int &y) {
    x = mouse_accum_relx;
    y = mouse_accum_rely;
    mouse_accum_relx = 0;
    mouse_accum_rely = 0;
}

void ags_domouse(int what) {
    // do mouse is "update the mouse x,y and also the cursor position", unless DOMOUSE_NOCURSOR is set.
    if (what == DOMOUSE_NOCURSOR)
        mgetgraphpos();
    else
        domouse(what);
}

int ags_check_mouse_wheel() {
    if (game.options[OPT_MOUSEWHEEL] == 0) { return 0; }
    if (sys_mouse_z == mouse_z_was) { return 0; }

    int result = 0;
    if (sys_mouse_z > mouse_z_was)
        result = 1;   // eMouseWheelNorth
    else
        result = -1;  // eMouseWheelSouth
    mouse_z_was = sys_mouse_z;
    return result;
}



void ags_clear_input_buffer()
{
    while (ags_kbhit()) ags_getch();
    mouse_button_state = 0;
    mouse_accum_button_state = 0;
    mouse_clear_at_time = AGS_Clock::now() + std::chrono::milliseconds(50);
    mouse_accum_relx = 0;
    mouse_accum_rely = 0;
}

void ags_wait_until_keypress()
{
    while (!ags_kbhit()) {
        platform->YieldCPU();
    }
    ags_getch();
}


// ----------------------------------------------------------------------------
// EVENTS
// ----------------------------------------------------------------------------
static void(*_on_quit_callback)(void) = nullptr;
static void(*_on_switchin_callback)(void) = nullptr;
static void(*_on_switchout_callback)(void) = nullptr;

void sys_evt_set_quit_callback(void(*proc)(void)) {
    _on_quit_callback = proc;
}

void sys_evt_set_focus_callbacks(void(*switch_in)(void), void(*switch_out)(void)) {
    _on_switchin_callback = switch_in;
    _on_switchout_callback = switch_out;
}

void sys_evt_process_one(const SDL_Event &event) {
    switch (event.type) {
    // GENERAL
    case SDL_QUIT:
        if (_on_quit_callback) {
            _on_quit_callback();
        }
        break;
    // WINDOW
    case SDL_WINDOWEVENT:
        switch (event.window.event)
        {
        case SDL_WINDOWEVENT_FOCUS_GAINED:
            if (_on_switchin_callback) {
                _on_switchin_callback();
            }
            break;
        case SDL_WINDOWEVENT_FOCUS_LOST:
            if (_on_switchout_callback) {
                _on_switchout_callback();
            }
            break;
        }
        break;
    // INPUT
    case SDL_MOUSEMOTION:
        on_sdl_mouse_motion(event.motion);
        break;
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
        on_sdl_mouse_button(event.button);
        break;
    case SDL_MOUSEWHEEL:
        on_sdl_mouse_wheel(event.wheel);
        break;
    }
}

void sys_evt_process_pending(void) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        sys_evt_process_one(event);
    }
}
