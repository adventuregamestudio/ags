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

#include "event.h"
#include "ac/common.h"
#include "ac/draw.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_game.h"
#include "ac/global_room.h"
#include "ac/global_screen.h"
#include "ac/gui.h"
#include "ac/roomstatus.h"
#include "ac/screen.h"
#include "ac/dynobj/scriptobjects.h"
#include "ac/dynobj/cc_hotspot.h"
#include "debug/debug_log.h"
#include "main/game_run.h"
#include "script/cc_common.h"
#include "platform/base/agsplatformdriver.h"
#include "plugin/plugin_engine.h"
#include "script/script.h"
#include "gfx/bitmap.h"
#include "gfx/ddb.h"
#include "gfx/graphicsdriver.h"
#include "media/audio/audio_system.h"
#include "ac/timer.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern GameSetupStruct game;
extern RoomStruct thisroom;
extern RoomStatus*croom;
extern int displayed_room;
extern RGB palette[256];
extern IGraphicsDriver *gfxDriver;
extern AGSPlatformDriver *platform;
extern int displayed_room;
extern ScriptHotspot scrHotspot[MAX_ROOM_HOTSPOTS];
extern CCHotspot ccDynamicHotspot;
// FIXME: refactor further to get rid of this extern, maybe move part of the code to screen.cpp?
extern std::unique_ptr<Bitmap> saved_viewport_bitmap;

// FIXME: change these from global variables into e.g. GamePlayState members
// TODO: replace these with "current transition state" enum?
bool in_enters_screen = false; // while running "before fade-in" script event
bool done_as_error = false;    // used to report a mistake in "enter room" event
int in_leaves_screen = -1; // while running "before fade-out" script event, stores a next room number
                           // CHECKME: in_leaves_screen seems to be not used for anything?
bool in_room_transition = false; // between previous "before fade-out" and next "after fade-in";
    // used to define a period during which the cursor and "@overhotspot@" labels should be hidden

std::vector<AGSEvent> events;

int inside_processevent=0;
int eventClaimed = EVENT_NONE;

std::array<ScriptEventCallback, kTS_Num> ScriptEventCb = { {
    { nullptr, 0u, kScCallback_Direct },
    { REP_EXEC_NAME, 0u, kScCallback_CommonUnClaimable },
    { "on_key_press", 2u, kScCallback_CommonClaimable },
    { "on_mouse_click", 3u, kScCallback_CommonClaimable },
    { "on_text_input", 1u, kScCallback_CommonClaimable },
    { "on_event", 5u, kScCallback_CommonClaimable },
    { "on_pointer_down", 3u, kScCallback_CommonClaimable },
    { "on_pointer_move", 3u, kScCallback_CommonClaimable },
    { "on_pointer_up", 3u, kScCallback_CommonClaimable },
} };


void run_claimable_event(const String &tsname, bool includeRoom, int numParams, const RuntimeScriptValue *params, bool *eventWasClaimed)
{
    *eventWasClaimed = true;
    // Run the room script function, and if it is not claimed,
    // then run the main one
    // We need to remember the eventClaimed variable's state, in case
    // this is a nested event
    int eventClaimedOldValue = eventClaimed;
    eventClaimed = EVENT_INPROGRESS;

    if (includeRoom && roomscript)
    {
        RunScriptFunction(roomscript.get(), tsname, numParams, params);
        if (eventClaimed == EVENT_CLAIMED)
        {
            eventClaimed = eventClaimedOldValue;
            return;
        }
    }

    // run script modules
    for (auto &module_script : scriptModules)
    {
        RunScriptFunction(module_script.get(), tsname, numParams, params);
        if (eventClaimed == EVENT_CLAIMED)
        {
            eventClaimed = eventClaimedOldValue;
            return;
        }
    }

    eventClaimed = eventClaimedOldValue;
    *eventWasClaimed = false;
}

// runs the global script on_event function
void run_on_event(AGSScriptEventType evtype, int data1, int data2, int data3, int data4)
{
    RuntimeScriptValue params[]{ evtype , data1, data2, data3, data4 };
    RunScriptFunctionAuto(kScTypeGame, "on_event", 5, params);
}

void run_room_event(int id)
{
    auto obj_evt = ObjectEvent(kScTypeRoom);
    run_event_script(obj_evt, &thisroom.GetEvents(), id);
}

// event list functions
void setevent(const AGSEvent &evt)
{
    events.push_back(evt);
}

// TODO: this is kind of a hack, which forces event to be processed even if
// it was fired from insides of other event processing.
// The proper solution would be to do the event processing overhaul in AGS.
void force_event(const AGSEvent &evt)
{
    if (inside_processevent)
        runevent_now(evt);
    else
        setevent(evt);
}

void runevent_now(const AGSEvent &evt)
{
    process_event(&evt);
}

void process_event(const AGSEvent *evp)
{
    const int room_was = play.room_changes;

    RuntimeScriptValue rval_null;
    if (evp->Type == kAGSEvent_Script)
    {
        cc_clear_error();
        const auto &ts = evp->Data.Script;
        if (ts.CbType < kTS_None || ts.CbType >= kTS_Num)
        { // invalid type, internal error?
            quit("process_event: kAGSEvent_Script: unknown callback type");
            return;
        }
        RuntimeScriptValue params[3]{ ts.Arg1, ts.Arg2, ts.Arg3 };
        RunScriptFunctionAuto(kScTypeGame, ScriptEventCb[ts.CbType].FnName, ScriptEventCb[ts.CbType].ArgCount, params);
    }
    else if (evp->Type == kAGSEvent_NewRoom)
    {
        NewRoom(evp->Data.Newroom.RoomID);
    }
    else if (evp->Type == kAGSEvent_Object)
    {
        const auto &obj = evp->Data.Object;
        ScriptEventsBase *obj_events = nullptr;
        ObjectEvent obj_evt;

        switch (obj.ObjEvType)
        {
        case kObjEventType_Hotspot:
        {
            const int hotspot_id = obj.ObjID;
            obj_events = &thisroom.Hotspots[hotspot_id].GetEvents();
            obj_evt = ObjectEvent(kScTypeRoom, LOCTYPE_HOTSPOT, hotspot_id,
                RuntimeScriptValue().SetScriptObject(&scrHotspot[hotspot_id], &ccDynamicHotspot));
            //Debug::Printf("Running hotspot interaction for hotspot %d, event %d", evp->data2, evp->data3);
            break;
        }
        case kObjEventType_Room:
        {
            obj_events = &thisroom.GetEvents();
            obj_evt = ObjectEvent(kScTypeRoom);
            if (obj.ObjEvent == kRoomEvent_BeforeFadein)
            {
                in_enters_screen = true;
                run_on_event(kScriptEvent_RoomEnter, displayed_room);
            }
            else if (obj.ObjEvent == kRoomEvent_FirstEnter)
            {
                in_room_transition = false;
                GUIE::MarkSpecialLabelsForUpdate(kLabelMacro_Overhotspot);
            }
            else if (obj.ObjEvent == kRoomEvent_AfterFadein)
            {
                in_room_transition = false;
                GUIE::MarkSpecialLabelsForUpdate(kLabelMacro_Overhotspot);
                run_on_event(kScriptEvent_RoomAfterFadein, displayed_room);
            }
            //Debug::Printf("Running room interaction, event %d", evp->data3);
            break;
        }
        default:
            // invalid type, internal error?
            quit("process_event: RunEvBlock: unknown evb type");
            break;
        }

        // If the room was changed inside a on_event call above,
        // then skip running further interaction scripts
        if (room_was != play.room_changes)
        {
            if ((obj.ObjEvType == kObjEventType_Room) && (obj.ObjEvent == kRoomEvent_BeforeFadein))
                in_enters_screen = false;
            return;
        }

        assert(obj_events);
        if (obj_events)
        {
            run_event_script(obj_evt, obj_events, obj.ObjEvent);
        }

        if ((obj.ObjEvType == kObjEventType_Room) && (obj.ObjEvent == kRoomEvent_BeforeFadein))
            in_enters_screen = false;
    }
    else if (evp->Type == kAGSEvent_FadeIn)
    {
        current_fade_in_effect();
    }
    else if (evp->Type == kAGSEvent_GUI)
    {
        const auto &gui = evp->Data.Gui;
        process_interface_click(gui.GuiID, gui.GuiObjID, gui.Mbtn);
    }
    else
    {
        quit("process_event: unknown event to process");
    }
}

void processallevents() {
    if (inside_processevent)
    {
        events.clear(); // flush queued events
        return;
    }

    // Make a copy of the events to process them safely.
    // WARNING: engine may actually add more events to the global events array,
    // and they must NOT be processed here, but instead discarded at the end
    // of this function; otherwise game may glitch.
    // TODO: need to redesign engine events system?
    std::vector<AGSEvent> evtcopy = events;

    const int room_was = play.room_changes;

    inside_processevent++;

    for (size_t i = 0; i < evtcopy.size(); ++i) {

        process_event(&evtcopy[i]);

        if (room_was != play.room_changes)
            break;  // changed room, so discard other events
    }

    events.clear();
    inside_processevent--;
}

// end event list functions


void ClaimEvent() {
    if (eventClaimed == EVENT_NONE)
        quit("!ClaimEvent: no event to claim");

    eventClaimed = EVENT_CLAIMED;
}

bool AssertCursorValidForEvent(const char *api_name, int mode)
{
    if (mode < 0 || mode >= game.numcursors)
    {
        debug_script_warn("%s: invalid mode number %d, valid range is 0..%d", api_name, game.numcursors - 1);
        return false;
    }
    return true;
}
