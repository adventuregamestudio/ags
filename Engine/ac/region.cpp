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
#include "ac/region.h"
#include "ac/common.h"
#include "ac/common_defines.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/properties.h"
#include "ac/room.h"
#include "ac/roomstatus.h"
#include "ac/dynobj/cc_region.h"
#include "ac/dynobj/scriptdrawingsurface.h"
#include "ac/dynobj/scriptstring.h"
#include "debug/debug_log.h"
#include "game/roomstruct.h"
#include "script/runtimescriptvalue.h"
#include "script/script.h"

using namespace AGS::Common;

extern ScriptRegion scrRegion[MAX_ROOM_REGIONS];
extern RoomStruct thisroom;
extern RoomStatus*croom;
extern GameSetupStruct game;
extern COLOR_MAP maincoltable;
extern RGB palette[256];
extern CCRegion ccDynamicRegion;


int GetRegionIDAtRoom(int xxx, int yyy)
{
    // if the co-ordinates are off the edge of the screen,
    // correct them to be just within
    // this fixes walk-off-screen problems
    xxx = room_to_mask_coord(xxx);
    yyy = room_to_mask_coord(yyy);

    if (xxx >= thisroom.RegionMask->GetWidth())
        xxx = thisroom.RegionMask->GetWidth() - 1;
    if (yyy >= thisroom.RegionMask->GetHeight())
        yyy = thisroom.RegionMask->GetHeight() - 1;
    if (xxx < 0)
        xxx = 0;
    if (yyy < 0)
        yyy = 0;

    int hsthere = thisroom.RegionMask->GetPixel(xxx, yyy);
    if (hsthere <= 0 || hsthere >= MAX_ROOM_REGIONS) return 0;
    if (croom->region_enabled[hsthere] == 0) return 0;
    return hsthere;
}

ScriptRegion *Region_GetAtRoomXY(int xx, int yy)
{
    return &scrRegion[GetRegionIDAtRoom(xx, yy)];
}

ScriptRegion *Region_GetAtScreenXY(int x, int y)
{
    VpPoint vpt = play.ScreenToRoom(x, y);
    if (vpt.second < 0)
        return &scrRegion[0]; // return region[0] for consistency and backwards compatibility
    return Region_GetAtRoomXY(vpt.first.X, vpt.first.Y);
}

void SetAreaLightLevel(int area, int brightness) {
    if ((area < 0) || (area > MAX_ROOM_REGIONS))
        quit("!SetAreaLightLevel: invalid region");
    if (brightness < -100) brightness = -100;
    if (brightness > 100) brightness = 100;
    thisroom.Regions[area].Light = brightness;
    // disable RGB tint for this area
    thisroom.Regions[area].Tint = 0;
    debug_script_log("Region %d light level set to %d", area, brightness);
}

void Region_SetLightLevel(ScriptRegion *ssr, int brightness) {
    SetAreaLightLevel(ssr->id, brightness);
}

int Region_GetLightLevel(ScriptRegion *ssr) {
    return thisroom.GetRegionLightLevel(ssr->id);
}

int Region_GetTintEnabled(ScriptRegion *srr) {
    if (thisroom.Regions[srr->id].Tint & 0xFF000000)
        return 1;
    return 0;
}

int Region_GetTintRed(ScriptRegion *srr) {

    return thisroom.Regions[srr->id].Tint & 0x000000ff;
}

int Region_GetTintGreen(ScriptRegion *srr) {

    return (thisroom.Regions[srr->id].Tint >> 8) & 0x000000ff;
}

int Region_GetTintBlue(ScriptRegion *srr) {

    return (thisroom.Regions[srr->id].Tint >> 16) & 0x000000ff;
}

int Region_GetTintSaturation(ScriptRegion *srr) {

    return (thisroom.Regions[srr->id].Tint >> 24) & 0xFF;
}

int Region_GetTintLuminance(ScriptRegion *srr)
{
    return thisroom.GetRegionTintLuminance(srr->id);
}

void SetRegionTint(int area, int red, int green, int blue, int amount, int luminance = 100)
{
    if ((area < 0) || (area > MAX_ROOM_REGIONS))
        quit("!SetRegionTint: invalid region");

    if ((red < 0) || (red > 255) || (green < 0) || (green > 255) ||
        (blue < 0) || (blue > 255)) {
        quit("!SetRegionTint: RGB values must be 0-255");
    }

    // originally the value was passed as 0
    // TODO: find out which versions had this; fixup only for past versions in the future!
    if (amount == 0)
        amount = 100;

    if ((amount < 1) || (amount > 100))
        quit("!SetRegionTint: amount must be 1-100");
    if ((luminance < 0) || (luminance > 100))
        quit("!SetRegionTint: luminance must be 0-100");

    debug_script_log("Region %d tint set to %d,%d,%d", area, red, green, blue);

    /*red -= 100;
    green -= 100;
    blue -= 100;*/

    thisroom.Regions[area].Tint = (red & 0xFF) |
        ((green & 0xFF) << 8) |
        ((blue & 0XFF) << 16) |
        ((amount & 0xFF) << 24);
    thisroom.Regions[area].Light = GfxDef::Value100ToValue250(luminance);
}

void Region_Tint(ScriptRegion *srr, int red, int green, int blue, int amount, int luminance)
{
    SetRegionTint(srr->id, red, green, blue, amount, luminance);
}

void Region_TintNoLum(ScriptRegion *srr, int red, int green, int blue, int amount)
{
    SetRegionTint(srr->id, red, green, blue, amount);
}

void DisableRegion(int hsnum)
{
    if ((hsnum < 0) || (hsnum >= MAX_ROOM_REGIONS))
        quit("!DisableRegion: invalid region specified");

    croom->region_enabled[hsnum] = 0;
    debug_script_log("Region %d disabled", hsnum);
}

void EnableRegion(int hsnum)
{
    if ((hsnum < 0) || (hsnum >= MAX_ROOM_REGIONS))
        quit("!EnableRegion: invalid region specified");

    croom->region_enabled[hsnum] = 1;
    debug_script_log("Region %d enabled", hsnum);
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

void RunRegionInteraction(int regnum, int mood) {
    if ((regnum < 0) || (regnum >= MAX_ROOM_REGIONS))
        quit("!RunRegionInteraction: invalid region specified");
    if ((mood < kRegionEvent_Standing) || (mood > kRegionEvent_WalkOff))
        quit("!RunRegionInteraction: invalid event specified");

    // Regions do not react to cursor modes, but this function
    // was historically executing special region events (see RegionEventID)
    const auto obj_evt = ObjectEvent(kScTypeRoom, RuntimeScriptValue().SetScriptObject(&scrRegion[regnum], &ccDynamicRegion));
    run_event_script(obj_evt, &thisroom.Regions[regnum].Events, mood);
}

void Region_RunInteraction(ScriptRegion *ssr, int mood) {
    RunRegionInteraction(ssr->id, mood);
}

int Region_GetProperty(ScriptRegion *ssr, const char *property)
{
    return get_int_property(thisroom.Regions[ssr->id].Properties, croom->regProps[ssr->id], property);
}

const char* Region_GetTextProperty(ScriptRegion *ssr, const char *property)
{
    return get_text_property_dynamic_string(thisroom.Regions[ssr->id].Properties, croom->regProps[ssr->id], property);
}

bool Region_SetProperty(ScriptRegion *ssr, const char *property, int value)
{
    return set_int_property(croom->regProps[ssr->id], property, value);
}

bool Region_SetTextProperty(ScriptRegion *ssr, const char *property, const char *value)
{
    return set_text_property(croom->regProps[ssr->id], property, value);
}

void DisableGroundLevelAreas(int alsoEffects)
{
    if ((alsoEffects < 0) || (alsoEffects > 1))
        quit("!DisableGroundLevelAreas: invalid parameter: must be 0 or 1");

    play.ground_level_areas_disabled = GLED_INTERACTION;

    if (alsoEffects)
        play.ground_level_areas_disabled |= GLED_EFFECTS;

    debug_script_log("Ground-level areas disabled");
}

void EnableGroundLevelAreas()
{
    play.ground_level_areas_disabled = 0;

    debug_script_log("Ground-level areas re-enabled");
}

//=============================================================================

void generate_light_table()
{
    if (game.color_depth == 1 && color_map == nullptr)
    {
        create_light_table(&maincoltable, palette, 0, 0, 0, nullptr);
        color_map = &maincoltable;
    }
}

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "debug/out.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

RuntimeScriptValue Sc_Region_GetAtRoomXY(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT2(ScriptRegion, ccDynamicRegion, Region_GetAtRoomXY);
}

RuntimeScriptValue Sc_Region_GetAtScreenXY(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT2(ScriptRegion, ccDynamicRegion, Region_GetAtScreenXY);
}

RuntimeScriptValue Sc_Region_GetDrawingSurface(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO(ScriptDrawingSurface, Region_GetDrawingSurface);
}

RuntimeScriptValue Sc_Region_Tint(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT5(ScriptRegion, Region_Tint);
}

// void (ScriptRegion *srr, int red, int green, int blue, int amount)
RuntimeScriptValue Sc_Region_TintNoLum(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT4(ScriptRegion, Region_TintNoLum);
}

// void (ScriptRegion *ssr, int mood)
RuntimeScriptValue Sc_Region_RunInteraction(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptRegion, Region_RunInteraction);
}

// int (ScriptRegion *ssr)
RuntimeScriptValue Sc_Region_GetEnabled(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptRegion, Region_GetEnabled);
}

// void (ScriptRegion *ssr, int enable)
RuntimeScriptValue Sc_Region_SetEnabled(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptRegion, Region_SetEnabled);
}

// int (ScriptRegion *ssr)
RuntimeScriptValue Sc_Region_GetID(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptRegion, Region_GetID);
}

// int (ScriptRegion *ssr)
RuntimeScriptValue Sc_Region_GetLightLevel(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptRegion, Region_GetLightLevel);
}

// void (ScriptRegion *ssr, int brightness)
RuntimeScriptValue Sc_Region_SetLightLevel(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptRegion, Region_SetLightLevel);
}

// int (ScriptRegion *srr)
RuntimeScriptValue Sc_Region_GetTintEnabled(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptRegion, Region_GetTintEnabled);
}

// int (ScriptRegion *srr)
RuntimeScriptValue Sc_Region_GetTintBlue(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptRegion, Region_GetTintBlue);
}

// int (ScriptRegion *srr)
RuntimeScriptValue Sc_Region_GetTintGreen(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptRegion, Region_GetTintGreen);
}

// int (ScriptRegion *srr)
RuntimeScriptValue Sc_Region_GetTintRed(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptRegion, Region_GetTintRed);
}

// int (ScriptRegion *srr)
RuntimeScriptValue Sc_Region_GetTintSaturation(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptRegion, Region_GetTintSaturation);
}

RuntimeScriptValue Sc_Region_GetTintLuminance(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptRegion, Region_GetTintLuminance);
}

RuntimeScriptValue Sc_Region_GetProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_POBJ(ScriptRegion, Region_GetProperty, const char);
}

RuntimeScriptValue Sc_Region_GetTextProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_POBJ(ScriptRegion, const char, myScriptStringImpl, Region_GetTextProperty, const char);
}

RuntimeScriptValue Sc_Region_SetProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL_POBJ_PINT(ScriptRegion, Region_SetProperty, const char);
}

RuntimeScriptValue Sc_Region_SetTextProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL_POBJ2(ScriptRegion, Region_SetTextProperty, const char, const char);
}



void RegisterRegionAPI()
{
    ScFnRegister region_api[] = {
        { "Region::GetAtRoomXY^2",        API_FN_PAIR(Region_GetAtRoomXY) },
        { "Region::GetAtScreenXY^2",      API_FN_PAIR(Region_GetAtScreenXY) },
        { "Region::GetDrawingSurface",    API_FN_PAIR(Region_GetDrawingSurface) },

        { "Region::Tint^4",               API_FN_PAIR(Region_TintNoLum) },
        { "Region::Tint^5",               API_FN_PAIR(Region_Tint) },
        { "Region::RunInteraction^1",     API_FN_PAIR(Region_RunInteraction) },
        { "Region::GetProperty^1",             API_FN_PAIR(Region_GetProperty) },
        { "Region::GetTextProperty^1",         API_FN_PAIR(Region_GetTextProperty) },
        { "Region::SetProperty^2",             API_FN_PAIR(Region_SetProperty) },
        { "Region::SetTextProperty^2",         API_FN_PAIR(Region_SetTextProperty) },
        { "Region::get_Enabled",          API_FN_PAIR(Region_GetEnabled) },
        { "Region::set_Enabled",          API_FN_PAIR(Region_SetEnabled) },
        { "Region::get_ID",               API_FN_PAIR(Region_GetID) },
        { "Region::get_LightLevel",       API_FN_PAIR(Region_GetLightLevel) },
        { "Region::set_LightLevel",       API_FN_PAIR(Region_SetLightLevel) },
        { "Region::get_TintEnabled",      API_FN_PAIR(Region_GetTintEnabled) },
        { "Region::get_TintBlue",         API_FN_PAIR(Region_GetTintBlue) },
        { "Region::get_TintGreen",        API_FN_PAIR(Region_GetTintGreen) },
        { "Region::get_TintRed",          API_FN_PAIR(Region_GetTintRed) },
        { "Region::get_TintSaturation",   API_FN_PAIR(Region_GetTintSaturation) },
        { "Region::get_TintLuminance",    API_FN_PAIR(Region_GetTintLuminance) },
    };

    ccAddExternalFunctions(region_api);
}
