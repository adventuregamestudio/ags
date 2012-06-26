
#include "ac/region.h"
#include "wgt2allg.h"
#include "ac/ac_defines.h"
#include "ac/ac_gamesetupstruct.h"
#include "ac/ac_roomstruct.h"
#include "ac/global_region.h"
#include "ac/roomstatus.h"


extern ScriptRegion scrRegion[MAX_REGIONS];
extern roomstruct thisroom;
extern RoomStatus*croom;
extern GameSetupStruct game;
extern COLOR_MAP maincoltable;
extern color palette[256];


ScriptRegion *GetRegionAtLocation(int xx, int yy) {
    int hsnum = GetRegionAt(xx, yy);
    if (hsnum <= 0)
        return &scrRegion[0];
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
