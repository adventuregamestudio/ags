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
#include <stdio.h>
#include "ac/global_hotspot.h"
#include "ac/common.h"
#include "ac/common_defines.h"
#include "ac/characterinfo.h"
#include "ac/draw.h"
#include "ac/event.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_character.h"
#include "ac/global_translation.h"
#include "ac/hotspot.h"
#include "ac/properties.h"
#include "ac/roomstatus.h"
#include "ac/string.h"
#include "ac/dynobj/cc_hotspot.h"
#include "debug/debug_log.h"
#include "game/roomstruct.h"
#include "script/script.h"

using namespace AGS::Common;

extern RoomStruct thisroom;
extern RoomStatus*croom;
extern CharacterInfo*playerchar;
extern GameSetupStruct game;
extern ScriptHotspot scrHotspot[MAX_ROOM_HOTSPOTS];
extern CCHotspot ccDynamicHotspot;


void DisableHotspot(int hsnum) {
    if ((hsnum<1) | (hsnum>=MAX_ROOM_HOTSPOTS))
        quit("!DisableHotspot: invalid hotspot specified");
    croom->hotspot[hsnum].Enabled = false;
    debug_script_log("Hotspot %d disabled", hsnum);
}

void EnableHotspot(int hsnum) {
    if ((hsnum<1) | (hsnum>=MAX_ROOM_HOTSPOTS))
        quit("!EnableHotspot: invalid hotspot specified");
    croom->hotspot[hsnum].Enabled = true;
    debug_script_log("Hotspot %d re-enabled", hsnum);
}

int GetHotspotPointX (int hotspot) {
    if ((hotspot < 0) || (hotspot >= MAX_ROOM_HOTSPOTS))
        quit("!GetHotspotPointX: invalid hotspot");

    if (thisroom.Hotspots[hotspot].WalkTo.X < 1)
        return -1;

    return thisroom.Hotspots[hotspot].WalkTo.X;
}

int GetHotspotPointY (int hotspot) {
    if ((hotspot < 0) || (hotspot >= MAX_ROOM_HOTSPOTS))
        quit("!GetHotspotPointY: invalid hotspot");

    if (thisroom.Hotspots[hotspot].WalkTo.X < 1) // TODO: there was "x" here, why?
        return -1;

    return thisroom.Hotspots[hotspot].WalkTo.Y;
}

int GetHotspotIDAtScreen(int scrx, int scry) {
    VpPoint vpt = play.ScreenToRoomDivDown(scrx, scry);
    if (vpt.second < 0) return 0;
    return get_hotspot_at(vpt.first.X, vpt.first.Y);
}

void GetHotspotName(int hotspot, char *buffer) {
    VALIDATE_STRING(buffer);
    if ((hotspot < 0) || (hotspot >= MAX_ROOM_HOTSPOTS))
        quit("!GetHotspotName: invalid hotspot number");

    snprintf(buffer, MAX_MAXSTRLEN, "%s", get_translation(croom->hotspot[hotspot].Name.GetCStr()));
}

void RunHotspotInteraction (int hotspothere, int mood) {

    // convert cursor mode to event index (in hotspot event table)
    // TODO: probably move this conversion table elsewhere? should be a global info
    // TODO: find out what is hotspot event with index 6 (5 is any-click)
    int evnt;
    switch (mood)
    {
    case MODE_WALK: evnt = 0; break;
    case MODE_LOOK: evnt = 1; break;
    case MODE_HAND: evnt = 2; break;
    case MODE_TALK: evnt = 4; break;
    case MODE_USE: evnt = 3; break;
    case MODE_PICKUP: evnt = 7; break;
    case MODE_CUSTOM1: evnt = 8; break;
    case MODE_CUSTOM2: evnt = 9; break;
    default: evnt = -1; break;
    }
    const int anyclick_evt = 5; // TODO: make global constant (hotspot any-click evt)

    // For USE verb: remember active inventory
    if (mood == MODE_USE)
    {
        play.usedinv = playerchar->activeinv;
    }

    if ((game.options[OPT_WALKONLOOK]==0) & (mood==MODE_LOOK)) ;
    else if (play.auto_use_walkto_points == 0) ;
    else if ((mood!=MODE_WALK) && (play.check_interaction_only == 0))
        MoveCharacterToHotspot(game.playercharacter,hotspothere);

    const auto obj_evt = ObjectEvent("hotspot%d", hotspothere,
        RuntimeScriptValue().SetScriptObject(&scrHotspot[hotspothere], &ccDynamicHotspot), mood);
    if (loaded_game_file_version > kGameVersion_272)
    {
        if ((evnt >= 0) &&
                run_interaction_script(obj_evt, thisroom.Hotspots[hotspothere].EventHandlers.get(), evnt, anyclick_evt) < 0)
            return; // game state changed, don't do "any click"
        run_interaction_script(obj_evt, thisroom.Hotspots[hotspothere].EventHandlers.get(), anyclick_evt); // any click on hotspot
    }
    else
    {
        if ((evnt >= 0) &&
                run_interaction_event(obj_evt, &croom->intrHotspot[hotspothere], evnt, anyclick_evt, (mood == MODE_USE)) < 0)
            return; // game state changed, don't do "any click"
        run_interaction_event(obj_evt, &croom->intrHotspot[hotspothere], anyclick_evt); // any click on hotspot
    }
}

int GetHotspotProperty (int hss, const char *property)
{
    return get_int_property(thisroom.Hotspots[hss].Properties, croom->hsProps[hss], property);
}

void GetHotspotPropertyText (int item, const char *property, char *bufer)
{
    get_text_property(thisroom.Hotspots[item].Properties, croom->hsProps[item], property, bufer);
}
