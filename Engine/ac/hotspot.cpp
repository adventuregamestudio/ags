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

#include "ac/hotspot.h"
#include "util/wgt2allg.h"
#include "ac/draw.h"
#include "ac/roomstruct.h"
#include "ac/global_hotspot.h"
#include "ac/global_translation.h"
#include "ac/properties.h"
#include "ac/roomstatus.h"
#include "ac/string.h"
#include "gfx/bitmap.h"
#include "script/runtimescriptvalue.h"
#include "ac/dynobj/cc_hotspot.h"

using AGS::Common::Bitmap;

extern roomstruct thisroom;
extern RoomStatus*croom;
extern ScriptHotspot scrHotspot[MAX_HOTSPOTS];
extern CCHotspot ccDynamicHotspot;
extern RuntimeScriptValue GlobalReturnValue;

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
    ScriptHotspot *ret_hotspot;
    if (hsnum <= 0)
        ret_hotspot = &scrHotspot[0];
    else
        ret_hotspot = &scrHotspot[hsnum];
    GlobalReturnValue.SetDynamicObject(ret_hotspot, &ccDynamicHotspot);
    return ret_hotspot;
}

void Hotspot_GetName(ScriptHotspot *hss, char *buffer) {
    GetHotspotName(hss->id, buffer);
}

const char* Hotspot_GetName_New(ScriptHotspot *hss) {
    return CreateNewScriptStringAsRetVal(get_translation(thisroom.hotspotnames[hss->id]));
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
    return get_text_property_dynamic_string_as_ret_val(&thisroom.hsProps[hss->id], property);
}

int get_hotspot_at(int xpp,int ypp) {
    int onhs=thisroom.lookat->GetPixel(convert_to_low_res(xpp), convert_to_low_res(ypp));
    if (onhs<0) return 0;
    if (croom->hotspot_enabled[onhs]==0) return 0;
    return onhs;
}
