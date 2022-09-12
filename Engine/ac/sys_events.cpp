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
#include <chrono>
#include <deque>
#include <SDL.h>
#include "core/platform.h"
#include "ac/common.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/keycode.h"
#include "ac/mouse.h"
#include "ac/timer.h"
#include "device/mousew32.h"
#include "gfx/graphicsdriver.h"
#include "platform/base/agsplatformdriver.h"
#include "platform/base/sys_main.h"
#include "main/engine.h"
#include "util/string_utils.h"
#include "util/utf8.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern GameSetupStruct game;
extern IGraphicsDriver *gfxDriver;

eAGSKeyCode sdl_key_to_ags_key(const SDL_KeyboardEvent &kbevt, bool old_keyhandle);
int sdl_mod_to_ags_mod(const SDL_KeyboardEvent &kbevt);

// Converts SDL scan and key codes to the ags keycode
KeyInput ags_keycode_from_sdl(const SDL_Event &event, bool old_keyhandle)
{
    KeyInput ki;
    // Normally SDL_TEXTINPUT is meant for handling the printable characters,
    // and not the actual key presses.
    // But in the "old key handle" mode we use it also to get the full range
    // of key codes corresponding to chars, including combos (SHIFT + 3 = #).
    // TODO: find out if it's feasible to just convert keydown + SHIFT combos
    // ourselves: then we can utilize SDL_KEYDOWN for both modes and leave
    // TEXTINPUT for char print only.
    switch (event.type)
    {
    case SDL_TEXTINPUT:
        char ascii[sizeof(SDL_TextInputEvent::text)];
        StrUtil::ConvertUtf8ToAscii(event.text.text, "C", &ascii[0], sizeof(ascii));
        if (old_keyhandle && (ascii[0] >= 32))
        {
            ki.Key = static_cast<eAGSKeyCode>(ascii[0]);
            ki.CompatKey = ki.Key;
        }
        strncpy(ki.Text, event.text.text, KeyInput::UTF8_ARR_SIZE);
        Utf8::GetChar(event.text.text, sizeof(SDL_TextInputEvent::text), &ki.UChar);
        return ki;
    case SDL_KEYDOWN:
        ki.Mod = sdl_mod_to_ags_mod(event.key);
        ki.Key = sdl_key_to_ags_key(event.key, old_keyhandle);
        ki.CompatKey = sdl_key_to_ags_key(event.key, true);
        if (!old_keyhandle && (ki.CompatKey == eAGSKeyCodeNone))
            ki.CompatKey = ki.Key; // in the new mode also assign letter-range keys
        return ki;
    default:
        return ki;
    }
}

int sdl_mod_to_ags_mod(const SDL_KeyboardEvent &kbevt)
{
    const SDL_Keysym key = kbevt.keysym;
    const Uint16 mod = key.mod;
    int ags_mod = 0;
    if (mod & KMOD_LSHIFT) ags_mod |= eAGSModLShift;
    if (mod & KMOD_RSHIFT) ags_mod |= eAGSModRShift;
    if (mod & KMOD_LCTRL)  ags_mod |= eAGSModLCtrl;
    if (mod & KMOD_RCTRL)  ags_mod |= eAGSModRCtrl;
    if (mod & KMOD_LALT)   ags_mod |= eAGSModLAlt;
    if (mod & KMOD_RALT)   ags_mod |= eAGSModRAlt;
    if (mod & KMOD_NUM)    ags_mod |= eAGSModNum;
    if (mod & KMOD_CAPS)   ags_mod |= eAGSModCaps;
    return ags_mod;
}

eAGSKeyCode sdl_key_to_ags_key(const SDL_KeyboardEvent &kbevt, bool old_keyhandle)
{
    const SDL_Keysym key = kbevt.keysym;
    const SDL_Keycode sym = key.sym;
    const Uint16 mod = key.mod;

    // NOTE: keycodes such as SDLK_EXCLAIM ('!') may be misleading, as they are NOT
    // received when user presses for example Shift + 1 on regular keyboard, but only on
    // systems where single keypress can produce that symbol.

    // Old mode: Ctrl and Alt combinations realign the letter code to certain offset
    if (old_keyhandle && (sym >= SDLK_a && sym <= SDLK_z))
    {
        if ((mod & KMOD_CTRL) != 0) // align letters to code 1
            return static_cast<eAGSKeyCode>( 0 + (sym - SDLK_a) + 1 );
        else if ((mod & KMOD_ALT) != 0) // align letters to code 301
            return static_cast<eAGSKeyCode>( AGS_EXT_KEY_SHIFT + (sym - SDLK_a) + 1 );
    }
    // New mode: also handle common key range
    else if (!old_keyhandle && (sym >= SDLK_SPACE && sym <= SDLK_z))
    {
        return static_cast<eAGSKeyCode>(sym);
    }

    // Remaining codes may match or not, but we use a big table anyway.
    // TODO: this is code by [sonneveld],
    // double check that we must use scan codes here, maybe can use sdl key (sym) too?
    switch (key.scancode)
    {
    case SDL_SCANCODE_BACKSPACE: return eAGSKeyCodeBackspace;
    case SDL_SCANCODE_TAB:
    case SDL_SCANCODE_KP_TAB: return eAGSKeyCodeTab;
    case SDL_SCANCODE_RETURN:
    case SDL_SCANCODE_RETURN2:
    case SDL_SCANCODE_KP_ENTER: return eAGSKeyCodeReturn;
    case SDL_SCANCODE_ESCAPE: return eAGSKeyCodeEscape;

    case SDL_SCANCODE_F1: return eAGSKeyCodeF1;
    case SDL_SCANCODE_F2: return eAGSKeyCodeF2;
    case SDL_SCANCODE_F3: return eAGSKeyCodeF3;
    case SDL_SCANCODE_F4: return eAGSKeyCodeF4;
    case SDL_SCANCODE_F5: return eAGSKeyCodeF5;
    case SDL_SCANCODE_F6: return eAGSKeyCodeF6;
    case SDL_SCANCODE_F7: return eAGSKeyCodeF7;
    case SDL_SCANCODE_F8: return eAGSKeyCodeF8;
    case SDL_SCANCODE_F9: return eAGSKeyCodeF9;
    case SDL_SCANCODE_F10: return eAGSKeyCodeF10;
    case SDL_SCANCODE_F11: return eAGSKeyCodeF11;
    case SDL_SCANCODE_F12: return eAGSKeyCodeF12;

    case SDL_SCANCODE_KP_7:
    case SDL_SCANCODE_HOME: return eAGSKeyCodeHome;
    case SDL_SCANCODE_KP_8:
    case SDL_SCANCODE_UP: return eAGSKeyCodeUpArrow;
    case SDL_SCANCODE_KP_9:
    case SDL_SCANCODE_PAGEUP: return eAGSKeyCodePageUp;
    case SDL_SCANCODE_KP_4:
    case SDL_SCANCODE_LEFT: return eAGSKeyCodeLeftArrow;
    case SDL_SCANCODE_KP_5: return eAGSKeyCodeNumPad5;
    case SDL_SCANCODE_KP_6:
    case SDL_SCANCODE_RIGHT: return eAGSKeyCodeRightArrow;
    case SDL_SCANCODE_KP_1:
    case SDL_SCANCODE_END: return eAGSKeyCodeEnd;
    case SDL_SCANCODE_KP_2:
    case SDL_SCANCODE_DOWN: return eAGSKeyCodeDownArrow;
    case SDL_SCANCODE_KP_3:
    case SDL_SCANCODE_PAGEDOWN: return eAGSKeyCodePageDown;
    case SDL_SCANCODE_KP_0:
    case SDL_SCANCODE_INSERT: return eAGSKeyCodeInsert;
    case SDL_SCANCODE_KP_PERIOD:
    case SDL_SCANCODE_DELETE: return eAGSKeyCodeDelete;

    case SDL_SCANCODE_LSHIFT: return eAGSKeyCodeLShift;
    case SDL_SCANCODE_RSHIFT: return eAGSKeyCodeRShift;
    case SDL_SCANCODE_LCTRL: return eAGSKeyCodeLCtrl;
    case SDL_SCANCODE_RCTRL: return eAGSKeyCodeRCtrl;
    case SDL_SCANCODE_LALT: return eAGSKeyCodeLAlt;
    case SDL_SCANCODE_RALT: return eAGSKeyCodeRAlt;

    default: return eAGSKeyCodeNone;
    }
}

// Converts ags key to SDL key scans (up to 3 values, because this is not a 1:1 match);
// NOTE: will ignore Ctrl+ or Alt+ script keys.
// TODO: double check and ammend later if anything is missing
bool ags_key_to_sdl_scan(eAGSKeyCode key, SDL_Scancode(&scan)[3])
{
    scan[0] = SDL_SCANCODE_UNKNOWN;
    scan[1] = SDL_SCANCODE_UNKNOWN;
    scan[2] = SDL_SCANCODE_UNKNOWN;
    SDL_Keycode sym = SDLK_UNKNOWN;

    // SDL sym codes happen to match small ASCII letters, so lowercase ours if necessary
    if (key >= eAGSKeyCodeA && key <= eAGSKeyCodeZ)
    {
        sym = static_cast<SDL_Keycode>(key - eAGSKeyCodeA + SDLK_a);
    }
    // Rest of the printable characters seem to match (and match ascii codes)
    else if (key >= eAGSKeyCodeSpace && key <= eAGSKeyCodeBackquote)
    {
        sym = static_cast<SDL_Keycode>(key);
    }

    // If we have got key sym, convert it to SDL scancode using library's function
    if (sym != SDLK_UNKNOWN)
    {
        scan[0] = SDL_GetScancodeFromKey(sym);
        return true;
    }

    // Other keys are mapped directly to scancode (based on [sonneveld]'s code)
    switch (key)
    {
    case eAGSKeyCodeBackspace: scan[0] = SDL_SCANCODE_BACKSPACE; scan[1] = SDL_SCANCODE_KP_BACKSPACE; return true;
    case eAGSKeyCodeTab: scan[0] = SDL_SCANCODE_TAB; scan[1] = SDL_SCANCODE_KP_TAB; return true;
    case eAGSKeyCodeReturn: scan[0] = SDL_SCANCODE_RETURN; scan[1] = SDL_SCANCODE_RETURN2; scan[2] = SDL_SCANCODE_KP_ENTER; return true;
    case eAGSKeyCodeEscape: scan[0] = SDL_SCANCODE_ESCAPE; return true;

    case eAGSKeyCodeF1: scan[0] = SDL_SCANCODE_F1; return true;
    case eAGSKeyCodeF2: scan[0] = SDL_SCANCODE_F2; return true;
    case eAGSKeyCodeF3: scan[0] = SDL_SCANCODE_F3; return true;
    case eAGSKeyCodeF4: scan[0] = SDL_SCANCODE_F4; return true;
    case eAGSKeyCodeF5: scan[0] = SDL_SCANCODE_F5; return true;
    case eAGSKeyCodeF6: scan[0] = SDL_SCANCODE_F6; return true;
    case eAGSKeyCodeF7: scan[0] = SDL_SCANCODE_F7; return true;
    case eAGSKeyCodeF8: scan[0] = SDL_SCANCODE_F8; return true;
    case eAGSKeyCodeF9: scan[0] = SDL_SCANCODE_F9; return true;
    case eAGSKeyCodeF10: scan[0] = SDL_SCANCODE_F10; return true;
    case eAGSKeyCodeF11: scan[0] = SDL_SCANCODE_F11; return true;
    case eAGSKeyCodeF12: scan[0] = SDL_SCANCODE_F12; return true;

    case eAGSKeyCodeHome: scan[0] = SDL_SCANCODE_KP_7; scan[1] = SDL_SCANCODE_HOME; return true;
    case eAGSKeyCodeUpArrow: scan[0] = SDL_SCANCODE_KP_8; scan[1] = SDL_SCANCODE_UP; return true;
    case eAGSKeyCodePageUp: scan[0] = SDL_SCANCODE_KP_9; scan[1] = SDL_SCANCODE_PAGEUP; return true;
    case eAGSKeyCodeLeftArrow: scan[0] = SDL_SCANCODE_KP_4; scan[1] = SDL_SCANCODE_LEFT; return true;
    case eAGSKeyCodeNumPad5: scan[0] = SDL_SCANCODE_KP_5; return true;
    case eAGSKeyCodeRightArrow: scan[0] = SDL_SCANCODE_KP_6; scan[1] = SDL_SCANCODE_RIGHT; return true;
    case eAGSKeyCodeEnd: scan[0] = SDL_SCANCODE_KP_1; scan[1] = SDL_SCANCODE_END; return true;
    case eAGSKeyCodeDownArrow: scan[0] = SDL_SCANCODE_KP_2; scan[1] = SDL_SCANCODE_DOWN; return true;
    case eAGSKeyCodePageDown: scan[0] = SDL_SCANCODE_KP_3; scan[1] = SDL_SCANCODE_PAGEDOWN; return true;
    case eAGSKeyCodeInsert: scan[0] = SDL_SCANCODE_KP_0; scan[1] = SDL_SCANCODE_INSERT; return true;
    case eAGSKeyCodeDelete: scan[0] = SDL_SCANCODE_KP_PERIOD; scan[1] = SDL_SCANCODE_DELETE; return true;

    case eAGSKeyCodeLShift: scan[0] = SDL_SCANCODE_LSHIFT; return true;
    case eAGSKeyCodeRShift: scan[0] = SDL_SCANCODE_RSHIFT; return true;
    case eAGSKeyCodeLCtrl: scan[0] = SDL_SCANCODE_LCTRL; return true;
    case eAGSKeyCodeRCtrl: scan[0] = SDL_SCANCODE_RCTRL; return true;
    case eAGSKeyCodeLAlt: scan[0] = SDL_SCANCODE_LALT; return true;
    case eAGSKeyCodeRAlt: scan[0] = SDL_SCANCODE_RALT; return true;

    default: return false;
    }
}







// ----------------------------------------------------------------------------
// KEYBOARD INPUT
// ----------------------------------------------------------------------------

// Because our game engine still uses input polling, we have to accumulate
// key events for our internal use whenever engine have to query key input.
static std::deque<SDL_Event> g_keyEvtQueue;

int sys_modkeys = 0; // saved accumulated key mods
bool sys_modkeys_fired = false; // saved mod key combination already fired

bool ags_keyevent_ready()
{
    return g_keyEvtQueue.size() > 0;
}

SDL_Event ags_get_next_keyevent()
{
    if (g_keyEvtQueue.size() > 0)
    {
        auto evt = g_keyEvtQueue.front();
        g_keyEvtQueue.pop_front();
        return evt;
    }
    SDL_Event empty = {};
    return empty;
}

int ags_iskeydown(eAGSKeyCode ags_key)
{
    // old input handling: update key state in realtime
    // left only in case if necessary for some ancient game, but
    // this really may only be required if there's a key waiting loop in
    // script without Wait(1) to let engine poll events in a natural way.
    if (game.options[OPT_KEYHANDLEAPI] == 0)
        SDL_PumpEvents();

    const Uint8 *state = SDL_GetKeyboardState(NULL);
    SDL_Scancode scan[3];
    if (!ags_key_to_sdl_scan(ags_key, scan))
        return -1;
    return (state[scan[0]] || state[scan[1]] || state[scan[2]]);
}

void ags_simulate_keypress(eAGSKeyCode ags_key)
{
    SDL_Scancode scan[3];
    if (!ags_key_to_sdl_scan(ags_key, scan))
        return;
    // Push a key event to the event queue; note that this won't affect the key states array
    SDL_Event sdlevent = {};
    sdlevent.type = SDL_KEYDOWN;
    sdlevent.key.keysym.sym = SDL_GetKeyFromScancode(scan[0]);
    sdlevent.key.keysym.scancode = scan[0];
    SDL_PushEvent(&sdlevent);
}

static void on_sdl_key_down(const SDL_Event &event)
{
    // Engine is not structured very well yet, and we cannot pass this event where it's needed;
    // instead we save it in the queue where it will be ready whenever any component asks for one.
    g_keyEvtQueue.push_back(event);
}

static void on_sdl_key_up(const SDL_Event &event)
{
    // Key up events are only used for reacting on mod key combinations at the moment.
    g_keyEvtQueue.push_back(event);
}

static void on_sdl_textinput(const SDL_Event &event)
{
    // We also push text input events to the same queue, as this is only valid way to get proper
    // text interpretation of the pressed key combination based on current system locale.
    g_keyEvtQueue.push_back(event);
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
// For accumulating relative mouse movement to be applied at a poll
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
static eAGSMouseButton mgetbutton()
{
    const int butis = mouse_button_poll();
    if ((butis > 0) & (butwas > 0))
        return kMouseNone;  // don't allow holding button down

    butwas = butis;
    if (butis & MouseBitLeft)
        return kMouseLeft;
    else if (butis & MouseBitRight)
        return kMouseRight;
    else if (butis & MouseBitMiddle)
        return kMouseMiddle;
    return kMouseNone;

    // TODO: presumably this was a hack for 1-button Mac mouse;
    // is this still necessary?
    // find an elegant way to reimplement this; e.g. allow to configure key->mouse mappings?!
#define AGS_SIMULATE_RIGHT_CLICK (AGS_PLATFORM_OS_MACOS)
#if defined (AGS_SIMULATE_RIGHT_CLICK__FIXME)
        // j Ctrl-left click should be right-click
        if (ags_iskeypressed(__allegro_KEY_LCONTROL) || ags_iskeypressed(__allegro_KEY_RCONTROL))
        {
            toret = RIGHT;
        }
#endif
}

extern eAGSMouseButton pluginSimulatedClick;
int mouse_z_was = 0;
// Convert mouse button id to flags
const int MouseButton2Bits[kNumMouseButtons] =
    { 0, MouseBitLeft, MouseBitRight, MouseBitMiddle };

bool ags_misbuttondown(eAGSMouseButton but)
{
    return (mouse_button_poll() & MouseButton2Bits[but]) != 0;
}

eAGSMouseButton ags_mgetbutton()
{
    if (pluginSimulatedClick > kMouseNone)
    {
        eAGSMouseButton mbut = pluginSimulatedClick;
        pluginSimulatedClick = kMouseNone;
        return mbut;
    }
    return mgetbutton();
}

void ags_mouse_get_relxy(int &x, int &y) {
    x = mouse_accum_relx;
    y = mouse_accum_rely;
    mouse_accum_relx = 0;
    mouse_accum_rely = 0;
}

void ags_domouse() {
    mgetgraphpos();
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


// ----------------------------------------------------------------------------
// TOUCH INPUT
// ----------------------------------------------------------------------------

// Accumulated finger bits (a collection of bits shifted by finger index)
static int fingers_down = 0;
// For touch-to-mouse emulation:
static Point touch_mouse_pos;
static bool touch_mouse_ignore_motion = false;
static int touch_mouse_force_button = 0;
static int touch_mouse_drag_down = 0;
// Double tap detection
// The last tapping action timestamp
static auto touch_last_tap_ts = AGS_Clock::now();
static auto touch_last_tap_finger = 0;
static auto touch_tap_count = 0;
static auto touch_quick_tap_delay = std::chrono::milliseconds(300);

// Converts touch finger index to the emulated mouse button
static int tfinger_to_mouse_but(int finger)
{
    switch (finger)
    {
    case 0: return SDL_BUTTON_LEFT;
    case 1: return SDL_BUTTON_RIGHT;
    default: return 0;
    }
}

static void send_mouse_button_event(int evt_type, int button, int x, int y)
{
    SDL_Event evt = {};
    evt.type = evt_type;
    evt.button.which = SDL_TOUCH_MOUSEID;
    evt.button.state = evt_type == SDL_MOUSEBUTTONDOWN ? SDL_PRESSED : SDL_RELEASED;
    evt.button.button = button;
    evt.button.x = x;
    evt.button.y = y;
    SDL_PushEvent(&evt);
}

static void send_mouse_motion_event(int x, int y, int xrel, int yrel)
{
    SDL_Event evt = {};
    evt.type = SDL_MOUSEMOTION;
    evt.motion.which = SDL_TOUCH_MOUSEID;
    evt.motion.x = x;
    evt.motion.y = y;
    evt.motion.xrel = xrel;
    evt.motion.yrel = yrel;
    SDL_PushEvent(&evt);
}

static void detect_double_tap(const SDL_TouchFingerEvent &event, bool down)
{
    auto tap_ts = AGS_Clock::now();
    if ((touch_last_tap_finger == event.fingerId) &&
        (tap_ts < (touch_last_tap_ts + touch_quick_tap_delay)))
    {
        if (down) touch_tap_count++;
    }
    else
    {
        touch_tap_count = down ? 1 : 0;
    }
    touch_last_tap_ts = tap_ts;
    touch_last_tap_finger = event.fingerId;
}

void on_sdl_touch_down(const SDL_TouchFingerEvent &event)
{
    fingers_down |= 1 << event.fingerId;
    Debug::Printf("on_sdl_touch_down: fingers_down = 0x%x", fingers_down);
    detect_double_tap(event, true);

    // TODO: better way to get SDL's logical size? we cannot access sdl renderer here
    int w = gfxDriver->GetDisplayMode().Width;
    int h = gfxDriver->GetDisplayMode().Height;

    switch (usetup.touch_emulate_mouse)
    {
    case kTouchMouse_OneFingerDrag:
    {
        // Touch down means LMB down
        int mouse_but = tfinger_to_mouse_but(event.fingerId);
        if (mouse_but == SDL_BUTTON_LEFT)
        {
            send_mouse_button_event(SDL_MOUSEBUTTONDOWN, mouse_but, event.x * w, event.y * h);
        }
        break;
    }
    case kTouchMouse_TwoFingersTap:
    {
        int mouse_but = tfinger_to_mouse_but(event.fingerId);
        // Handle double tap for drag-n-drop movement
        if ((mouse_but == SDL_BUTTON_LEFT) && (touch_tap_count == 2))
        {
            touch_mouse_drag_down = SDL_BUTTON_LEFT;
            send_mouse_button_event(SDL_MOUSEBUTTONDOWN, touch_mouse_drag_down,
                touch_mouse_pos.X, touch_mouse_pos.Y);
        }
        // If another finger was down, then unpress the drag
        else if (touch_mouse_drag_down > 0)
        {
            touch_mouse_drag_down = 0;
            send_mouse_button_event(SDL_MOUSEBUTTONUP, touch_mouse_drag_down,
                touch_mouse_pos.X, touch_mouse_pos.Y);
        }

        // If only the first finger is down: allow to move the cursor;
        // otherwise, ignore the movement for now
        if ((!touch_mouse_ignore_motion) && (mouse_but == SDL_BUTTON_LEFT))
        {
            touch_mouse_pos = Point(event.x * w, event.y * h);
            send_mouse_motion_event(touch_mouse_pos.X, touch_mouse_pos.Y, 0, 0 /* TODO? */);
        }
        // If more than one finger was down, lock the cursor motion,
        // and force any following emulated clicks to RMB,
        // until *all* the fingers are released
        if (fingers_down != 1)
        {
            touch_mouse_ignore_motion = true;
            touch_mouse_force_button = SDL_BUTTON_RIGHT;
        }
        break;
    }
    default: break; // do nothing
    }
}

void on_sdl_touch_up(const SDL_TouchFingerEvent &event)
{
    fingers_down &= ~(1 << event.fingerId);
    Debug::Printf("on_sdl_touch_up: fingers_down = 0x%x", fingers_down);
    detect_double_tap(event, false);

    // TODO: better way to get SDL's logical size? we cannot access sdl renderer here
    int w = gfxDriver->GetDisplayMode().Width;
    int h = gfxDriver->GetDisplayMode().Height;

    switch (usetup.touch_emulate_mouse)
    {
    case kTouchMouse_OneFingerDrag:
    {
        // Touch up means LMB up
        int mouse_but = tfinger_to_mouse_but(event.fingerId);
        if (mouse_but == SDL_BUTTON_LEFT)
        {
            send_mouse_button_event(SDL_MOUSEBUTTONUP, mouse_but, event.x * w, event.y * h);
        }
        break;
    }
    case kTouchMouse_TwoFingersTap:
    {
        int mouse_but = tfinger_to_mouse_but(event.fingerId);
        // If there's a force button set, then click only if the current button matches it;
        // otherwise use whatever was released
        if ((mouse_but > 0) &&
            ((touch_mouse_force_button == 0) || (mouse_but == touch_mouse_force_button)))
        {
            // If was dragging, then only release
            if (touch_mouse_drag_down)
            {
                send_mouse_button_event(SDL_MOUSEBUTTONUP, touch_mouse_drag_down,
                    touch_mouse_pos.X, touch_mouse_pos.Y);
            }
            // Else perform a "click" (send both mouse button down and up)
            else
            {
                send_mouse_button_event(SDL_MOUSEBUTTONDOWN, mouse_but,
                    touch_mouse_pos.X, touch_mouse_pos.Y);
                send_mouse_button_event(SDL_MOUSEBUTTONUP, mouse_but,
                    touch_mouse_pos.X, touch_mouse_pos.Y);
            }
        }
        // If all fingers are up, reset the locked mouse motion and forced button
        if (fingers_down == 0)
        {
            touch_mouse_ignore_motion = false;
            touch_mouse_force_button = 0;
        }
        break;
    }
    default: break; // do nothing
    }
}

void on_sdl_touch_motion(const SDL_TouchFingerEvent &event)
{
    // TODO: better way to get SDL's logical size? we cannot access sdl renderer here
    int w = gfxDriver->GetDisplayMode().Width;
    int h = gfxDriver->GetDisplayMode().Height;

    switch (usetup.touch_emulate_mouse)
    {
    case kTouchMouse_OneFingerDrag:
    case kTouchMouse_TwoFingersTap:
    {
        // Touch motion means mouse motion;
        // move only if it's the first finger, and motion is not ignored
        int mouse_but = tfinger_to_mouse_but(event.fingerId);
        if ((!touch_mouse_ignore_motion) && (mouse_but == SDL_BUTTON_LEFT))
        {
            touch_mouse_pos = Point(event.x * w, event.y * h);
            send_mouse_motion_event(touch_mouse_pos.X, touch_mouse_pos.Y, 0, 0 /* TODO? */);
        }
    }
    default: break; // do nothing
    }
}



void ags_clear_input_state()
{
    // clear everything related to the input state
    g_keyEvtQueue.clear();
    sys_modkeys = 0;
    sys_modkeys_fired = false;
    mouse_button_state = 0;
    mouse_accum_button_state = 0;
    mouse_clear_at_time = AGS_Clock::now();
    mouse_accum_relx = 0;
    mouse_accum_rely = 0;
}

void ags_clear_input_buffer()
{
    g_keyEvtQueue.clear();
    // accumulated mod keys have to be cleared because they depend on key evt queue
    sys_modkeys = 0;
    sys_modkeys_fired = false;
    // accumulated state only helps to not miss clicks
    mouse_accum_button_state = 0;
    // forget about recent mouse relative movement too
    mouse_accum_relx = 0;
    mouse_accum_rely = 0;
}

void ags_clear_mouse_movement()
{
    mouse_accum_relx = 0;
    mouse_accum_rely = 0;
}

// TODO: this is an awful function that should be removed eventually.
// Must replace with proper updateable game state.
void ags_wait_until_keypress()
{
    do
    {
        sys_evt_process_pending();
        platform->YieldCPU();
    } while (!ags_keyevent_ready());
    ags_clear_input_buffer();
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
        Debug::Printf("SDL event: quit");
        if (_on_quit_callback) {
            _on_quit_callback();
        }
        break;
    // WINDOW
    case SDL_WINDOWEVENT:
        switch (event.window.event)
        {
        case SDL_WINDOWEVENT_FOCUS_GAINED:
            Debug::Printf("Window event: focus gained");
            if (_on_switchin_callback) {
                _on_switchin_callback();
            }
            break;
        case SDL_WINDOWEVENT_FOCUS_LOST:
            Debug::Printf("Window event: focus lost");
            if (_on_switchout_callback) {
                _on_switchout_callback();
            }
            break;
        case SDL_WINDOWEVENT_CLOSE:
            Debug::Printf("Window event: close");
            break;
        case SDL_WINDOWEVENT_SIZE_CHANGED:
            Debug::Printf("Window event: size changed (%d, %d)", event.window.data1, event.window.data2);
            engine_on_window_changed(Size(event.window.data1, event.window.data2));
            break;
        }
        break;
    // KEY INPUT
    case SDL_KEYDOWN:
        on_sdl_key_down(event);
        break;
    case SDL_KEYUP:
        on_sdl_key_up(event);
        break;
    case SDL_TEXTINPUT:
        on_sdl_textinput(event);
        break;
    // MOUSE INPUT
    case SDL_MOUSEMOTION:
        Debug::Printf("SDL_MOUSEMOTION: event.motion.which: %d (%d, %d)", event.motion.which, event.motion.x, event.motion.y);
        on_sdl_mouse_motion(event.motion);
        break;
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
        Debug::Printf("SDL_MOUSEBUTTONDOWN/UP: event.button.which: %d, button = %d, state = %d (%d, %d)",
            event.button.which, event.button.button, event.button.state,
            event.button.x, event.button.y);
        on_sdl_mouse_button(event.button);
        break;
    case SDL_MOUSEWHEEL:
        on_sdl_mouse_wheel(event.wheel);
        break;
    // TOUCH INPUT
    case SDL_FINGERDOWN:
        Debug::Printf("SDL_FINGERDOWN: tfinger: %d at %.3f, %.3f", event.tfinger.fingerId, event.tfinger.x, event.tfinger.y);
        on_sdl_touch_down(event.tfinger);
        break;
    case SDL_FINGERUP:
        Debug::Printf("SDL_FINGERUP: tfinger: %d at %.3f, %.3f", event.tfinger.fingerId, event.tfinger.x, event.tfinger.y);
        on_sdl_touch_up(event.tfinger);
        break;
    case SDL_FINGERMOTION:
        Debug::Printf("SDL_FINGERMOTION: tfinger: %d at %.3f, %.3f", event.tfinger.fingerId, event.tfinger.x, event.tfinger.y);
        on_sdl_touch_motion(event.tfinger);
        break;
    default: break;
    }
}

void sys_evt_process_pending(void) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        sys_evt_process_one(event);
    }
}

void sys_flush_events(void) {
    SDL_PumpEvents();
    SDL_FlushEvent(SDL_WINDOWEVENT);
    SDL_FlushEvents(SDL_KEYDOWN, SDL_TEXTINPUT);
    SDL_FlushEvents(SDL_MOUSEMOTION, SDL_MOUSEWHEEL);
    ags_clear_input_state();
}
