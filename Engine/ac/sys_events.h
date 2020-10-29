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

// AGS own mouse button codes
// TODO: these were internal button codes, but AGS script uses different ones,
// which start with Left=1, and make more sense (0 is easier to use as "no value").
// Must research if there are any dependencies to these internal values, and if not,
// then just replace these matching script ones!
// UPD: even plugin API seem to match script codes and require remap to internals.
enum eAGSMouseButton
{
    MouseNone     = -1,
    MouseLeft     =  0,
    MouseRight    =  1,
    MouseMiddle   =  2
};

// Input handling
//
int  ags_getch ();
int  ags_kbhit ();
int  ags_iskeypressed (int keycode);

int  ags_misbuttondown (int but);
int  ags_mgetbutton();
void ags_domouse (int what);
int  ags_check_mouse_wheel ();

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
