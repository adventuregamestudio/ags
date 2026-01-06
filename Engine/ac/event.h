//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// AGS game events definitions.
//
//=============================================================================
#ifndef __AGS_EE_AC__EVENT_H
#define __AGS_EE_AC__EVENT_H

#include "ac/keycode.h" // eAGSMouseButton
#include "ac/runtime_defines.h"
#include "script/runtimescriptvalue.h"
#include "util/string.h"


// AGS Game event types,
// these events are scheduled during game update to be run in the end of
// the update, after the rest of the game logic has been resolved.
// Event type defines which parameters are used to describe event,
// and whether to run any script callback.
enum AGSGameEventType
{
    kAGSEvent_None              = 0,
    // global script callback (call predefined function in script)
    kAGSEvent_Script            = 1,
    // object event callback (run function attached to object event table)
    kAGSEvent_Object            = 2,
    // fade-in event
    kAGSEvent_FadeIn            = 3,
    // gui interaction
    kAGSEvent_GUI               = 4,
    // new room event
    kAGSEvent_NewRoom           = 5
};

// Text script event callback subtypes:
enum ScriptCallbackType
{
    kTS_None                    = 0,
    // repeatedly execute
    kTS_Repeat                  = 1,
    // on key press
    kTS_KeyPress                = 2,
    // mouse click
    kTS_MouseClick              = 3,
    // on text input
    kTS_TextInput               = 4,
    // script callback types number
    kTS_Num
};

// Interaction event types (more like an object type)
enum ObjectEventType
{
    kObjEventType_None          = 0,
    kObjEventType_Room          = 1,
    kObjEventType_Hotspot       = 2,
    kObjEventType_Region        = 3
};

// Room event indexes
enum RoomEventType
{
    // room edge crossing
    kRoomEvent_EdgeLeft         = 0,
    kRoomEvent_EdgeRight        = 1,
    kRoomEvent_EdgeBottom       = 2,
    kRoomEvent_EdgeTop          = 3,
    // first time enters room
    kRoomEvent_FirstEnter       = 4,
    // load room; aka before fade-in
    kRoomEvent_BeforeFadein     = 5,
    // room's rep-exec
    kRoomEvent_Repexec          = 6,
    // after fade-in
    kRoomEvent_AfterFadein      = 7,
    // leave room (before fade-out)
    kRoomEvent_BeforeFadeout    = 8,
    // unload room; aka after fade-out
    kRoomEvent_AfterFadeout     = 9,
};

// Hotspot event indexes
enum HotspotEventType
{
    // player stands on hotspot
    kHotspotEvent_StandOn       = 0,
    // cursor is over hotspot
    kHotspotEvent_MouseOver     = 6,
};

// Region event indexes
enum RegionEventID
{
    kRegionEvent_Standing       = 0,
    kRegionEvent_WalkOn         = 1,
    kRegionEvent_WalkOff        = 2,
};

// AGS Script events are events reported by a "on_event" callback.
enum AGSScriptEventType
{
    kScriptEvent_RoomLeave      = 1, // before fade-out
    kScriptEvent_RoomEnter      = 2, // before fade-in, right after loaded
    // kScriptEvent_PlayerDies  = 3, // ancient obsolete event
    kScriptEvent_Score          = 4, // score points added
    kScriptEvent_GUIMouseDown   = 5, // mouse button down over gui
    kScriptEvent_GUIMouseUp     = 6, // mouse button up over gui
    kScriptEvent_InventoryAdd   = 7, // inventory added to player char
    kScriptEvent_InventoryLose  = 8, // inventory removed from player char
    kScriptEvent_GameRestored   = 9, // a game save was restored successfully
    kScriptEvent_RoomAfterFadein = 10, // enter after fade-in
    kScriptEvent_RoomAfterFadeout = 11, // after fade-out, right before unloading
    kScriptEvent_GameSaved      = 12, // reports game save result
    kScriptEvent_DialogStart    = 13, // before game enters a "dialog" state
    kScriptEvent_DialogStop     = 14, // after game returns from a "dialog" state
    kScriptEvent_DialogRun      = 15, // a dialog option is run
    kScriptEvent_DialogOptionsOpen = 16, // before dialog options are displayed on screen
    kScriptEvent_DialogOptionsClose = 17, // after dialog options are removed from screen
    kScriptEvent_SavesScanComplete = 18, // after exeuted scheduled saves prescan
};


//-----------------------------------------------------------------------------
// AGSEvent_* structs, used to configure a parent AGSEvent (see below)
//
// AGSEvent_Script describes a scheduled call to a global script callback
struct AGSEvent_Script
{
    ScriptCallbackType CbType = kTS_None;
    int Arg1 = 0, Arg2 = 0, Arg3 = 0;

    AGSEvent_Script() = default;
    AGSEvent_Script(ScriptCallbackType cb, int arg1 = 0, int arg2 = 0, int arg3 = 0)
        : CbType(cb), Arg1(arg1), Arg2(arg2), Arg3(arg3) {}
};

// AGSEvent_Object describes a scheduled call to a object event
struct AGSEvent_Object
{
    ObjectEventType ObjEvType = kObjEventType_None;
    int ObjID = 0;
    int ObjEvent = 0; // object event identifier, depends on ObjID
    int Player = -1; // optional player character id, if event requires that

    AGSEvent_Object() = default;
    AGSEvent_Object(ObjectEventType objtype, int obj_id, int obj_event, int player = -1)
        : ObjEvType(objtype), ObjID(obj_id), ObjEvent(obj_event), Player(player) {}
};

// AGSEvent_GUI describes a scheduled call to a GUI interaction event
struct AGSEvent_GUI
{
    int GuiID = 0; // parent gui's id
    int GuiObjID = 0; // gui child control's id
    eAGSMouseButton Mbtn = kMouseNone;

    AGSEvent_GUI() = default;
    AGSEvent_GUI(int gui_id, int guiobj_id, eAGSMouseButton mbut = kMouseNone)
        : GuiID(gui_id), GuiObjID(guiobj_id), Mbtn(mbut) {}
};

// AGSEvent_NewRoom describes a scheduled call to a NewRoom
struct AGSEvent_NewRoom
{
    int RoomID = -1;

    AGSEvent_NewRoom() = default;
    AGSEvent_NewRoom(int room) : RoomID(room) {}
};

// AGSEvent struct describes one of many possible AGS game events,
// using a union of sub-event structs
struct AGSEvent
{
    // general event type
    AGSGameEventType Type = kAGSEvent_None;
    union EventData
    {
        AGSEvent_Script         Script;
        AGSEvent_Object         Object;
        AGSEvent_GUI            Gui;
        AGSEvent_NewRoom        Newroom;

        EventData() {}
        EventData(const AGSEvent_Script &par) : Script(par) {}
        EventData(const AGSEvent_Object &par) : Object(par) {}
        EventData(const AGSEvent_GUI &par) : Gui(par) {}
        EventData(const AGSEvent_NewRoom &par) : Newroom(par) {}
    } Data;

    AGSEvent() = default;
    // Parameterless event, defined by the type only
    AGSEvent(AGSGameEventType type)
        : Type(type) {}
    AGSEvent(const AGSEvent_Script &evt)
        : Type(kAGSEvent_Script), Data(evt) {}
    AGSEvent(const AGSEvent_Object &evt)
        : Type(kAGSEvent_Object), Data(evt) {}
    AGSEvent(const AGSEvent_GUI &evt)
        : Type(kAGSEvent_GUI), Data(evt) {}
    AGSEvent(const AGSEvent_NewRoom &evt)
        : Type(kAGSEvent_NewRoom), Data(evt) {}
};

void run_claimable_event(const AGS::Common::String &tsname, bool includeRoom, int numParams, const RuntimeScriptValue *params, bool *eventWasClaimed);
// runs the global script on_event function, passing a number of integer parameters
void run_on_event(AGSScriptEventType evtype, int data1 = 0, int data2 = 0, int data3 = 0, int data4 = 0);
void run_room_event(int id);
// event list functions
void setevent(const AGSEvent &evt);
void force_event(const AGSEvent &evt);
void runevent_now(const AGSEvent &evt);
void process_event(const AGSEvent *evp);
void processallevents();
// end event list functions
void ClaimEvent();

extern bool in_enters_screen;
extern bool done_as_error;
extern int in_leaves_screen;
extern bool in_room_transition;

extern std::vector<AGSEvent> events;

extern int eventClaimed;

// ScriptEventCallback describes a predefined script function callback
struct ScriptEventCallback
{
    const char *FnName;
    uint32_t ArgCount;
};

extern ScriptEventCallback ScriptEventCb[kTS_Num];

#endif // __AGS_EE_AC__EVENT_H

