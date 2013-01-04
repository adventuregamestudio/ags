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

#include "util/wgt2allg.h"
#include "ac/object.h"
#include "gfx/ali3d.h"
#include "ac/common.h"
#include "ac/gamesetupstruct.h"
#include "ac/draw.h"
#include "ac/character.h"
#include "ac/global_object.h"
#include "ac/global_translation.h"
#include "ac/objectcache.h"
#include "ac/path.h"
#include "ac/properties.h"
#include "ac/roomstatus.h"
#include "ac/roomstruct.h"
#include "ac/runtime_defines.h"
#include "ac/string.h"
#include "ac/walkablearea.h"
#include "debug/debug_log.h"
#include "main/game_run.h"
#include "ac/route_finder.h"
#include "gfx/graphicsdriver.h"
#include "gfx/bitmap.h"
#include "script/runtimescriptvalue.h"
#include "ac/dynobj/cc_object.h"

using AGS::Common::Bitmap;


extern ScriptObject scrObj[MAX_INIT_SPR];
extern RoomStatus*croom;
extern RoomObject*objs;
extern roomstruct thisroom;
extern ObjectCache objcache[MAX_INIT_SPR];
extern int final_scrn_wid,final_scrn_hit,final_col_dep;
extern MoveList *mls;
extern GameSetupStruct game;
extern Bitmap *walkable_areas_temp;
extern IGraphicsDriver *gfxDriver;
extern int offsetx,offsety;
extern CCObject ccDynamicObject;


int Object_IsCollidingWithObject(ScriptObject *objj, ScriptObject *obj2) {
    return AreObjectsColliding(objj->id, obj2->id);
}

ScriptObject *GetObjectAtLocation(int xx, int yy) {
    int hsnum = GetObjectAt(xx, yy);
    if (hsnum < 0)
        return NULL;
    return &scrObj[hsnum];
}

AGS_INLINE int is_valid_object(int obtest) {
    if ((obtest < 0) || (obtest >= croom->numobj)) return 0;
    return 1;
}

void Object_Tint(ScriptObject *objj, int red, int green, int blue, int saturation, int luminance) {
    SetObjectTint(objj->id, red, green, blue, saturation, luminance);
}

void Object_RemoveTint(ScriptObject *objj) {
    RemoveObjectTint(objj->id);
}

void Object_SetView(ScriptObject *objj, int view, int loop, int frame) {
    SetObjectFrame(objj->id, view, loop, frame);
}

void Object_SetTransparency(ScriptObject *objj, int trans) {
    SetObjectTransparency(objj->id, trans);
}

int Object_GetTransparency(ScriptObject *objj) {
    if (!is_valid_object(objj->id))
        quit("!Object.Transparent: invalid object number specified");

    if (objs[objj->id].transparent == 0)
        return 0;
    if (objs[objj->id].transparent == 255)
       return 100;

    return 100 - ((objs[objj->id].transparent * 10) / 25);
}

void Object_SetBaseline(ScriptObject *objj, int basel) {
    SetObjectBaseline(objj->id, basel);
}

int Object_GetBaseline(ScriptObject *objj) {
    return GetObjectBaseline(objj->id);
}

void Object_Animate(ScriptObject *objj, int loop, int delay, int repeat, int blocking, int direction) {
    if (direction == FORWARDS)
        direction = 0;
    else if (direction == BACKWARDS)
        direction = 1;
    else
        quit("!Object.Animate: Invalid DIRECTION parameter");

    if ((blocking == BLOCKING) || (blocking == 1))
        blocking = 1;
    else if ((blocking == IN_BACKGROUND) || (blocking == 0))
        blocking = 0;
    else
        quit("!Object.Animate: Invalid BLOCKING parameter");

    AnimateObjectEx(objj->id, loop, delay, repeat, direction, blocking);
}

void Object_StopAnimating(ScriptObject *objj) {
    if (!is_valid_object(objj->id))
        quit("!Object.StopAnimating: invalid object number");

    if (objs[objj->id].cycling) {
        objs[objj->id].cycling = 0;
        objs[objj->id].wait = 0;
    }
}

void Object_MergeIntoBackground(ScriptObject *objj) {
    MergeObject(objj->id);
}

void Object_StopMoving(ScriptObject *objj) {
    StopObjectMoving(objj->id);
}

void Object_SetVisible(ScriptObject *objj, int onoroff) {
    if (onoroff)
        ObjectOn(objj->id);
    else
        ObjectOff(objj->id);
}

int Object_GetView(ScriptObject *objj) {
    if (objs[objj->id].view < 0)
        return 0;
    return objs[objj->id].view + 1;
}

int Object_GetLoop(ScriptObject *objj) {
    if (objs[objj->id].view < 0)
        return 0;
    return objs[objj->id].loop;
}

int Object_GetFrame(ScriptObject *objj) {
    if (objs[objj->id].view < 0)
        return 0;
    return objs[objj->id].frame;
}

int Object_GetVisible(ScriptObject *objj) {
    return IsObjectOn(objj->id);
}

void Object_SetGraphic(ScriptObject *objj, int slott) {
    SetObjectGraphic(objj->id, slott);
}

int Object_GetGraphic(ScriptObject *objj) {
    return GetObjectGraphic(objj->id);
}

int GetObjectX (int objj) {
    if (!is_valid_object(objj)) quit("!GetObjectX: invalid object number");
    return objs[objj].x;
}

int Object_GetX(ScriptObject *objj) {
    return GetObjectX(objj->id);
}

int Object_GetY(ScriptObject *objj) {
    return GetObjectY(objj->id);
}

int Object_GetAnimating(ScriptObject *objj) {
    return IsObjectAnimating(objj->id);
}

int Object_GetMoving(ScriptObject *objj) {
    return IsObjectMoving(objj->id);
}

void Object_SetPosition(ScriptObject *objj, int xx, int yy) {
    SetObjectPosition(objj->id, xx, yy);
}

void Object_SetX(ScriptObject *objj, int xx) {
    SetObjectPosition(objj->id, xx, objs[objj->id].y);
}

void Object_SetY(ScriptObject *objj, int yy) {
    SetObjectPosition(objj->id, objs[objj->id].x, yy);
}

void Object_GetName(ScriptObject *objj, char *buffer) {
    GetObjectName(objj->id, buffer);
}

const char* Object_GetName_New(ScriptObject *objj) {
    if (!is_valid_object(objj->id))
        quit("!Object.Name: invalid object number");

    return CreateNewScriptString(get_translation(thisroom.objectnames[objj->id]));
}

void Object_Move(ScriptObject *objj, int x, int y, int speed, int blocking, int direct) {
    if ((direct == ANYWHERE) || (direct == 1))
        direct = 1;
    else if ((direct == WALKABLE_AREAS) || (direct == 0))
        direct = 0;
    else
        quit("Object.Move: invalid DIRECT parameter");

    move_object(objj->id, x, y, speed, direct);

    if ((blocking == BLOCKING) || (blocking == 1))
        do_main_cycle(UNTIL_SHORTIS0,(long)&objs[objj->id].moving);
    else if ((blocking != IN_BACKGROUND) && (blocking != 0))
        quit("Object.Move: invalid BLOCKING paramter");
}

void Object_SetClickable(ScriptObject *objj, int clik) {
    SetObjectClickable(objj->id, clik);
}

int Object_GetClickable(ScriptObject *objj) {
    if (!is_valid_object(objj->id))
        quit("!Object.Clickable: Invalid object specified");

    if (objs[objj->id].flags & OBJF_NOINTERACT)
        return 0;
    return 1;
}

void Object_SetIgnoreScaling(ScriptObject *objj, int newval) {
    if (!is_valid_object(objj->id))
        quit("!Object.IgnoreScaling: Invalid object specified");

    objs[objj->id].flags &= ~OBJF_USEROOMSCALING;
    if (!newval)
        objs[objj->id].flags |= OBJF_USEROOMSCALING;

    // clear the cache
    objcache[objj->id].ywas = -9999;
}

int Object_GetIgnoreScaling(ScriptObject *objj) {
    if (!is_valid_object(objj->id))
        quit("!Object.IgnoreScaling: Invalid object specified");

    if (objs[objj->id].flags & OBJF_USEROOMSCALING)
        return 0;
    return 1;
}

void Object_SetSolid(ScriptObject *objj, int solid) {
    objs[objj->id].flags &= ~OBJF_SOLID;
    if (solid)
      objs[objj->id].flags |= OBJF_SOLID;
}

int Object_GetSolid(ScriptObject *objj) {
    if (objs[objj->id].flags & OBJF_SOLID)
        return 1;
    return 0;
}

void Object_SetBlockingWidth(ScriptObject *objj, int bwid) {
    objs[objj->id].blocking_width = bwid;
}

int Object_GetBlockingWidth(ScriptObject *objj) {
    return objs[objj->id].blocking_width;
}

void Object_SetBlockingHeight(ScriptObject *objj, int bhit) {
    objs[objj->id].blocking_height = bhit;
}

int Object_GetBlockingHeight(ScriptObject *objj) {
    return objs[objj->id].blocking_height;
}

int Object_GetID(ScriptObject *objj) {
    return objj->id;
}

void Object_SetIgnoreWalkbehinds(ScriptObject *chaa, int clik) {
    SetObjectIgnoreWalkbehinds(chaa->id, clik);
}

int Object_GetIgnoreWalkbehinds(ScriptObject *chaa) {
    if (!is_valid_object(chaa->id))
        quit("!Object.IgnoreWalkbehinds: Invalid object specified");

    if (objs[chaa->id].flags & OBJF_NOWALKBEHINDS)
        return 1;
    return 0;
}

void move_object(int objj,int tox,int toy,int spee,int ignwal) {

    if (!is_valid_object(objj))
        quit("!MoveObject: invalid object number");

    // AGS <= 2.61 uses MoveObject with spp=-1 internally instead of SetObjectPosition
    if ((loaded_game_file_version <= kGameVersion_261) && (spee == -1))
    {
        objs[objj].x = tox;
        objs[objj].y = toy;
        return;
    }

    DEBUG_CONSOLE("Object %d start move to %d,%d", objj, tox, toy);

    int objX = convert_to_low_res(objs[objj].x);
    int objY = convert_to_low_res(objs[objj].y);
    tox = convert_to_low_res(tox);
    toy = convert_to_low_res(toy);

    set_route_move_speed(spee, spee);
    set_color_depth(8);
    int mslot=find_route(objX, objY, tox, toy, prepare_walkable_areas(-1), objj+1, 1, ignwal);
    set_color_depth(final_col_dep);
    if (mslot>0) {
        objs[objj].moving = mslot;
        mls[mslot].direct = ignwal;

        if ((game.options[OPT_NATIVECOORDINATES] != 0) &&
            (game.default_resolution > 2))
        {
            convert_move_path_to_high_res(&mls[mslot]);
        }
    }
}

void Object_RunInteraction(ScriptObject *objj, int mode) {
    RunObjectInteraction(objj->id, mode);
}

int Object_GetProperty (ScriptObject *objj, const char *property) {
    return GetObjectProperty(objj->id, property);
}

void Object_GetPropertyText(ScriptObject *objj, const char *property, char *bufer) {
    GetObjectPropertyText(objj->id, property, bufer);
}
const char* Object_GetTextProperty(ScriptObject *objj, const char *property) {
    return get_text_property_dynamic_string(&thisroom.objProps[objj->id], property);
}

void get_object_blocking_rect(int objid, int *x1, int *y1, int *width, int *y2) {
    RoomObject *tehobj = &objs[objid];
    int cwidth, fromx;

    if (tehobj->blocking_width < 1)
        cwidth = divide_down_coordinate(tehobj->last_width) - 4;
    else
        cwidth = tehobj->blocking_width;

    fromx = tehobj->x + (divide_down_coordinate(tehobj->last_width) / 2) - cwidth / 2;
    if (fromx < 0) {
        cwidth += fromx;
        fromx = 0;
    }
    if (fromx + cwidth >= convert_back_to_high_res(walkable_areas_temp->GetWidth()))
        cwidth = convert_back_to_high_res(walkable_areas_temp->GetWidth()) - fromx;

    if (x1)
        *x1 = fromx;
    if (width)
        *width = cwidth;
    if (y1) {
        if (tehobj->blocking_height > 0)
            *y1 = tehobj->y - tehobj->blocking_height / 2;
        else
            *y1 = tehobj->y - 2;
    }
    if (y2) {
        if (tehobj->blocking_height > 0)
            *y2 = tehobj->y + tehobj->blocking_height / 2;
        else
            *y2 = tehobj->y + 3;
    }
}

int isposinbox(int mmx,int mmy,int lf,int tp,int rt,int bt) {
    if ((mmx>=lf) & (mmx<=rt) & (mmy>=tp) & (mmy<=bt)) return TRUE;
    else return FALSE;
}

// xx,yy is the position in room co-ordinates that we are checking
// arx,ary is the sprite x/y co-ordinates
int is_pos_in_sprite(int xx,int yy,int arx,int ary, Bitmap *sprit, int spww,int sphh, int flipped) {
    if (spww==0) spww = divide_down_coordinate(sprit->GetWidth()) - 1;
    if (sphh==0) sphh = divide_down_coordinate(sprit->GetHeight()) - 1;

    if (isposinbox(xx,yy,arx,ary,arx+spww,ary+sphh)==FALSE)
        return FALSE;

    if (game.options[OPT_PIXPERFECT]) 
    {
        // if it's transparent, or off the edge of the sprite, ignore
        int xpos = multiply_up_coordinate(xx - arx);
        int ypos = multiply_up_coordinate(yy - ary);

        if (gfxDriver->HasAcceleratedStretchAndFlip())
        {
            // hardware acceleration, so the sprite in memory will not have
            // been stretched, it will be original size. Thus, adjust our
            // calculations to compensate
            multiply_up_coordinates(&spww, &sphh);

            if (spww != sprit->GetWidth())
                xpos = (xpos * sprit->GetWidth()) / spww;
            if (sphh != sprit->GetHeight())
                ypos = (ypos * sprit->GetHeight()) / sphh;
        }

        if (flipped)
            xpos = (sprit->GetWidth() - 1) - xpos;

        int gpcol = my_getpixel(sprit, xpos, ypos);

        if ((gpcol == sprit->GetMaskColor()) || (gpcol == -1))
            return FALSE;
    }
    return TRUE;
}

// X and Y co-ordinates must be in 320x200 format
int check_click_on_object(int xx,int yy,int mood) {
    int aa = GetObjectAt(xx - divide_down_coordinate(offsetx), yy - divide_down_coordinate(offsety));
    if (aa < 0) return 0;
    RunObjectInteraction(aa, mood);
    return 1;
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

extern ScriptString myScriptStringImpl;

// void (ScriptObject *objj, int loop, int delay, int repeat, int blocking, int direction)
RuntimeScriptValue Sc_Object_Animate(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT5(ScriptObject, Object_Animate);
}

// int (ScriptObject *objj, ScriptObject *obj2)
RuntimeScriptValue Sc_Object_IsCollidingWithObject(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_POBJ(ScriptObject, Object_IsCollidingWithObject, ScriptObject);
}

// void (ScriptObject *objj, char *buffer)
RuntimeScriptValue Sc_Object_GetName(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(ScriptObject, Object_GetName, char);
}

// int (ScriptObject *objj, const char *property)
RuntimeScriptValue Sc_Object_GetProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_POBJ(ScriptObject, Object_GetProperty, const char);
}

// void (ScriptObject *objj, const char *property, char *bufer)
RuntimeScriptValue Sc_Object_GetPropertyText(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ2(ScriptObject, Object_GetPropertyText, const char, char);
}

//const char* (ScriptObject *objj, const char *property)
RuntimeScriptValue Sc_Object_GetTextProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_POBJ(ScriptObject, const char, myScriptStringImpl, Object_GetTextProperty, const char);
}

// void (ScriptObject *objj)
RuntimeScriptValue Sc_Object_MergeIntoBackground(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptObject, Object_MergeIntoBackground);
}

// void (ScriptObject *objj, int x, int y, int speed, int blocking, int direct)
RuntimeScriptValue Sc_Object_Move(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT5(ScriptObject, Object_Move);
}

// void (ScriptObject *objj)
RuntimeScriptValue Sc_Object_RemoveTint(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptObject, Object_RemoveTint);
}

// void (ScriptObject *objj, int mode)
RuntimeScriptValue Sc_Object_RunInteraction(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptObject, Object_RunInteraction);
}

// void (ScriptObject *objj, int xx, int yy)
RuntimeScriptValue Sc_Object_SetPosition(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT2(ScriptObject, Object_SetPosition);
}

// void (ScriptObject *objj, int view, int loop, int frame)
RuntimeScriptValue Sc_Object_SetView(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT3(ScriptObject, Object_SetView);
}

// void (ScriptObject *objj)
RuntimeScriptValue Sc_Object_StopAnimating(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptObject, Object_StopAnimating);
}

// void (ScriptObject *objj)
RuntimeScriptValue Sc_Object_StopMoving(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptObject, Object_StopMoving);
}

// void (ScriptObject *objj, int red, int green, int blue, int saturation, int luminance)
RuntimeScriptValue Sc_Object_Tint(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT5(ScriptObject, Object_Tint);
}

// ScriptObject *(int xx, int yy)
RuntimeScriptValue Sc_GetObjectAtLocation(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT2(ScriptObject, ccDynamicObject, GetObjectAtLocation);
}

// int (ScriptObject *objj)
RuntimeScriptValue Sc_Object_GetAnimating(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetAnimating);
}

// int (ScriptObject *objj)
RuntimeScriptValue Sc_Object_GetBaseline(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetBaseline);
}

// void (ScriptObject *objj, int basel)
RuntimeScriptValue Sc_Object_SetBaseline(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptObject, Object_SetBaseline);
}

// int (ScriptObject *objj)
RuntimeScriptValue Sc_Object_GetBlockingHeight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetBlockingHeight);
}

// void (ScriptObject *objj, int bhit)
RuntimeScriptValue Sc_Object_SetBlockingHeight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptObject, Object_SetBlockingHeight);
}

// int (ScriptObject *objj)
RuntimeScriptValue Sc_Object_GetBlockingWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetBlockingWidth);
}

// void (ScriptObject *objj, int bwid)
RuntimeScriptValue Sc_Object_SetBlockingWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptObject, Object_SetBlockingWidth);
}

// int (ScriptObject *objj)
RuntimeScriptValue Sc_Object_GetClickable(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetClickable);
}

// void (ScriptObject *objj, int clik)
RuntimeScriptValue Sc_Object_SetClickable(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptObject, Object_SetClickable);
}

// int (ScriptObject *objj)
RuntimeScriptValue Sc_Object_GetFrame(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetFrame);
}

// int (ScriptObject *objj)
RuntimeScriptValue Sc_Object_GetGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetGraphic);
}

// void (ScriptObject *objj, int slott)
RuntimeScriptValue Sc_Object_SetGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptObject, Object_SetGraphic);
}

// int (ScriptObject *objj)
RuntimeScriptValue Sc_Object_GetID(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetID);
}

// int (ScriptObject *objj)
RuntimeScriptValue Sc_Object_GetIgnoreScaling(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetIgnoreScaling);
}

// void (ScriptObject *objj, int newval)
RuntimeScriptValue Sc_Object_SetIgnoreScaling(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptObject, Object_SetIgnoreScaling);
}

// int (ScriptObject *chaa)
RuntimeScriptValue Sc_Object_GetIgnoreWalkbehinds(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetIgnoreWalkbehinds);
}

// void (ScriptObject *chaa, int clik)
RuntimeScriptValue Sc_Object_SetIgnoreWalkbehinds(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptObject, Object_SetIgnoreWalkbehinds);
}

// int (ScriptObject *objj)
RuntimeScriptValue Sc_Object_GetLoop(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetLoop);
}

// int (ScriptObject *objj)
RuntimeScriptValue Sc_Object_GetMoving(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetMoving);
}

// const char* (ScriptObject *objj)
RuntimeScriptValue Sc_Object_GetName_New(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptObject, const char, myScriptStringImpl, Object_GetName_New);
}

// int (ScriptObject *objj)
RuntimeScriptValue Sc_Object_GetSolid(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetSolid);
}

// void (ScriptObject *objj, int solid)
RuntimeScriptValue Sc_Object_SetSolid(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptObject, Object_SetSolid);
}

// int (ScriptObject *objj)
RuntimeScriptValue Sc_Object_GetTransparency(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetTransparency);
}

// void (ScriptObject *objj, int trans)
RuntimeScriptValue Sc_Object_SetTransparency(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptObject, Object_SetTransparency);
}

// int (ScriptObject *objj)
RuntimeScriptValue Sc_Object_GetView(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetView);
}

// int (ScriptObject *objj)
RuntimeScriptValue Sc_Object_GetVisible(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetVisible);
}

// void (ScriptObject *objj, int onoroff)
RuntimeScriptValue Sc_Object_SetVisible(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptObject, Object_SetVisible);
}

// int (ScriptObject *objj)
RuntimeScriptValue Sc_Object_GetX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetX);
}

// void (ScriptObject *objj, int xx)
RuntimeScriptValue Sc_Object_SetX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptObject, Object_SetX);
}

// int (ScriptObject *objj)
RuntimeScriptValue Sc_Object_GetY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetY);
}

// void (ScriptObject *objj, int yy)
RuntimeScriptValue Sc_Object_SetY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptObject, Object_SetY);
}



void RegisterObjectAPI()
{
    ccAddExternalObjectFunction("Object::Animate^5",                Sc_Object_Animate);
    ccAddExternalObjectFunction("Object::IsCollidingWithObject^1",  Sc_Object_IsCollidingWithObject);
    ccAddExternalObjectFunction("Object::GetName^1",                Sc_Object_GetName);
    ccAddExternalObjectFunction("Object::GetProperty^1",            Sc_Object_GetProperty);
    ccAddExternalObjectFunction("Object::GetPropertyText^2",        Sc_Object_GetPropertyText);
    ccAddExternalObjectFunction("Object::GetTextProperty^1",        Sc_Object_GetTextProperty);
    ccAddExternalObjectFunction("Object::MergeIntoBackground^0",    Sc_Object_MergeIntoBackground);
    ccAddExternalObjectFunction("Object::Move^5",                   Sc_Object_Move);
    ccAddExternalObjectFunction("Object::RemoveTint^0",             Sc_Object_RemoveTint);
    ccAddExternalObjectFunction("Object::RunInteraction^1",         Sc_Object_RunInteraction);
    ccAddExternalObjectFunction("Object::SetPosition^2",            Sc_Object_SetPosition);
    ccAddExternalObjectFunction("Object::SetView^3",                Sc_Object_SetView);
    ccAddExternalObjectFunction("Object::StopAnimating^0",          Sc_Object_StopAnimating);
    ccAddExternalObjectFunction("Object::StopMoving^0",             Sc_Object_StopMoving);
    ccAddExternalObjectFunction("Object::Tint^5",                   Sc_Object_Tint);

    // static
    ccAddExternalStaticFunction("Object::GetAtScreenXY^2",          Sc_GetObjectAtLocation);

    ccAddExternalObjectFunction("Object::get_Animating",            Sc_Object_GetAnimating);
    ccAddExternalObjectFunction("Object::get_Baseline",             Sc_Object_GetBaseline);
    ccAddExternalObjectFunction("Object::set_Baseline",             Sc_Object_SetBaseline);
    ccAddExternalObjectFunction("Object::get_BlockingHeight",       Sc_Object_GetBlockingHeight);
    ccAddExternalObjectFunction("Object::set_BlockingHeight",       Sc_Object_SetBlockingHeight);
    ccAddExternalObjectFunction("Object::get_BlockingWidth",        Sc_Object_GetBlockingWidth);
    ccAddExternalObjectFunction("Object::set_BlockingWidth",        Sc_Object_SetBlockingWidth);
    ccAddExternalObjectFunction("Object::get_Clickable",            Sc_Object_GetClickable);
    ccAddExternalObjectFunction("Object::set_Clickable",            Sc_Object_SetClickable);
    ccAddExternalObjectFunction("Object::get_Frame",                Sc_Object_GetFrame);
    ccAddExternalObjectFunction("Object::get_Graphic",              Sc_Object_GetGraphic);
    ccAddExternalObjectFunction("Object::set_Graphic",              Sc_Object_SetGraphic);
    ccAddExternalObjectFunction("Object::get_ID",                   Sc_Object_GetID);
    ccAddExternalObjectFunction("Object::get_IgnoreScaling",        Sc_Object_GetIgnoreScaling);
    ccAddExternalObjectFunction("Object::set_IgnoreScaling",        Sc_Object_SetIgnoreScaling);
    ccAddExternalObjectFunction("Object::get_IgnoreWalkbehinds",    Sc_Object_GetIgnoreWalkbehinds);
    ccAddExternalObjectFunction("Object::set_IgnoreWalkbehinds",    Sc_Object_SetIgnoreWalkbehinds);
    ccAddExternalObjectFunction("Object::get_Loop",                 Sc_Object_GetLoop);
    ccAddExternalObjectFunction("Object::get_Moving",               Sc_Object_GetMoving);
    ccAddExternalObjectFunction("Object::get_Name",                 Sc_Object_GetName_New);
    ccAddExternalObjectFunction("Object::get_Solid",                Sc_Object_GetSolid);
    ccAddExternalObjectFunction("Object::set_Solid",                Sc_Object_SetSolid);
    ccAddExternalObjectFunction("Object::get_Transparency",         Sc_Object_GetTransparency);
    ccAddExternalObjectFunction("Object::set_Transparency",         Sc_Object_SetTransparency);
    ccAddExternalObjectFunction("Object::get_View",                 Sc_Object_GetView);
    ccAddExternalObjectFunction("Object::get_Visible",              Sc_Object_GetVisible);
    ccAddExternalObjectFunction("Object::set_Visible",              Sc_Object_SetVisible);
    ccAddExternalObjectFunction("Object::get_X",                    Sc_Object_GetX);
    ccAddExternalObjectFunction("Object::set_X",                    Sc_Object_SetX);
    ccAddExternalObjectFunction("Object::get_Y",                    Sc_Object_GetY);
    ccAddExternalObjectFunction("Object::set_Y",                    Sc_Object_SetY);

    /* ----------------------- Registering unsafe exports for plugins -----------------------*/

    ccAddExternalFunctionForPlugin("Object::Animate^5",                Object_Animate);
    ccAddExternalFunctionForPlugin("Object::IsCollidingWithObject^1",  Object_IsCollidingWithObject);
    ccAddExternalFunctionForPlugin("Object::GetName^1",                Object_GetName);
    ccAddExternalFunctionForPlugin("Object::GetProperty^1",            Object_GetProperty);
    ccAddExternalFunctionForPlugin("Object::GetPropertyText^2",        Object_GetPropertyText);
    ccAddExternalFunctionForPlugin("Object::GetTextProperty^1",        Object_GetTextProperty);
    ccAddExternalFunctionForPlugin("Object::MergeIntoBackground^0",    Object_MergeIntoBackground);
    ccAddExternalFunctionForPlugin("Object::Move^5",                   Object_Move);
    ccAddExternalFunctionForPlugin("Object::RemoveTint^0",             Object_RemoveTint);
    ccAddExternalFunctionForPlugin("Object::RunInteraction^1",         Object_RunInteraction);
    ccAddExternalFunctionForPlugin("Object::SetPosition^2",            Object_SetPosition);
    ccAddExternalFunctionForPlugin("Object::SetView^3",                Object_SetView);
    ccAddExternalFunctionForPlugin("Object::StopAnimating^0",          Object_StopAnimating);
    ccAddExternalFunctionForPlugin("Object::StopMoving^0",             Object_StopMoving);
    ccAddExternalFunctionForPlugin("Object::Tint^5",                   Object_Tint);
    ccAddExternalFunctionForPlugin("Object::GetAtScreenXY^2",          GetObjectAtLocation);
    ccAddExternalFunctionForPlugin("Object::get_Animating",            Object_GetAnimating);
    ccAddExternalFunctionForPlugin("Object::get_Baseline",             Object_GetBaseline);
    ccAddExternalFunctionForPlugin("Object::set_Baseline",             Object_SetBaseline);
    ccAddExternalFunctionForPlugin("Object::get_BlockingHeight",       Object_GetBlockingHeight);
    ccAddExternalFunctionForPlugin("Object::set_BlockingHeight",       Object_SetBlockingHeight);
    ccAddExternalFunctionForPlugin("Object::get_BlockingWidth",        Object_GetBlockingWidth);
    ccAddExternalFunctionForPlugin("Object::set_BlockingWidth",        Object_SetBlockingWidth);
    ccAddExternalFunctionForPlugin("Object::get_Clickable",            Object_GetClickable);
    ccAddExternalFunctionForPlugin("Object::set_Clickable",            Object_SetClickable);
    ccAddExternalFunctionForPlugin("Object::get_Frame",                Object_GetFrame);
    ccAddExternalFunctionForPlugin("Object::get_Graphic",              Object_GetGraphic);
    ccAddExternalFunctionForPlugin("Object::set_Graphic",              Object_SetGraphic);
    ccAddExternalFunctionForPlugin("Object::get_ID",                   Object_GetID);
    ccAddExternalFunctionForPlugin("Object::get_IgnoreScaling",        Object_GetIgnoreScaling);
    ccAddExternalFunctionForPlugin("Object::set_IgnoreScaling",        Object_SetIgnoreScaling);
    ccAddExternalFunctionForPlugin("Object::get_IgnoreWalkbehinds",    Object_GetIgnoreWalkbehinds);
    ccAddExternalFunctionForPlugin("Object::set_IgnoreWalkbehinds",    Object_SetIgnoreWalkbehinds);
    ccAddExternalFunctionForPlugin("Object::get_Loop",                 Object_GetLoop);
    ccAddExternalFunctionForPlugin("Object::get_Moving",               Object_GetMoving);
    ccAddExternalFunctionForPlugin("Object::get_Name",                 Object_GetName_New);
    ccAddExternalFunctionForPlugin("Object::get_Solid",                Object_GetSolid);
    ccAddExternalFunctionForPlugin("Object::set_Solid",                Object_SetSolid);
    ccAddExternalFunctionForPlugin("Object::get_Transparency",         Object_GetTransparency);
    ccAddExternalFunctionForPlugin("Object::set_Transparency",         Object_SetTransparency);
    ccAddExternalFunctionForPlugin("Object::get_View",                 Object_GetView);
    ccAddExternalFunctionForPlugin("Object::get_Visible",              Object_GetVisible);
    ccAddExternalFunctionForPlugin("Object::set_Visible",              Object_SetVisible);
    ccAddExternalFunctionForPlugin("Object::get_X",                    Object_GetX);
    ccAddExternalFunctionForPlugin("Object::set_X",                    Object_SetX);
    ccAddExternalFunctionForPlugin("Object::get_Y",                    Object_GetY);
    ccAddExternalFunctionForPlugin("Object::set_Y",                    Object_SetY);
}
