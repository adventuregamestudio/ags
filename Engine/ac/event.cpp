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

int in_enters_screen=0,done_es_error = 0;
int in_leaves_screen = -1;

std::vector<AGSEvent> events;

int inside_processevent=0;
int eventClaimed = EVENT_NONE;

ScriptEventCallback ScriptEventCb[kTS_Num] = {
    { nullptr, 0u },
    { REP_EXEC_NAME, 0u },
    { "on_key_press", 2u },
    { "on_mouse_click", 3u },
    { "on_text_input", 1u }
};


void run_claimable_event(const String &tsname, bool includeRoom, int numParams, const RuntimeScriptValue *params, bool *eventWasClaimed)
{
    *eventWasClaimed = true;
    // Run the room script function, and if it is not claimed,
    // then run the main one
    // We need to remember the eventClaimed variable's state, in case
    // this is a nested event
    int eventClaimedOldValue = eventClaimed;
    eventClaimed = EVENT_INPROGRESS;

    if (includeRoom && roominst)
    {
        RunScriptFunction(roominst.get(), tsname, numParams, params);
        if (eventClaimed == EVENT_CLAIMED)
        {
            eventClaimed = eventClaimedOldValue;
            return;
        }
    }

    // run script modules
    for (auto &module_inst : moduleInst)
    {
        RunScriptFunction(module_inst.get(), tsname, numParams, params);
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
    QueueScriptFunction(kScTypeGame, "on_event", 5, params);
}

void run_room_event(int id)
{
    auto obj_evt = ObjectEvent(kScTypeRoom, "room");
    assert(thisroom.EventHandlers);
    run_interaction_script(obj_evt, thisroom.EventHandlers.get(), id);
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
        QueueScriptFunction(kScTypeGame, ScriptEventCb[ts.CbType].FnName, ScriptEventCb[ts.CbType].ArgCount, params);
    }
    else if (evp->Type == kAGSEvent_NewRoom)
    {
        NewRoom(evp->Data.Newroom.RoomID);
    }
    else if (evp->Type == kAGSEvent_Interaction)
    {
        const auto &inter = evp->Data.Inter;
        InteractionEvents *obj_events = nullptr;
        ObjectEvent obj_evt;

        switch (inter.IntEvType)
        {
        case kIntEventType_Hotspot:
        {
            const int hotspot_id = inter.ObjID;
            if (thisroom.Hotspots[hotspot_id].EventHandlers != nullptr)
                obj_events = thisroom.Hotspots[hotspot_id].EventHandlers.get();

            obj_evt = ObjectEvent(kScTypeRoom, "hotspot%d", hotspot_id,
                RuntimeScriptValue().SetScriptObject(&scrHotspot[hotspot_id], &ccDynamicHotspot));
            //Debug::Printf("Running hotspot interaction for hotspot %d, event %d", evp->data2, evp->data3);
            break;
        }
        case kIntEventType_Room:
        {
            if (thisroom.EventHandlers != nullptr)
                obj_events = thisroom.EventHandlers.get();

            obj_evt = ObjectEvent(kScTypeRoom, "room");
            if (inter.ObjEvent == kRoomEvent_BeforeFadein) {
                in_enters_screen ++;
                run_on_event(kScriptEvent_RoomEnter, displayed_room);
            } else if (inter.ObjEvent == kRoomEvent_AfterFadein) {
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

        assert(obj_events);
        if (obj_events)
        {
            run_interaction_script(obj_evt, obj_events, inter.ObjEvent);
        }

        if ((inter.IntEvType == kIntEventType_Room) && (inter.ObjEvent == kRoomEvent_BeforeFadein))
            in_enters_screen --;
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

    int room_was = play.room_changes;

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
