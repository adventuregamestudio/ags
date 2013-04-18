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

#include "ac/global_hotspot.h"
#include "ac/common.h"
#include "ac/common_defines.h"
#include "ac/characterinfo.h"
#include "ac/draw.h"
#include "ac/event.h"
#include "ac/global_character.h"
#include "ac/global_translation.h"
#include "ac/hotspot.h"
#include "ac/properties.h"
#include "ac/string.h"
#include "debug/debug_log.h"
#include "game/game_objects.h"
#include "script/script.h"

extern int offsetx, offsety;
extern CharacterInfo*playerchar;


void DisableHotspot(int hsnum) {
    if ((hsnum<1) | (hsnum>=MAX_HOTSPOTS))
        quit("!DisableHotspot: invalid hotspot specified");
    croom->Hotspots[hsnum].Enabled=0;
    DEBUG_CONSOLE("Hotspot %d disabled", hsnum);
}

void EnableHotspot(int hsnum) {
    if ((hsnum<1) | (hsnum>=MAX_HOTSPOTS))
        quit("!EnableHotspot: invalid hotspot specified");
    croom->Hotspots[hsnum].Enabled=1;
    DEBUG_CONSOLE("Hotspot %d re-enabled", hsnum);
}

int GetHotspotPointX (int hotspot) {
    if ((hotspot < 0) || (hotspot >= MAX_HOTSPOTS))
        quit("!GetHotspotPointX: invalid hotspot");

    if (thisroom.Hotspots[hotspot].WalkToPoint.x < 1)
        return -1;

    return thisroom.Hotspots[hotspot].WalkToPoint.x;
}

int GetHotspotPointY (int hotspot) {
    if ((hotspot < 0) || (hotspot >= MAX_HOTSPOTS))
        quit("!GetHotspotPointY: invalid hotspot");

    if (thisroom.Hotspots[hotspot].WalkToPoint.x < 1)
        return -1;

    return thisroom.Hotspots[hotspot].WalkToPoint.y;
}

int GetHotspotAt(int xxx,int yyy) {
    xxx += divide_down_coordinate(offsetx);
    yyy += divide_down_coordinate(offsety);
    if ((xxx>=thisroom.Width) | (xxx<0) | (yyy<0) | (yyy>=thisroom.Height))
        return 0;
    return get_hotspot_at(xxx,yyy);
}

void GetHotspotName(int hotspot, char *buffer) {
    VALIDATE_STRING(buffer);
    if ((hotspot < 0) || (hotspot >= MAX_HOTSPOTS))
        quit("!GetHotspotName: invalid hotspot number");

    strcpy(buffer, get_translation(thisroom.Hotspots[hotspot].Name));
}

void RunHotspotInteraction (int hotspothere, int mood) {

    int passon=-1,cdata=-1;
    if (mood==MODE_TALK) passon=4;
    else if (mood==MODE_WALK) passon=0;
    else if (mood==MODE_LOOK) passon=1;
    else if (mood==MODE_HAND) passon=2;
    else if (mood==MODE_PICKUP) passon=7;
    else if (mood==MODE_CUSTOM1) passon = 8;
    else if (mood==MODE_CUSTOM2) passon = 9;
    else if (mood==MODE_USE) { passon=3;
    cdata=playerchar->activeinv;
    play.usedinv=cdata;
    }

    if ((game.Options[OPT_WALKONLOOK]==0) & (mood==MODE_LOOK)) ;
    else if (play.auto_use_walkto_points == 0) ;
    else if ((mood!=MODE_WALK) && (play.check_interaction_only == 0))
        MoveCharacterToHotspot(game.PlayerCharacterIndex,hotspothere);

    // can't use the setevent functions because this ProcessClick is only
    // executed once in a eventlist
    char *oldbasename = evblockbasename;
    int   oldblocknum = evblocknum;

    evblockbasename="hotspot%d";
    evblocknum=hotspothere;

    if (thisroom.Hotspots[hotspothere].EventHandlers.ScriptFnRef) 
    {
        if (passon>=0)
            run_interaction_script(thisroom.Hotspots[hotspothere].EventHandlers.ScriptFnRef, passon, 5, (passon == 3));
        run_interaction_script(thisroom.Hotspots[hotspothere].EventHandlers.ScriptFnRef, 5);  // any click on hotspot
    }
    else
    {
        if (passon>=0) {
            if (run_interaction_event(&croom->Hotspots[hotspothere].Interaction,passon, 5, (passon == 3))) {
                evblockbasename = oldbasename;
                evblocknum = oldblocknum;
                return;
            }
        }
        // run the 'any click on hs' event
        run_interaction_event(&croom->Hotspots[hotspothere].Interaction,5);
    }

    evblockbasename = oldbasename;
    evblocknum = oldblocknum;
}

int GetHotspotProperty (int hss, const char *property) {
    if (hss >= 0)
    {
        return get_int_property (&thisroom.Hotspots[hss].Properties, property);
    }
    return 0;
}
void GetHotspotPropertyText (int item, const char *property, char *bufer) {
    if (item >= 0)
    {
        get_text_property (&thisroom.Hotspots[item].Properties, property, bufer);
    }
    else
    {
        bufer[0] = 0;
    }
}
