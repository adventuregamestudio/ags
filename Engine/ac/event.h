//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
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
#ifndef __AGS_EE_AC__EVENT_H
#define __AGS_EE_AC__EVENT_H

#include "ac/runtime_defines.h"
#include "script/runtimescriptvalue.h"

// parameters to run_on_event
#define GE_LEAVE_ROOM    1
#define GE_ENTER_ROOM    2
// #define GE_MAN_DIES 3 // ancient obsolete event
#define GE_GOT_SCORE     4
#define GE_GUI_MOUSEDOWN 5
#define GE_GUI_MOUSEUP   6
#define GE_ADD_INV       7
#define GE_LOSE_INV      8
#define GE_RESTORE_GAME  9
#define GE_ENTER_ROOM_AFTERFADE 10
#define GE_LEAVE_ROOM_AFTERFADE 11
#define GE_SAVE_GAME     12

// Game event types:
// common script callback
#define EV_TEXTSCRIPT 1
// room event
#define EV_RUNEVBLOCK 2
// fade-in event
#define EV_FADEIN     3
// gui click
#define EV_IFACECLICK 4
// new room event
#define EV_NEWROOM    5
// Text script callback types:
enum kTS_CallbackTypes {
    kTS_None = 0,
// repeatedly execute
    kTS_Repeat,
// on key press
    kTS_KeyPress,
// mouse click
    kTS_MouseClick,
// on text input
    kTS_TextInput,
// script callback types number
    kTS_Num
};

// Room event types:
// hotspot event
#define EVB_HOTSPOT   1
// room own event
#define EVB_ROOM      2
// Room event sub-types:
// room edge crossing
#define EVROM_EDGELEFT     0
#define EVROM_EDGERIGHT    1
#define EVROM_EDGEBOTTOM   2
#define EVROM_EDGETOP      3
// first time enters room
#define EVROM_FIRSTENTER   4
// load room; aka before fade-in
#define EVROM_BEFOREFADEIN 5
// room's rep-exec
#define EVROM_REPEXEC      6
// after fade-in
#define EVROM_AFTERFADEIN  7
// leave room (before fade-out)
#define EVROM_LEAVE        8
// unload room; aka after fade-out
#define EVROM_AFTERFADEOUT 9
// Hotspot event types:
// player stands on hotspot
#define EVHOT_STANDSON  0
// cursor is over hotspot
#define EVHOT_MOUSEOVER 6

struct EventHappened
{
    int type = 0;
    int data1 = 0, data2 = 0, data3 = 0;
    int player = -1;

    EventHappened() = default;
    EventHappened(int type_, int data1_, int data2_, int data3_, int player_)
        : type(type_), data1(data1_), data2(data2_), data3(data3_), player(player_) {}
};

int run_claimable_event(const char *tsname, bool includeRoom, int numParams, const RuntimeScriptValue *params, bool *eventWasClaimed);
// runs the global script on_event fnuction
void run_on_event (int evtype, RuntimeScriptValue &wparam);
void run_room_event(int id);
// event list functions
void setevent(int evtyp,int ev1=0,int ev2=-1000,int ev3=-1000);
void force_event(int evtyp,int ev1=0,int ev2=-1000,int ev3=-1000);
void process_event(const EventHappened *evp);
void runevent_now (int evtyp, int ev1, int ev2, int ev3);
void processallevents();
// end event list functions
void ClaimEvent();

extern int in_enters_screen,done_es_error;
extern int in_leaves_screen;

extern std::vector<EventHappened> events;

extern int eventClaimed;

extern const char*tsnames[kTS_Num];

#endif // __AGS_EE_AC__EVENT_H

