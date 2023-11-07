//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2023 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__SYS_EVENTS_H
#define __AGS_EE_AC__SYS_EVENTS_H
#include <SDL_keyboard.h>
#include <SDL_events.h>
#include "ac/keycode.h"

// Internal AGS device and event type, made as flags
// NOTE: this matches InputType in script (with a 24-bit shift)
enum InputType
{
    kInputNone      = 0x00,
    // 0x01 is skipped for a purpose, because it has special meaning in script
    kInputKeyboard  = 0x02,
    kInputMouse     = 0x04,
    kInputGamepad   = 0x08,
    kInputAny       = 0xFF
};

// Keyboard input handling
//
// avoid including SDL.h here, at least for now, because that leads to conflicts with allegro
union SDL_Event;

// Converts SDL key data to eAGSKeyCode, which may be also directly used as an ASCII char
// if it is in proper range, see comments to eAGSKeyCode for details.
// Optionally works in bacward compatible mode (old_keyhandle)
KeyInput sdl_keyevt_to_ags_key(const SDL_Event &event, bool old_keyhandle);
// Converts eAGSKeyCode to SDL key scans (up to 3 values, because this is not a 1:1 match);
// NOTE: fails at Ctrl+ or Alt+ AGS keys, or any unknown key codes.
bool ags_key_to_sdl_scan(eAGSKeyCode key, SDL_Scancode(&scan)[3]);

// Tells if key event refers to one of the mod-keys
inline bool is_sdl_mod_key(const SDL_Keysym &key)
{
    return key.scancode == SDL_SCANCODE_LCTRL || key.scancode == SDL_SCANCODE_RCTRL ||
        key.scancode == SDL_SCANCODE_LALT || key.scancode == SDL_SCANCODE_RALT ||
        key.scancode == SDL_SCANCODE_LSHIFT || key.scancode == SDL_SCANCODE_RSHIFT ||
        key.scancode == SDL_SCANCODE_MODE;
}

// Makes a mod flag out of the scan code
inline int make_sdl_mod_flag(const SDL_Keysym &key)
{
    switch (key.scancode)
    {
    case SDL_SCANCODE_LSHIFT: return KMOD_LSHIFT;
    case SDL_SCANCODE_RSHIFT: return KMOD_RSHIFT;
    case SDL_SCANCODE_LCTRL: return KMOD_LCTRL;
    case SDL_SCANCODE_RCTRL: return KMOD_RCTRL;
    case SDL_SCANCODE_LALT: return KMOD_LALT;
    case SDL_SCANCODE_RALT: return KMOD_RALT;
    case SDL_SCANCODE_MODE: return KMOD_MODE;
    default: return KMOD_NONE;
    }
}

// Converts mod key into merged mod (left & right) for easier handling
inline int make_sdl_merged_mod(int mod)
{
    int m_mod = 0;
    if ((mod & KMOD_CTRL) != 0) m_mod |= KMOD_CTRL;
    if ((mod & KMOD_SHIFT) != 0) m_mod |= KMOD_SHIFT;
    if ((mod & KMOD_ALT) != 0) m_mod |= KMOD_ALT;
    // what about KMOD_GUI, and there's also some SDL_SCANCODE_MODE?
    return m_mod;
}

// Tells if there are any buffered input events;
// return the InputType corresponding to the first queued event.
InputType ags_inputevent_ready();
// Queries for the next input event in buffer; returns uninitialized data if none was queued
SDL_Event ags_get_next_inputevent();
// Tells if the key is currently down, provided AGS key.
// NOTE: for particular script codes this function returns positive if either of two keys are down.
int ags_iskeydown(eAGSKeyCode ags_key);
// Simulates key press with the given AGS key
void ags_simulate_keypress(eAGSKeyCode ags_key);

// TODO: hide these later? (bad design with run_service_key_controls,
// but also need to clear them in ags_clear_input_buffer())
extern int sys_modkeys; // accumulated mod flags
extern bool sys_modkeys_fired; // tells whether mod combination had been used for action


// Mouse input handling
// Converts SDL mouse button code to AGS code
eAGSMouseButton sdl_mbut_to_ags_but(int sdl_mbut);
// Tells if the mouse button is currently down
bool ags_misbuttondown(eAGSMouseButton but);
// Returns recent relative mouse movement; resets accumulated values
void ags_mouse_acquire_relxy(int &x, int &y);
// Updates mouse cursor position in game
void ags_domouse();
// Returns -1 for wheel down and +1 for wheel up
// TODO: introduce constants for this
int  ags_check_mouse_wheel();
// Simulates a click with the given mouse button
void ags_simulate_mouseclick(eAGSMouseButton but);

// TODO: hide these later after refactoring mousew32.cpp
extern volatile int sys_mouse_x; // mouse x position
extern volatile int sys_mouse_y; // mouse y position
extern volatile int sys_mouse_z; // mouse wheel position

// Touch input handling
// currently only performs touch-to-mouse emulation
//
// Touch-to-mouse emulation mode
enum TouchMouseEmulation
{
    // don't emulate mouse
    kTouchMouse_None = 0,
    // copy default SDL2 behavior:
    // touch down means hold LMB down, no RMB emulation
    kTouchMouse_OneFingerDrag,
    // tap 1,2 fingers means LMB/RMB click;
    // double tap + drag 1 finger would drag the cursor with LMB down
    kTouchMouse_TwoFingersTap,
    kNumTouchMouseModes
};
// Configures touch to mouse emulation
void ags_touch_set_mouse_emulation(TouchMouseEmulation mode,
    bool relative, float speed);


// Other input utilities
//
// Clears buffered keypresses and mouse clicks;
// resets current key/mb states
void ags_clear_input_state();
// Clears buffered keypresses and mouse clicks, if any;
// does NOT reset current key/mb states
void ags_clear_input_buffer();
// Clears buffered mouse movement
void ags_clear_mouse_movement();


// Events.
//
union SDL_Event;
// Set engine callback for when quit event is received by the backend.
void sys_evt_set_quit_callback(void(*proc)(void));
// Set engine callback for when input focus is received or lost by the window.
void sys_evt_set_focus_callbacks(void(*switch_in)(void), void(*switch_out)(void));

// Process single event.
void sys_evt_process_one(const SDL_Event &event);
// Process all events in the backend's queue.
void sys_evt_process_pending(void);
// Flushes system events following window initialization.
void sys_flush_events(void);

#endif // __AGS_EE_AC__SYS_EVENTS_H
