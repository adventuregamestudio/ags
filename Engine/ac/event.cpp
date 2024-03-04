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
#include "plugin/agsplugin_evts.h"
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

std::vector<EventHappened> events;

int inside_processevent=0;
int eventClaimed = EVENT_NONE;

const char *tsnames[kTS_Num] = {nullptr, REP_EXEC_NAME, "on_key_press", "on_mouse_click", "on_text_input" };


int run_claimable_event(const char *tsname, bool includeRoom, int numParams, const RuntimeScriptValue *params, bool *eventWasClaimed) {
    *eventWasClaimed = true;
    // Run the room script function, and if it is not claimed,
    // then run the main one
    // We need to remember the eventClaimed variable's state, in case
    // this is a nested event
    int eventClaimedOldValue = eventClaimed;
    eventClaimed = EVENT_INPROGRESS;
    int toret;

    if (includeRoom && roominst) {
        toret = RunScriptFunction(roominst.get(), tsname, numParams, params);

        if (eventClaimed == EVENT_CLAIMED) {
            eventClaimed = eventClaimedOldValue;
            return toret;
        }
    }

    // run script modules
    for (auto &module_inst : moduleInst) {
        toret = RunScriptFunction(module_inst.get(), tsname, numParams, params);

        if (eventClaimed == EVENT_CLAIMED) {
            eventClaimed = eventClaimedOldValue;
            return toret;
        }
    }

    eventClaimed = eventClaimedOldValue;
    *eventWasClaimed = false;
    return 0;
}

// runs the global script on_event function
void run_on_event(int evtype, RuntimeScriptValue &wparam)
{
    RuntimeScriptValue params[]{ evtype , wparam };
    QueueScriptFunction(kScInstGame, "on_event", 2, params);
}

void run_room_event(int id) {
    auto obj_evt = ObjectEvent("room");
    assert(thisroom.EventHandlers);
    run_interaction_script(obj_evt, thisroom.EventHandlers.get(), id);
}

// event list functions
void setevent(int evtyp,int ev1,int ev2,int ev3) {
    EventHappened evt;
    evt.type = evtyp;
    evt.data1 = ev1;
    evt.data2 = ev2;
    evt.data3 = ev3;
    evt.player = game.playercharacter;
    events.push_back(evt);
}

// TODO: this is kind of a hack, which forces event to be processed even if
// it was fired from insides of other event processing.
// The proper solution would be to do the event processing overhaul in AGS.
void force_event(int evtyp,int ev1,int ev2,int ev3)
{
    if (inside_processevent)
        runevent_now(evtyp, ev1, ev2, ev3);
    else
        setevent(evtyp, ev1, ev2, ev3);
}

void process_event(const EventHappened *evp) {
    RuntimeScriptValue rval_null;
    if (evp->type==EV_TEXTSCRIPT) {
        cc_clear_error();
        RuntimeScriptValue params[2]{ evp->data2, evp->data3 };
        if (evp->data3 > -1000)
            QueueScriptFunction(kScInstGame, tsnames[evp->data1], 2, params);
        else if (evp->data2 > -1000)
            QueueScriptFunction(kScInstGame, tsnames[evp->data1], 1, params);
        else
            QueueScriptFunction(kScInstGame, tsnames[evp->data1]);
    }
    else if (evp->type==EV_NEWROOM) {
        NewRoom(evp->data1);
    }
    else if (evp->type==EV_RUNEVBLOCK) {
        PInteractionScripts scriptPtr = nullptr;
        ObjectEvent obj_evt;

        if (evp->data1==EVB_HOTSPOT) {
            const int hotspot_id = evp->data2;
            if (thisroom.Hotspots[hotspot_id].EventHandlers != nullptr)
                scriptPtr = thisroom.Hotspots[hotspot_id].EventHandlers;

            obj_evt = ObjectEvent("hotspot%d", hotspot_id,
                RuntimeScriptValue().SetScriptObject(&scrHotspot[hotspot_id], &ccDynamicHotspot));
            //Debug::Printf("Running hotspot interaction for hotspot %d, event %d", evp->data2, evp->data3);
        }
        else if (evp->data1==EVB_ROOM) {

            if (thisroom.EventHandlers != nullptr)
                scriptPtr = thisroom.EventHandlers;

            obj_evt = ObjectEvent("room");
            if (evp->data3 == EVROM_BEFOREFADEIN) {
                in_enters_screen ++;
                run_on_event (GE_ENTER_ROOM, RuntimeScriptValue().SetInt32(displayed_room));
            } else if (evp->data3 == EVROM_AFTERFADEIN) {
                run_on_event(GE_ENTER_ROOM_AFTERFADE, RuntimeScriptValue().SetInt32(displayed_room));
            }
            //Debug::Printf("Running room interaction, event %d", evp->data3);
        }
        else {
            quit("process_event: RunEvBlock: unknown evb type");
        }

        assert(scriptPtr);
        if (scriptPtr != nullptr)
        {
            run_interaction_script(obj_evt, scriptPtr.get(), evp->data3);
        }

        if ((evp->data1 == EVB_ROOM) && (evp->data3 == EVROM_BEFOREFADEIN))
            in_enters_screen --;
    }
    else if (evp->type==EV_FADEIN)
    {
        current_fade_in_effect();
    }
    else if (evp->type == EV_IFACECLICK)
    {
        process_interface_click(evp->data1, evp->data2, evp->data3);
    }
    else
    {
        quit("process_event: unknown event to process");
    }
}


void runevent_now (int evtyp, int ev1, int ev2, int ev3) {
    EventHappened evh;
    evh.type = evtyp;
    evh.data1 = ev1;
    evh.data2 = ev2;
    evh.data3 = ev3;
    evh.player = game.playercharacter;
    process_event(&evh);
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
    std::vector<EventHappened> evtcopy = events;

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
