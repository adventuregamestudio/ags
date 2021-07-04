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
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__SYS_EVENTS_H
#define __AGS_EE_AC__SYS_EVENTS_H
#include "ac/keycode.h"
#include <SDL_keyboard.h>

// AGS own mouse button codes
// TODO: these were internal button codes, but AGS script uses different ones,
// which start with Left=1, and make more sense (0 is easier to use as "no value").
// Must research if there are any dependencies to these internal values, and if not,
// then just replace these matching script ones!
// UPD: even plugin API seem to match script codes and require remap to internals.
// UPD: or use SDL constants in the engine, but make conversion more visible by using a function.
enum eAGSMouseButton
{
    MouseNone     = -1,
    MouseLeft     =  0,
    MouseRight    =  1,
    MouseMiddle   =  2
};


// Keyboard input handling
//
// avoid including SDL.h here, at least for now, because that leads to conflicts with allegro
union SDL_Event;

// Converts SDL key data to eAGSKeyCode, which may be also directly used as an ASCII char
// if it is in proper range, see comments to eAGSKeyCode for details.
KeyInput ags_keycode_from_sdl(const SDL_Event &event);
// Converts eAGSKeyCode to SDL key scans (up to 3 values, because this is not a 1:1 match);
// NOTE: fails at Ctrl+ or Alt+ AGS keys, or any unknown key codes.
bool ags_key_to_sdl_scan(eAGSKeyCode key, SDL_Scancode(&scan)[3]);

// Tells if key event refers to one of the mod-keys
inline bool is_mod_key(const SDL_Keysym &key)
{
    return key.scancode == SDL_SCANCODE_LCTRL || key.scancode == SDL_SCANCODE_RCTRL ||
        key.scancode == SDL_SCANCODE_LALT || key.scancode == SDL_SCANCODE_RALT ||
        key.scancode == SDL_SCANCODE_LSHIFT || key.scancode == SDL_SCANCODE_RSHIFT ||
        key.scancode == SDL_SCANCODE_MODE;
}

// Converts mod key into merged mod (left & right) for easier handling
inline int make_merged_mod(int mod)
{
    int m_mod = 0;
    if ((mod & KMOD_CTRL) != 0) m_mod |= KMOD_CTRL;
    if ((mod & KMOD_SHIFT) != 0) m_mod |= KMOD_SHIFT;
    if ((mod & KMOD_ALT) != 0) m_mod |= KMOD_ALT;
    // what about KMOD_GUI, and there's also some SDL_SCANCODE_MODE?
    return m_mod;
}

// Tells if there are any buffered key events
bool ags_keyevent_ready();
// Queries for the next key event in buffer; returns uninitialized data if none was queued
SDL_Event ags_get_next_keyevent();
// Tells if the key is currently down, provided AGS key;
// Returns positive value if it's down, 0 if it's not, negative value if the key code is not supported.
// NOTE: for particular script codes this function returns positive if either of two keys are down.
int ags_iskeydown(eAGSKeyCode ags_key);
// Simulates key press with the given AGS key
void ags_simulate_keypress(eAGSKeyCode ags_key);


// Mouse input handling
//
// Tells if the mouse button is currently down
bool ags_misbuttondown(int but);
// Returns mouse button code
int  ags_mgetbutton();
// Returns recent relative mouse movement
void ags_mouse_get_relxy(int &x, int &y);
// Updates mouse cursor position in game
void ags_domouse(int what);
// Returns -1 for wheel down and +1 for wheel up
// TODO: introduce constants for this
int  ags_check_mouse_wheel();

// TODO: hide these later after refactoring mousew32.cpp
extern volatile int sys_mouse_x; // mouse x position
extern volatile int sys_mouse_y; // mouse y position
extern volatile int sys_mouse_z; // mouse wheel position


// Other input utilities
//
// Clears buffered keypresses and mouse clicks, if any
void ags_clear_input_buffer();
// Halts execution until any user input
// TODO: seriously not a good design, replace with event listening
void ags_wait_until_keypress();


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

#endif // __AGS_EE_AC__SYS_EVENTS_H
