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

#include "ac/region.h"
#include "ac/common_defines.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_region.h"
#include "ac/room.h"
#include "ac/roomstatus.h"
#include "ac/dynobj/cc_region.h"
#include "ac/dynobj/scriptdrawingsurface.h"
#include "game/roomstruct.h"
#include "script/runtimescriptvalue.h"

using namespace AGS::Common;

extern ScriptRegion scrRegion[MAX_ROOM_REGIONS];
extern RoomStruct thisroom;
extern RoomStatus*croom;
extern GameSetupStruct game;
extern COLOR_MAP maincoltable;
extern RGB palette[256];
extern CCRegion ccDynamicRegion;


ScriptRegion *GetRegionAtRoom(int xx, int yy) {
    return &scrRegion[GetRegionIDAtRoom(xx, yy)];
}

ScriptRegion *GetRegionAtScreen(int x, int y)
{
    VpPoint vpt = play.ScreenToRoomDivDown(x, y);
    if (vpt.second < 0)
        return nullptr;
    return GetRegionAtRoom(vpt.first.X, vpt.first.Y);
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

void Region_Tint(ScriptRegion *srr, int red, int green, int blue, int amount, int luminance)
{
    SetRegionTint(srr->id, red, green, blue, amount, luminance);
}

void Region_TintNoLum(ScriptRegion *srr, int red, int green, int blue, int amount)
{
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

// ScriptRegion *(int xx, int yy)
RuntimeScriptValue Sc_GetRegionAtRoom(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT2(ScriptRegion, ccDynamicRegion, GetRegionAtRoom);
}

RuntimeScriptValue Sc_GetRegionAtScreen(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT2(ScriptRegion, ccDynamicRegion, GetRegionAtScreen);
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



void RegisterRegionAPI()
{
    ScFnRegister region_api[] = {
        { "Region::GetAtRoomXY^2",        API_FN_PAIR(GetRegionAtRoom) },
        { "Region::GetAtScreenXY^2",      API_FN_PAIR(GetRegionAtScreen) },
        { "Region::GetDrawingSurface",    API_FN_PAIR(Region_GetDrawingSurface) },

        { "Region::Tint^4",               API_FN_PAIR(Region_TintNoLum) },
        { "Region::Tint^5",               API_FN_PAIR(Region_Tint) },
        { "Region::RunInteraction^1",     API_FN_PAIR(Region_RunInteraction) },
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
