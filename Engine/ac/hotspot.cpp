
#include "ac/hotspot.h"
#include "wgt2allg.h"
#include "ac/ac_roomstruct.h"
#include "ac/global_hotspot.h"
#include "ac/roomstatus.h"
#include "acmain/ac_customproperties.h"
#include "acmain/ac_draw.h"
#include "acmain/ac_string.h"
#include "acmain/ac_translation.h"

extern roomstruct thisroom;
extern RoomStatus*croom;
extern ScriptHotspot scrHotspot[MAX_HOTSPOTS];

void Hotspot_SetEnabled(ScriptHotspot *hss, int newval) {
    if (newval)
        EnableHotspot(hss->id);
    else
        DisableHotspot(hss->id);
}

int Hotspot_GetEnabled(ScriptHotspot *hss) {
    return croom->hotspot_enabled[hss->id];
}

int Hotspot_GetID(ScriptHotspot *hss) {
    return hss->id;
}

int Hotspot_GetWalkToX(ScriptHotspot *hss) {
    return GetHotspotPointX(hss->id);
}

int Hotspot_GetWalkToY(ScriptHotspot *hss) {
    return GetHotspotPointY(hss->id);
}

ScriptHotspot *GetHotspotAtLocation(int xx, int yy) {
    int hsnum = GetHotspotAt(xx, yy);
    if (hsnum <= 0)
        return &scrHotspot[0];
    return &scrHotspot[hsnum];
}

void Hotspot_GetName(ScriptHotspot *hss, char *buffer) {
    GetHotspotName(hss->id, buffer);
}

const char* Hotspot_GetName_New(ScriptHotspot *hss) {
    return CreateNewScriptString(get_translation(thisroom.hotspotnames[hss->id]));
}

void Hotspot_RunInteraction (ScriptHotspot *hss, int mood) {
    RunHotspotInteraction(hss->id, mood);
}

int Hotspot_GetProperty (ScriptHotspot *hss, const char *property) {
    return get_int_property (&thisroom.hsProps[hss->id], property);
}

void Hotspot_GetPropertyText (ScriptHotspot *hss, const char *property, char *bufer) {
    get_text_property (&thisroom.hsProps[hss->id], property, bufer);
}

const char* Hotspot_GetTextProperty(ScriptHotspot *hss, const char *property) {
    return get_text_property_dynamic_string(&thisroom.hsProps[hss->id], property);
}

int get_hotspot_at(int xpp,int ypp) {
    int onhs=getpixel(thisroom.lookat, convert_to_low_res(xpp), convert_to_low_res(ypp));
    if (onhs<0) return 0;
    if (croom->hotspot_enabled[onhs]==0) return 0;
    return onhs;
}
