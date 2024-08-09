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
#include "ac/common.h"
#include "ac/object.h"
#include "ac/character.h"
#include "ac/gamestate.h"
#include "ac/gamesetupstruct.h"
#include "ac/global_walkablearea.h"
#include "ac/object.h"
#include "ac/properties.h"
#include "ac/room.h"
#include "ac/roomobject.h"
#include "ac/roomstatus.h"
#include "ac/walkablearea.h"
#include "ac/dynobj/cc_walkablearea.h"
#include "game/roomstruct.h"
#include "gfx/bitmap.h"

using namespace AGS::Common;

extern RoomStruct thisroom;
extern GameSetupStruct game;
extern int displayed_room;
extern RoomStatus*croom;
extern RoomObject*objs;
extern ScriptWalkableArea scrWalkarea[MAX_WALK_AREAS];
extern CCWalkableArea ccDynamicWalkarea;

Bitmap *walkareabackup=nullptr, *walkable_areas_temp = nullptr;

void redo_walkable_areas()
{
    thisroom.WalkAreaMask->Blit(walkareabackup, 0, 0);
    for (int h = 0; h < walkareabackup->GetHeight(); ++h)
    {
        uint8_t *walls_scanline = thisroom.WalkAreaMask->GetScanLineForWriting(h);
        for (int w = 0; w < walkareabackup->GetWidth(); ++w)
        {
            if ((walls_scanline[w] >= sizeof(play.walkable_areas_on)) ||
                    (play.walkable_areas_on[walls_scanline[w]] == 0))
                walls_scanline[w] = 0;
        }
    }
}

int get_walkable_area_pixel(int x, int y)
{
    return thisroom.WalkAreaMask->GetPixel(room_to_mask_coord(x), room_to_mask_coord(y));
}

int get_area_scaling (int onarea, int xx, int yy) {

    int zoom_level = 100;
    xx = room_to_mask_coord(xx);
    yy = room_to_mask_coord(yy);

    if ((onarea >= 0) && (onarea < MAX_WALK_AREAS) &&
        (thisroom.WalkAreas[onarea].ScalingNear != NOT_VECTOR_SCALED)) {
            // We have vector scaling!
            // In case the character is off the screen, limit the Y co-ordinate
            // to within the area range (otherwise we get silly zoom levels
            // that cause Out Of Memory crashes)
            if (yy > thisroom.WalkAreas[onarea].Bottom)
                yy = thisroom.WalkAreas[onarea].Bottom;
            if (yy < thisroom.WalkAreas[onarea].Top)
                yy = thisroom.WalkAreas[onarea].Top;
            // Work it all out without having to use floats
            // Percent = ((y - top) * 100) / (areabottom - areatop)
            // Zoom level = ((max - min) * Percent) / 100
            if (thisroom.WalkAreas[onarea].Bottom != thisroom.WalkAreas[onarea].Top)
            {
                int percent = ((yy - thisroom.WalkAreas[onarea].Top) * 100)
                    / (thisroom.WalkAreas[onarea].Bottom - thisroom.WalkAreas[onarea].Top);
                zoom_level = ((thisroom.WalkAreas[onarea].ScalingNear - thisroom.WalkAreas[onarea].ScalingFar) * (percent)) / 100 + thisroom.WalkAreas[onarea].ScalingFar;
            }
            else
            {
                // Special case for 1px tall walkable area: take bottom line scaling
                zoom_level = thisroom.WalkAreas[onarea].ScalingNear;
            }
            zoom_level += 100;
    }
    else if ((onarea >= 0) & (onarea < MAX_WALK_AREAS))
        zoom_level = thisroom.WalkAreas[onarea].ScalingFar + 100;

    if (zoom_level == 0)
        zoom_level = 100;

    return zoom_level;
}

void scale_sprite_size(int sppic, int zoom_level, int *newwidth, int *newheight) {
    newwidth[0] = (game.SpriteInfos[sppic].Width * zoom_level) / 100;
    newheight[0] = (game.SpriteInfos[sppic].Height * zoom_level) / 100;
    if (newwidth[0] < 1)
        newwidth[0] = 1;
    if (newheight[0] < 1)
        newheight[0] = 1;
}

void remove_walkable_areas_from_temp(int fromx, int cwidth, int starty, int endy) {
    fromx = room_to_mask_coord(fromx);
    cwidth = room_to_mask_coord(cwidth);
    starty = room_to_mask_coord(starty);
    endy = room_to_mask_coord(endy);
    int yyy;
    if (endy >= walkable_areas_temp->GetHeight())
        endy = walkable_areas_temp->GetHeight() - 1;
    if (starty < 0)
        starty = 0;

    for (; cwidth > 0; cwidth --) {
        for (yyy = starty; yyy <= endy; yyy++)
            walkable_areas_temp->PutPixel (fromx, yyy, 0);
        fromx ++;
    }

}

int is_point_in_rect(int x, int y, int left, int top, int right, int bottom) {
    if ((x >= left) && (x < right) && (y >= top ) && (y <= bottom))
        return 1;
    return 0;
}

// TODO: possibly optimize this by caching and/or knowing if walkable area changed since the last call?
Bitmap *prepare_walkable_areas (int sourceChar) {
    // copy the walkable areas to the temp bitmap
    walkable_areas_temp->Blit(thisroom.WalkAreaMask.get(), 0,0,0,0,thisroom.WalkAreaMask->GetWidth(),thisroom.WalkAreaMask->GetHeight());
    // if the character who's moving doesn't block, don't bother checking
    if (sourceChar < 0) ;
    else if (game.chars[sourceChar].flags & CHF_NOBLOCKING)
        return walkable_areas_temp;

    // for each character in the current room, make the area under them unwalkable
    for (int ww = 0; ww < game.numcharacters; ww++) {
        if (!game.chars[ww].is_enabled()) continue;
        if (game.chars[ww].room != displayed_room) continue;
        if (ww == sourceChar) continue;
        if (game.chars[ww].flags & CHF_NOBLOCKING) continue;
        if (room_to_mask_coord(game.chars[ww].y) >= walkable_areas_temp->GetHeight()) continue;
        if (room_to_mask_coord(game.chars[ww].x) >= walkable_areas_temp->GetWidth()) continue;
        if ((game.chars[ww].y < 0) || (game.chars[ww].x < 0)) continue;

        CharacterInfo *char1 = &game.chars[ww];
        int cwidth, fromx;

        if (is_char_on_another(sourceChar, ww, &fromx, &cwidth))
            continue;
        if ((sourceChar >= 0) && (is_char_on_another(ww, sourceChar, nullptr, nullptr)))
            continue;

        remove_walkable_areas_from_temp(fromx, cwidth, char1->get_blocking_top(), char1->get_blocking_bottom());
    }

    // check for any blocking objects in the room, and deal with them as well
    for (uint32_t ww = 0; ww < croom->numobj; ww++) {
        if (!objs[ww].is_enabled()) continue;
        if ((objs[ww].flags & OBJF_SOLID) == 0)
            continue;
        if (room_to_mask_coord(objs[ww].y) >= walkable_areas_temp->GetHeight()) continue;
        if (room_to_mask_coord(objs[ww].x) >= walkable_areas_temp->GetWidth()) continue;
        if ((objs[ww].y < 0) || (objs[ww].x < 0)) continue;

        int x1, y1, width, y2;
        get_object_blocking_rect(ww, &x1, &y1, &width, &y2);

        // if the character is currently standing on the object, ignore
        // it so as to allow him to escape
        if ((sourceChar >= 0) &&
            (is_point_in_rect(game.chars[sourceChar].x, game.chars[sourceChar].y, 
            x1, y1, x1 + width, y2)))
            continue;

        remove_walkable_areas_from_temp(x1, width, y1, y2);
    }

    return walkable_areas_temp;
}

// return the walkable area at the character's feet, taking into account
// that he might just be off the edge of one
int get_walkable_area_at_location(int xx, int yy) {

    int onarea = get_walkable_area_pixel(xx, yy);

    if (onarea < 0) {
        // the character has walked off the edge of the screen, so stop them
        // jumping up to full size when leaving
        if (xx >= thisroom.Width)
            onarea = get_walkable_area_pixel(thisroom.Width-1, yy);
        else if (xx < 0)
            onarea = get_walkable_area_pixel(0, yy);
        else if (yy >= thisroom.Height)
            onarea = get_walkable_area_pixel(xx, thisroom.Height - 1);
        else if (yy < 0)
            onarea = get_walkable_area_pixel(xx, 1);
    }
    if (onarea==0) {
        // the path finder sometimes slightly goes into non-walkable areas;
        // so check for scaling in adjacent pixels
        const int TRYGAP=2;
        onarea = get_walkable_area_pixel(xx + TRYGAP, yy);
        if (onarea<=0)
            onarea = get_walkable_area_pixel(xx - TRYGAP, yy);
        if (onarea<=0)
            onarea = get_walkable_area_pixel(xx, yy + TRYGAP);
        if (onarea<=0)
            onarea = get_walkable_area_pixel(xx, yy - TRYGAP);
        if (onarea < 0)
            onarea = 0;
    }

    return onarea;
}

int get_walkable_area_at_character (int charnum) {
    CharacterInfo *chin = &game.chars[charnum];
    return get_walkable_area_at_location(chin->x, chin->y);
}

ScriptWalkableArea *Walkarea_GetAtRoomXY(int x, int y)
{
    return &scrWalkarea[GetWalkableAreaAtRoom(x, y)]; // always a valid index
}

ScriptWalkableArea *Walkarea_GetAtScreenXY(int x, int y)
{
    int area = GetWalkableAreaAtScreen(x, y);
    if (area < 0 || area >= MAX_WALK_AREAS)
        return nullptr;
    return &scrWalkarea[area];
}

int Walkarea_GetID(ScriptWalkableArea *wa)
{
    return wa->id;
}

int Walkarea_GetEnabled(ScriptWalkableArea *wa)
{
    return play.walkable_areas_on[wa->id];
}

void Walkarea_SetEnabled(ScriptWalkableArea *wa, int enable)
{
    if (enable == play.walkable_areas_on[wa->id])
        return; // no change necessary
    if (enable)
        RestoreWalkableArea(wa->id);
    else
        RemoveWalkableArea(wa->id);
}

void Walkarea_SetScaling(ScriptWalkableArea *wa, int min, int max)
{
    SetAreaScaling(wa->id, min, max);
}

int Walkarea_GetScalingMin(ScriptWalkableArea *wa)
{
    return thisroom.WalkAreas[wa->id].ScalingFar;
}

int Walkarea_GetScalingMax(ScriptWalkableArea *wa)
{
    return thisroom.WalkAreas[wa->id].ScalingNear;
}

float Walkarea_GetFaceDirectionRatio(ScriptWalkableArea *wa)
{
    return thisroom.WalkAreas[wa->id].FaceDirectionRatio;
}

void Walkarea_SetFaceDirectionRatio(ScriptWalkableArea *wa, float ratio)
{
    thisroom.WalkAreas[wa->id].FaceDirectionRatio = ratio;
}

int Walkarea_GetProperty(ScriptWalkableArea *wa, const char *property)
{
    return get_int_property(thisroom.WalkAreas[wa->id].Properties, croom->waProps[wa->id], property);
}

const char* Walkarea_GetTextProperty(ScriptWalkableArea *wa, const char *property)
{
    return get_text_property_dynamic_string(thisroom.WalkAreas[wa->id].Properties, croom->waProps[wa->id], property);
}

bool Walkarea_SetProperty(ScriptWalkableArea *wa, const char *property, int value)
{
    return set_int_property(croom->waProps[wa->id], property, value);
}

bool Walkarea_SetTextProperty(ScriptWalkableArea *wa, const char *property, const char *value)
{
    return set_text_property(croom->waProps[wa->id], property, value);
}

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "debug/out.h"
#include "script/script_api.h"
#include "script/script_runtime.h"
#include "ac/dynobj/scriptstring.h"


extern RuntimeScriptValue Sc_GetDrawingSurfaceForWalkableArea(const RuntimeScriptValue *params, int32_t param_count);

RuntimeScriptValue Sc_Walkarea_GetAtRoomXY(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT2(ScriptWalkableArea, ccDynamicWalkarea, Walkarea_GetAtRoomXY);
}

RuntimeScriptValue Sc_Walkarea_GetAtScreenXY(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT2(ScriptWalkableArea, ccDynamicWalkarea, Walkarea_GetAtScreenXY);
}

RuntimeScriptValue Sc_Walkarea_GetDrawingSurface(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO(ScriptDrawingSurface, GetDrawingSurfaceForWalkableArea);
}

RuntimeScriptValue Sc_Walkarea_SetScaling(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT2(ScriptWalkableArea, Walkarea_SetScaling);
}

RuntimeScriptValue Sc_Walkarea_GetEnabled(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptWalkableArea, Walkarea_GetEnabled);
}

RuntimeScriptValue Sc_Walkarea_SetEnabled(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptWalkableArea, Walkarea_SetEnabled);
}

RuntimeScriptValue Sc_Walkarea_GetID(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptWalkableArea, Walkarea_GetID);
}

RuntimeScriptValue Sc_Walkarea_GetScalingMin(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptWalkableArea, Walkarea_GetScalingMin);
}

RuntimeScriptValue Sc_Walkarea_GetScalingMax(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptWalkableArea, Walkarea_GetScalingMax);
}

RuntimeScriptValue Sc_Walkarea_GetFaceDirectionRatio(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(ScriptWalkableArea, Walkarea_GetFaceDirectionRatio);
}

RuntimeScriptValue Sc_Walkarea_SetFaceDirectionRatio(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PFLOAT(ScriptWalkableArea, Walkarea_SetFaceDirectionRatio);
}

RuntimeScriptValue Sc_Walkarea_GetProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_POBJ(ScriptWalkableArea, Walkarea_GetProperty, const char);
}

RuntimeScriptValue Sc_Walkarea_GetTextProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_POBJ(ScriptWalkableArea, const char, myScriptStringImpl, Walkarea_GetTextProperty, const char);
}

RuntimeScriptValue Sc_Walkarea_SetProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL_POBJ_PINT(ScriptWalkableArea, Walkarea_SetProperty, const char);
}

RuntimeScriptValue Sc_Walkarea_SetTextProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL_POBJ2(ScriptWalkableArea, Walkarea_SetTextProperty, const char, const char);
}


void RegisterWalkareaAPI()
{
    ScFnRegister walkarea_api[] = {
        { "WalkableArea::GetAtRoomXY^2",       API_FN_PAIR(Walkarea_GetAtRoomXY) },
        { "WalkableArea::GetAtScreenXY^2",     API_FN_PAIR(Walkarea_GetAtScreenXY) },
        { "WalkableArea::GetDrawingSurface",   API_FN_PAIR(GetDrawingSurfaceForWalkableArea) },

        { "WalkableArea::SetScaling^2",        API_FN_PAIR(Walkarea_SetScaling) },
        { "WalkableArea::GetProperty^1",             API_FN_PAIR(Walkarea_GetProperty) },
        { "WalkableArea::GetTextProperty^1",         API_FN_PAIR(Walkarea_GetTextProperty) },
        { "WalkableArea::SetProperty^2",             API_FN_PAIR(Walkarea_SetProperty) },
        { "WalkableArea::SetTextProperty^2",         API_FN_PAIR(Walkarea_SetTextProperty) },
        { "WalkableArea::get_Enabled",         API_FN_PAIR(Walkarea_GetEnabled) },
        { "WalkableArea::set_Enabled",         API_FN_PAIR(Walkarea_SetEnabled) },
        { "WalkableArea::get_ID",              API_FN_PAIR(Walkarea_GetID) },
        { "WalkableArea::get_ScalingMin",      API_FN_PAIR(Walkarea_GetScalingMin) },
        { "WalkableArea::get_ScalingMax",      API_FN_PAIR(Walkarea_GetScalingMax) },
        { "WalkableArea::get_FaceDirectionRatio", API_FN_PAIR(Walkarea_GetFaceDirectionRatio) },
        { "WalkableArea::set_FaceDirectionRatio", API_FN_PAIR(Walkarea_SetFaceDirectionRatio) },
    };

    ccAddExternalFunctions(walkarea_api);
}
