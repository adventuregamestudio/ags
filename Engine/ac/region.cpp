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

#include "ac/region.h"
#include "util/wgt2allg.h"
#include "ac/common_defines.h"
#include "ac/gamesetupstruct.h"
#include "ac/roomstruct.h"
#include "ac/global_region.h"
#include "ac/roomstatus.h"
#include "ac/dynobj/cc_region.h"
#include "script/runtimescriptvalue.h"


extern ScriptRegion scrRegion[MAX_REGIONS];
extern roomstruct thisroom;
extern RoomStatus*croom;
extern GameSetupStruct game;
extern COLOR_MAP maincoltable;
extern color palette[256];
extern CCRegion ccDynamicRegion;
extern RuntimeScriptValue GlobalReturnValue;


ScriptRegion *GetRegionAtLocation(int xx, int yy) {
    int hsnum = GetRegionAt(xx, yy);
    if (hsnum <= 0)
        return &scrRegion[0];
    GlobalReturnValue.SetDynamicObject(&scrRegion[hsnum], &ccDynamicRegion);
    return &scrRegion[hsnum];
}

void Region_SetLightLevel(ScriptRegion *ssr, int brightness) {
    SetAreaLightLevel(ssr->id, brightness);
}

int Region_GetLightLevel(ScriptRegion *ssr) {
    return thisroom.regionLightLevel[ssr->id];
}

int Region_GetTintEnabled(ScriptRegion *srr) {
    if (thisroom.regionTintLevel[srr->id] & TINT_IS_ENABLED)
        return 1;
    return 0;
}

int Region_GetTintRed(ScriptRegion *srr) {

    return thisroom.regionTintLevel[srr->id] & 0x000000ff;
}

int Region_GetTintGreen(ScriptRegion *srr) {

    return (thisroom.regionTintLevel[srr->id] >> 8) & 0x000000ff;
}

int Region_GetTintBlue(ScriptRegion *srr) {

    return (thisroom.regionTintLevel[srr->id] >> 16) & 0x000000ff;
}

int Region_GetTintSaturation(ScriptRegion *srr) {

    return thisroom.regionLightLevel[srr->id];
}

void Region_Tint(ScriptRegion *srr, int red, int green, int blue, int amount) {
    SetRegionTint(srr->id, red, green, blue, amount);
}

void Region_SetEnabled(ScriptRegion *ssr, int enable) {
    if (enable)
        EnableRegion(ssr->id);
    else
        DisableRegion(ssr->id);
}

int Region_GetEnabled(ScriptRegion *ssr) {
    return croom->region_enabled[ssr->id];
}

int Region_GetID(ScriptRegion *ssr) {
    return ssr->id;
}

void Region_RunInteraction(ScriptRegion *ssr, int mood) {
    RunRegionInteraction(ssr->id, mood);
}

//=============================================================================

void generate_light_table() {
    int cc;
    if ((game.color_depth == 1) && (color_map == NULL)) {
        // in 256-col mode, check if we need the light table this room
        for (cc=0;cc < MAX_REGIONS;cc++) {
            if (thisroom.regionLightLevel[cc] < 0) {
                create_light_table(&maincoltable,palette,0,0,0,NULL);
                color_map=&maincoltable;
                break;
            }
        }
    }
}
