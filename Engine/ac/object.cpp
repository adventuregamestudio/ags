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
#include "ac/object.h"
#include "ac/common.h"
#include "ac/gamesetupstruct.h"
#include "ac/draw.h"
#include "ac/character.h"
#include "ac/game.h"
#include "ac/gamestate.h"
#include "ac/global_object.h"
#include "ac/global_translation.h"
#include "ac/gui.h"
#include "ac/movelist.h"
#include "ac/properties.h"
#include "ac/room.h"
#include "ac/roomstatus.h"
#include "ac/runtime_defines.h"
#include "ac/string.h"
#include "ac/system.h"
#include "ac/view.h"
#include "ac/viewframe.h"
#include "ac/walkablearea.h"
#include "ac/dynobj/cc_object.h"
#include "debug/debug_log.h"
#include "main/game_run.h"
#include "ac/route_finder.h"
#include "gfx/graphicsdriver.h"
#include "gfx/bitmap.h"
#include "gfx/gfx_def.h"
#include "script/runtimescriptvalue.h"

using namespace AGS::Common;
using namespace AGS::Engine;


extern ScriptObject scrObj[MAX_ROOM_OBJECTS];
extern RoomStatus*croom;
extern RoomObject*objs;
extern std::vector<ViewStruct> views;
extern RoomStruct thisroom;
extern GameSetupStruct game;
extern Bitmap *walkable_areas_temp;
extern IGraphicsDriver *gfxDriver;
extern CCObject ccDynamicObject;


bool is_valid_object(int obj_id)
{
    return (obj_id >= 0) && (static_cast<uint32_t>(obj_id) < croom->numobj);
}

bool AssertObject(const char *apiname, int obj_id)
{
    if ((obj_id >= 0) && (static_cast<uint32_t>(obj_id) < croom->numobj))
        return true;
    debug_script_warn("%s: invalid object id %d (range is 0..%d)", apiname, obj_id, croom->numobj - 1);
    return false;
}

int Object_IsCollidingWithObject(ScriptObject *objj, ScriptObject *obj2) {
    return AreObjectsColliding(objj->id, obj2->id);
}

ScriptObject *GetObjectAtScreen(int xx, int yy) {
    int hsnum = GetObjectIDAtScreen(xx, yy);
    if (hsnum < 0)
        return nullptr;
    return &scrObj[hsnum];
}

ScriptObject *GetObjectAtRoom(int x, int y)
{
    int hsnum = GetObjectIDAtRoom(x, y);
    if (hsnum < 0)
        return nullptr;
    return &scrObj[hsnum];
}

void Object_Tint(ScriptObject *objj, int red, int green, int blue, int saturation, int luminance) {
    SetObjectTint(objj->id, red, green, blue, saturation, luminance);
}

void Object_RemoveTint(ScriptObject *objj) {
    RemoveObjectTint(objj->id);
}

void Object_SetView(ScriptObject *objj, int view, int loop, int frame) {
    SetObjectFrameSimple(objj->id, view, loop, frame);
}

void Object_SetTransparency(ScriptObject *objj, int trans) {
    SetObjectTransparency(objj->id, trans);
}

int Object_GetTransparency(ScriptObject *objj) {
    if (!is_valid_object(objj->id))
        quit("!Object.Transparent: invalid object number specified");

    return GfxDef::LegacyTrans255ToTrans100(objs[objj->id].transparent);
}

int Object_GetAnimationVolume(ScriptObject *objj) {
    return objs[objj->id].anim_volume;
}

void Object_SetAnimationVolume(ScriptObject *objj, int newval) {

    objs[objj->id].anim_volume = Math::Clamp(newval, 0, 100);
}

void Object_SetBaseline(ScriptObject *objj, int basel) {
    SetObjectBaseline(objj->id, basel);
}

int Object_GetBaseline(ScriptObject *objj) {
    return GetObjectBaseline(objj->id);
}

void Object_Animate(ScriptObject *objj, int loop, int delay, int repeat,
    int blocking, int direction, int sframe, int volume) {
    AnimateObjectImpl(objj->id, loop, delay, repeat, direction, blocking, sframe, volume);
}

void Object_Animate5(ScriptObject *objj, int loop, int delay, int repeat, int blocking, int direction) {
    Object_Animate(objj, loop, delay, repeat, blocking, direction, 0 /* frame */, 100 /* full volume */);
}

void Object_Animate6(ScriptObject *objj, int loop, int delay, int repeat, int blocking, int direction, int sframe) {
    Object_Animate(objj, loop, delay, repeat, blocking, direction, sframe, 100 /* full volume */);
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
    if (objs[objj->id].view == RoomObject::NoView)
        return 0;
    return objs[objj->id].view + 1;
}

int Object_GetLoop(ScriptObject *objj) {
    if (objs[objj->id].view == RoomObject::NoView)
        return 0;
    return objs[objj->id].loop;
}

int Object_GetFrame(ScriptObject *objj) {
    if (objs[objj->id].view == RoomObject::NoView)
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

bool Object_HasExplicitLight(ScriptObject *obj)
{
    return objs[obj->id].has_explicit_light();
}

bool Object_HasExplicitTint(ScriptObject *obj)
{
    return objs[obj->id].has_explicit_tint();
}

int Object_GetLightLevel(ScriptObject *obj)
{
    return objs[obj->id].has_explicit_light() ? objs[obj->id].tint_light : 0;
}

void Object_SetLightLevel(ScriptObject *objj, int light_level)
{
    int obj = objj->id;
    if (!is_valid_object(obj))
        quit("!SetObjectTint: invalid object number specified");

    objs[obj].tint_light = light_level;
    objs[obj].flags &= ~OBJF_HASTINT;
    objs[obj].flags |= OBJF_HASLIGHT;
}

int Object_GetTintRed(ScriptObject *obj)
{
    return objs[obj->id].has_explicit_tint() ? objs[obj->id].tint_r : 0;
}

int Object_GetTintGreen(ScriptObject *obj)
{
    return objs[obj->id].has_explicit_tint() ? objs[obj->id].tint_g : 0;
}

int Object_GetTintBlue(ScriptObject *obj)
{
    return objs[obj->id].has_explicit_tint() ? objs[obj->id].tint_b : 0;
}

int Object_GetTintSaturation(ScriptObject *obj)
{
     return objs[obj->id].has_explicit_tint() ? objs[obj->id].tint_level : 0;
}

int Object_GetTintLuminance(ScriptObject *obj)
{
    return objs[obj->id].has_explicit_tint() ? ((objs[obj->id].tint_light * 10) / 25) : 0;
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

    return CreateNewScriptString(get_translation(croom->obj[objj->id].name.GetCStr()));
}

void Object_SetName(ScriptObject *objj, const char *newName) {
    if (!is_valid_object(objj->id))
        quit("!Object.Name: invalid object number");
    croom->obj[objj->id].name = newName;
    GUIE::MarkSpecialLabelsForUpdate(kLabelMacro_Overhotspot);
}

bool Object_IsInteractionAvailable(ScriptObject *oobj, int mood) {

    play.check_interaction_only = 1;
    RunObjectInteraction(oobj->id, mood);
    int ciwas = play.check_interaction_only;
    play.check_interaction_only = 0;
    return (ciwas == 2);
}

void Object_Move(ScriptObject *objj, int x, int y, int speed, int blocking, int ignwal)
{
    ValidateMoveParams("Object.Move", blocking, ignwal);

    move_object(objj->id, x, y, speed, ignwal);

    if (blocking)
        GameLoopUntilNotMoving(&objs[objj->id].moving);
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

int Object_GetDestinationX(ScriptObject *objj)
{
    if (!is_valid_object(objj->id))
        quit("!Object.DestionationX: Invalid object specified");

    if (objs[objj->id].moving)
    {
        MoveList *cmls = &mls[objs[objj->id].moving];
        return cmls->pos.back().X;
    }
    return objs[objj->id].x;
}

int Object_GetDestinationY(ScriptObject *objj)
{
    if (!is_valid_object(objj->id))
        quit("!Object.DestionationX: Invalid object specified");

    if (objs[objj->id].moving)
    {
        MoveList *cmls = &mls[objs[objj->id].moving];
        return cmls->pos.back().Y;
    }
    return objs[objj->id].y;
}

void Object_SetManualScaling(ScriptObject *objj, bool on)
{
    if (on) objs[objj->id].flags &= ~OBJF_USEROOMSCALING;
    else objs[objj->id].flags |= OBJF_USEROOMSCALING;
    // clear the cache
    mark_object_changed(objj->id);
}

void Object_SetIgnoreScaling(ScriptObject *objj, int newval) {
    if (!is_valid_object(objj->id))
        quit("!Object.IgnoreScaling: Invalid object specified");
    if (newval)
        objs[objj->id].zoom = 100; // compatibility, for before manual scaling existed
    Object_SetManualScaling(objj, newval != 0);
}

int Object_GetIgnoreScaling(ScriptObject *objj) {
    if (!is_valid_object(objj->id))
        quit("!Object.IgnoreScaling: Invalid object specified");

    if (objs[objj->id].flags & OBJF_USEROOMSCALING)
        return 0;
    return 1;
}

int Object_GetScaling(ScriptObject *objj) {
    return objs[objj->id].zoom;
}

void Object_SetScaling(ScriptObject *objj, int zoomlevel) {
    if ((objs[objj->id].flags & OBJF_USEROOMSCALING) != 0)
    {
        debug_script_warn("Object.Scaling: cannot set property unless ManualScaling is enabled");
        return;
    }
    int zoom_fixed = Math::Clamp(zoomlevel, 1, (int)(INT16_MAX)); // RoomObject.zoom is int16
    if (zoomlevel != zoom_fixed)
        debug_script_warn("Object.Scaling: scaling level must be between 1 and %d%%, asked for: %d",
                        (int)(INT16_MAX), zoomlevel);
    objs[objj->id].zoom = zoom_fixed;
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

const char *Object_GetScriptName(ScriptObject *objj)
{
    return CreateNewScriptString(thisroom.Objects[objj->id].ScriptName);
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

void move_object(int objj, int tox, int toy, int speed, int ignwal) {

    if (!is_valid_object(objj))
        quit("!MoveObject: invalid object number");

    auto &obj = objs[objj];
    // AGS <= 2.61 uses MoveObject with spp=-1 internally instead of SetObjectPosition
    if ((loaded_game_file_version <= kGameVersion_261) && (speed == -1))
    {
        obj.x = tox;
        obj.y = toy;
        return;
    }

    debug_script_log("Object %d start move to %d,%d", objj, tox, toy);

    // NOTE: for old games we assume the input coordinates are in the "data" coordinate system
    const int src_x = data_to_game_coord(objs[objj].x);
    const int src_y = data_to_game_coord(objs[objj].y);
    const int dst_x = data_to_game_coord(tox);
    const int dst_y = data_to_game_coord(toy);

    const int mslot = objj + 1;
    MaskRouteFinder *pathfind = get_room_pathfinder();
    pathfind->SetWalkableArea(prepare_walkable_areas(-1), thisroom.MaskResolution);
    if (Pathfinding::FindRoute(mls[mslot], pathfind, src_x, src_y, dst_x, dst_y,
        speed, speed, false, ignwal != 0))
    {
        objs[objj].moving = mslot;
        convert_move_path_to_data_resolution(mls[mslot]);
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

const char* Object_GetTextProperty(ScriptObject *objj, const char *property)
{
    if (!AssertObject("Object.GetTextProperty", objj->id))
        return nullptr;
    return get_text_property_dynamic_string(thisroom.Objects[objj->id].Properties, croom->objProps[objj->id], property);
}

bool Object_SetProperty(ScriptObject *objj, const char *property, int value)
{
    if (!AssertObject("Object.SetProperty", objj->id))
        return false;
    return set_int_property(croom->objProps[objj->id], property, value);
}

bool Object_SetTextProperty(ScriptObject *objj, const char *property, const char *value)
{
    if (!AssertObject("Object.SetTextProperty", objj->id))
        return false;
    return set_text_property(croom->objProps[objj->id], property, value);
}

void update_object_scale(int &res_zoom, int &res_width, int &res_height,
    int objx, int objy, int sprnum, int own_zoom, bool use_region_scaling)
{
    int zoom = own_zoom;
    if (use_region_scaling)
    {
        // Only apply area zoom if we're on a a valid area:
        // * either area is > 0, or
        // * area 0 has valid scaling property (<> 0)
        int onarea = get_walkable_area_at_location(objx, objy);
        if ((onarea > 0) || (thisroom.WalkAreas[0].ScalingFar != 0))
        {
            zoom = get_area_scaling(onarea, objx, objy);
        }
    }

    // safety fixes
    if ((sprnum < 0) || (static_cast<uint32_t>(sprnum) >= game.SpriteInfos.size())
            || !game.SpriteInfos[sprnum].IsValid())
        sprnum = 0;
    if (zoom == 0)
        zoom = 100;

    int sprwidth = game.SpriteInfos[sprnum].Width;
    int sprheight = game.SpriteInfos[sprnum].Height;
    if (zoom != 100)
    {
        scale_sprite_size(sprnum, zoom, &sprwidth, &sprheight);
    }

    res_zoom = zoom;
    res_width = sprwidth;
    res_height = sprheight;
}

void update_object_scale(int objid)
{
    RoomObject &obj = objs[objid];
    if (obj.on == 0)
        return; // not enabled

    int zoom, scale_width, scale_height;
    update_object_scale(zoom, scale_width, scale_height,
        obj.x, obj.y, obj.num, obj.zoom, (obj.flags & OBJF_USEROOMSCALING) != 0);

    obj.zoom = zoom;
    obj.last_width = scale_width;
    obj.last_height = scale_height;
}

void get_object_blocking_rect(int objid, int *x1, int *y1, int *width, int *y2) {
    RoomObject *tehobj = &objs[objid];
    int cwidth, fromx;

    if (tehobj->blocking_width < 1)
        cwidth = game_to_data_coord(tehobj->last_width) - 4;
    else
        cwidth = tehobj->blocking_width;

    fromx = tehobj->x + (game_to_data_coord(tehobj->last_width) / 2) - cwidth / 2;
    if (fromx < 0) {
        cwidth += fromx;
        fromx = 0;
    }
    if (fromx + cwidth >= mask_to_room_coord(walkable_areas_temp->GetWidth()))
        cwidth = mask_to_room_coord(walkable_areas_temp->GetWidth()) - fromx;

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
// arx,ary,spww,sphh are the sprite's bounding box
// bitmap_original tells whether bitmap is an original sprite, or transformed version
int is_pos_in_sprite(int xx, int yy, int arx, int ary, Bitmap *sprit,
                     int spww, int sphh, int flipped, bool bitmap_original) {
    if (spww==0) spww = game_to_data_coord(sprit->GetWidth()) - 1;
    if (sphh==0) sphh = game_to_data_coord(sprit->GetHeight()) - 1;

    if (isposinbox(xx,yy,arx,ary,arx+spww,ary+sphh)==FALSE)
        return FALSE;

    if (game.options[OPT_PIXPERFECT]) 
    {
        // if it's transparent, or off the edge of the sprite, ignore
        int xpos = data_to_game_coord(xx - arx);
        int ypos = data_to_game_coord(yy - ary);

        if (bitmap_original)
        {
            // Bitmap has original sprite's resolution,
            // thus adjust our calculations to compensate
            data_to_game_coords(&spww, &sphh);

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

// X and Y co-ordinates must be in native format (TODO: find out if this comment is still true)
int check_click_on_object(int roomx, int roomy, int mood)
{
    int aa = GetObjectIDAtRoom(roomx, roomy);
    if (aa < 0) return 0;
    RunObjectInteraction(aa, mood);
    return 1;
}

void ValidateViewAnimParams(const char *apiname, int &repeat, int &blocking, int &direction)
{
    if (blocking == BLOCKING)
        blocking = 1;
    else if (blocking == IN_BACKGROUND)
        blocking = 0;

    if (direction == FORWARDS)
        direction = 0;
    else if (direction == BACKWARDS)
        direction = 1;

    if ((repeat < ANIM_ONCE) || (repeat > ANIM_ONCERESET))
    {
        debug_script_warn("%s: invalid repeat value %d, will treat as REPEAT (1).", apiname, repeat);
        repeat = ANIM_REPEAT;
    }
    if ((blocking < 0) || (blocking > 1))
    {
        debug_script_warn("%s: invalid blocking value %d, will treat as BLOCKING (1)", apiname, blocking);
        blocking = 1;
    }
    if ((direction < 0) || (direction > 1))
    {
        debug_script_warn("%s: invalid direction value %d, will treat as BACKWARDS (1)", apiname, direction);
        direction = 1;
    }
}

void ValidateViewAnimVLF(const char *apiname, int view, int loop, int &sframe)
{
    // NOTE: we assume that the view is already in an internal 0-based range.
    // but when printing an error we will use (view + 1) for compliance with the script API.
    AssertLoop(apiname, view, loop);

    if (views[view].loops[loop].numFrames < 1)
        debug_script_warn("%s: view %d loop %d does not have any frames, will use a frame placeholder.",
            apiname, view + 1, loop);
    else if (sframe < 0 || sframe >= views[view].loops[loop].numFrames)
        debug_script_warn("%s: invalid starting frame number %d for view %d loop %d (range is 0..%d)",
            apiname, sframe, view + 1, loop, views[view].loops[loop].numFrames - 1);
    // NOTE: there's always frame 0 allocated for safety
    sframe = std::max(0, std::min(sframe, views[view].loops[loop].numFrames - 1));
}

int SetFirstAnimFrame(int view, int loop, int sframe, int direction)
{
    if (views[view].loops[loop].numFrames <= 1)
        return 0;
    // reverse animation starts at the *previous frame*
    if (direction != 0)
    {
        sframe--;
        if (sframe < 0)
            sframe = views[view].loops[loop].numFrames - (-sframe);
    }
    return sframe;
}

// General view animation algorithm: find next loop and frame, depending on anim settings
bool CycleViewAnim(int view, uint16_t &o_loop, uint16_t &o_frame, bool forwards, int repeat)
{
    // Allow multi-loop repeat: idk why, but original engine behavior
    // was to only check this for forward animation, not backward
    const bool multi_loop_repeat = !forwards || (play.no_multiloop_repeat == 0);

    ViewStruct *aview = &views[view];
    uint16_t loop = o_loop;
    uint16_t frame = o_frame;
    bool done = false;
    
    if (forwards)
    {
        if (frame + 1 >= aview->loops[loop].numFrames)
        { // Reached the last frame in the loop, find out what to do next
            if (aview->loops[loop].RunNextLoop())
            {
                // go to next loop
                loop++;
                frame = 0;
            }
            else
            {
                // If either ANIM_REPEAT or ANIM_ONCERESET:
                // reset to the beginning of a multiloop animation
                if (repeat != ANIM_ONCE)
                {
                    frame = 0;
                    if (multi_loop_repeat)
                        while ((loop > 0) && (aview->loops[loop - 1].RunNextLoop()))
                            loop--;
                }
                else
                { // if ANIM_ONCE, stop at the last frame
                    frame = aview->loops[loop].numFrames - 1;
                }

                if (repeat != ANIM_REPEAT) // either ANIM_ONCE or ANIM_ONCERESET
                    done = true; // finished animation
            }
        }
        else
            frame++;
    }
    else // backwards
    {
        if (frame == 0)
        { // Reached the first frame in the loop, find out what to do next
            if ((loop > 0) && aview->loops[loop - 1].RunNextLoop())
            {
                // go to next loop
                loop--;
                frame = aview->loops[loop].numFrames - 1;
            }
            else
            {
                // If either ANIM_REPEAT or ANIM_ONCERESET:
                // reset to the beginning of a multiloop animation
                if (repeat != ANIM_ONCE)
                {
                    if (multi_loop_repeat)
                        while (aview->loops[loop].RunNextLoop())
                            loop++;
                    frame = aview->loops[loop].numFrames - 1;
                }
                else
                { // if ANIM_ONCE, stop at the first frame
                    frame = 0;
                }

                if (repeat != ANIM_REPEAT) // either ANIM_ONCE or ANIM_ONCERESET
                    done = true; // finished animation
            }
        }
        else
            frame--;
    }

    // Update object values
    o_loop = loop;
    o_frame = frame;
    return !done; // have we finished animating?
}

void ValidateMoveParams(const char *apiname, int &blocking, int &ignwal)
{
    if (blocking == BLOCKING)
        blocking = 1;
    else if (blocking == IN_BACKGROUND)
        blocking = 0;

    if (ignwal == ANYWHERE)
        ignwal = 1;
    else if (ignwal == WALKABLE_AREAS)
        ignwal = 0;

    if ((blocking < 0) || (blocking > 1))
    {
        debug_script_warn("%s: invalid 'blocking' value %d, will treat as BLOCKING (1)", apiname, blocking);
        blocking = 1;
    }
    if ((ignwal < 0) || (ignwal > 1))
    {
        debug_script_warn("%s: invalid 'walk where' value %d, will treat as ANYWHERE (1)", apiname, ignwal);
        ignwal = 1;
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
#include "ac/dynobj/scriptstring.h"


ScriptObject *Object_GetByName(const char *name)
{
    return static_cast<ScriptObject*>(ccGetScriptObjectAddress(name, ccDynamicObject.GetType()));
}


RuntimeScriptValue Sc_Object_GetByName(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_POBJ(ScriptObject, ccDynamicObject, Object_GetByName, const char);
}

// void (ScriptObject *objj, int loop, int delay, int repeat, int blocking, int direction)
RuntimeScriptValue Sc_Object_Animate5(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT5(ScriptObject, Object_Animate5);
}

RuntimeScriptValue Sc_Object_Animate6(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT6(ScriptObject, Object_Animate6);
}

RuntimeScriptValue Sc_Object_Animate(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT7(ScriptObject, Object_Animate);
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

RuntimeScriptValue Sc_Object_SetProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL_POBJ_PINT(ScriptObject, Object_SetProperty, const char);
}

RuntimeScriptValue Sc_Object_SetTextProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL_POBJ2(ScriptObject, Object_SetTextProperty, const char, const char);
}

// void (ScriptObject *objj)
RuntimeScriptValue Sc_Object_MergeIntoBackground(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptObject, Object_MergeIntoBackground);
}

RuntimeScriptValue Sc_Object_IsInteractionAvailable(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL_PINT(ScriptObject, Object_IsInteractionAvailable);
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

RuntimeScriptValue Sc_Object_HasExplicitLight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(ScriptObject, Object_HasExplicitLight);
}

RuntimeScriptValue Sc_Object_HasExplicitTint(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(ScriptObject, Object_HasExplicitTint);
}

RuntimeScriptValue Sc_Object_GetLightLevel(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetLightLevel);
}

RuntimeScriptValue Sc_Object_SetLightLevel(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptObject, Object_SetLightLevel);
}

RuntimeScriptValue Sc_Object_GetTintBlue(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetTintBlue);
}

RuntimeScriptValue Sc_Object_GetTintGreen(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetTintGreen);
}

RuntimeScriptValue Sc_Object_GetTintRed(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetTintRed);
}

RuntimeScriptValue Sc_Object_GetTintSaturation(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetTintSaturation);
}

RuntimeScriptValue Sc_Object_GetTintLuminance(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetTintLuminance);
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

RuntimeScriptValue Sc_GetObjectAtRoom(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT2(ScriptObject, ccDynamicObject, GetObjectAtRoom);
}

// ScriptObject *(int xx, int yy)
RuntimeScriptValue Sc_GetObjectAtScreen(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT2(ScriptObject, ccDynamicObject, GetObjectAtScreen);
}

// int (ScriptObject *objj)
RuntimeScriptValue Sc_Object_GetAnimating(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetAnimating);
}

RuntimeScriptValue Sc_Object_GetAnimationVolume(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetAnimationVolume);
}

RuntimeScriptValue Sc_Object_SetAnimationVolume(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptObject, Object_SetAnimationVolume);
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

RuntimeScriptValue Sc_Object_GetDestinationX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetDestinationX);
}

RuntimeScriptValue Sc_Object_GetDestinationY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetDestinationY);
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

RuntimeScriptValue Sc_Object_GetScriptName(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptObject, const char, myScriptStringImpl, Object_GetScriptName);
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

RuntimeScriptValue Sc_Object_SetManualScaling(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PBOOL(ScriptObject, Object_SetManualScaling);
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

RuntimeScriptValue Sc_Object_SetName(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(ScriptObject, Object_SetName, const char);
}

RuntimeScriptValue Sc_Object_GetScaling(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetScaling);
}

RuntimeScriptValue Sc_Object_SetScaling(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptObject, Object_SetScaling);
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
    ScFnRegister object_api[] = {
        { "Object::GetAtRoomXY^2",            API_FN_PAIR(GetObjectAtRoom) },
        { "Object::GetAtScreenXY^2",          API_FN_PAIR(GetObjectAtScreen) },
        { "Object::GetByName",                API_FN_PAIR(Object_GetByName) },

        { "Object::Animate^5",                API_FN_PAIR(Object_Animate5) },
        { "Object::Animate^6",                API_FN_PAIR(Object_Animate6) },
        { "Object::Animate^7",                API_FN_PAIR(Object_Animate) },
        { "Object::IsCollidingWithObject^1",  API_FN_PAIR(Object_IsCollidingWithObject) },
        { "Object::GetName^1",                API_FN_PAIR(Object_GetName) },
        { "Object::GetProperty^1",            API_FN_PAIR(Object_GetProperty) },
        { "Object::GetPropertyText^2",        API_FN_PAIR(Object_GetPropertyText) },
        { "Object::GetTextProperty^1",        API_FN_PAIR(Object_GetTextProperty) },
        { "Object::SetProperty^2",            API_FN_PAIR(Object_SetProperty) },
        { "Object::SetTextProperty^2",        API_FN_PAIR(Object_SetTextProperty) },
        { "Object::IsInteractionAvailable^1", API_FN_PAIR(Object_IsInteractionAvailable) },
        { "Object::MergeIntoBackground^0",    API_FN_PAIR(Object_MergeIntoBackground) },
        { "Object::Move^5",                   API_FN_PAIR(Object_Move) },
        { "Object::RemoveTint^0",             API_FN_PAIR(Object_RemoveTint) },
        { "Object::RunInteraction^1",         API_FN_PAIR(Object_RunInteraction) },
        { "Object::SetLightLevel^1",          API_FN_PAIR(Object_SetLightLevel) },
        { "Object::SetPosition^2",            API_FN_PAIR(Object_SetPosition) },
        { "Object::SetView^3",                API_FN_PAIR(Object_SetView) },
        { "Object::StopAnimating^0",          API_FN_PAIR(Object_StopAnimating) },
        { "Object::StopMoving^0",             API_FN_PAIR(Object_StopMoving) },
        { "Object::Tint^5",                   API_FN_PAIR(Object_Tint) },
        { "Object::get_Animating",            API_FN_PAIR(Object_GetAnimating) },
        { "Object::get_AnimationVolume",      API_FN_PAIR(Object_GetAnimationVolume) },
        { "Object::set_AnimationVolume",      API_FN_PAIR(Object_SetAnimationVolume) },
        { "Object::get_Baseline",             API_FN_PAIR(Object_GetBaseline) },
        { "Object::set_Baseline",             API_FN_PAIR(Object_SetBaseline) },
        { "Object::get_BlockingHeight",       API_FN_PAIR(Object_GetBlockingHeight) },
        { "Object::set_BlockingHeight",       API_FN_PAIR(Object_SetBlockingHeight) },
        { "Object::get_BlockingWidth",        API_FN_PAIR(Object_GetBlockingWidth) },
        { "Object::set_BlockingWidth",        API_FN_PAIR(Object_SetBlockingWidth) },
        { "Object::get_Clickable",            API_FN_PAIR(Object_GetClickable) },
        { "Object::set_Clickable",            API_FN_PAIR(Object_SetClickable) },
        { "Object::get_DestinationX",         API_FN_PAIR(Object_GetDestinationX) },
        { "Object::get_DestinationY",         API_FN_PAIR(Object_GetDestinationY) },
        { "Object::get_Frame",                API_FN_PAIR(Object_GetFrame) },
        { "Object::get_Graphic",              API_FN_PAIR(Object_GetGraphic) },
        { "Object::set_Graphic",              API_FN_PAIR(Object_SetGraphic) },
        { "Object::get_ID",                   API_FN_PAIR(Object_GetID) },
        { "Object::get_IgnoreScaling",        API_FN_PAIR(Object_GetIgnoreScaling) },
        { "Object::set_IgnoreScaling",        API_FN_PAIR(Object_SetIgnoreScaling) },
        { "Object::get_IgnoreWalkbehinds",    API_FN_PAIR(Object_GetIgnoreWalkbehinds) },
        { "Object::set_IgnoreWalkbehinds",    API_FN_PAIR(Object_SetIgnoreWalkbehinds) },
        { "Object::get_Loop",                 API_FN_PAIR(Object_GetLoop) },
        { "Object::get_ManualScaling",        API_FN_PAIR(Object_GetIgnoreScaling) },
        { "Object::set_ManualScaling",        API_FN_PAIR(Object_SetManualScaling) },
        { "Object::get_Moving",               API_FN_PAIR(Object_GetMoving) },
        { "Object::get_Name",                 API_FN_PAIR(Object_GetName_New) },
        { "Object::set_Name",                 API_FN_PAIR(Object_SetName) },
        { "Object::get_Scaling",              API_FN_PAIR(Object_GetScaling) },
        { "Object::set_Scaling",              API_FN_PAIR(Object_SetScaling) },
        { "Object::get_ScriptName",           API_FN_PAIR(Object_GetScriptName) },
        { "Object::get_Solid",                API_FN_PAIR(Object_GetSolid) },
        { "Object::set_Solid",                API_FN_PAIR(Object_SetSolid) },
        { "Object::get_Transparency",         API_FN_PAIR(Object_GetTransparency) },
        { "Object::set_Transparency",         API_FN_PAIR(Object_SetTransparency) },
        { "Object::get_View",                 API_FN_PAIR(Object_GetView) },
        { "Object::get_Visible",              API_FN_PAIR(Object_GetVisible) },
        { "Object::set_Visible",              API_FN_PAIR(Object_SetVisible) },
        { "Object::get_X",                    API_FN_PAIR(Object_GetX) },
        { "Object::set_X",                    API_FN_PAIR(Object_SetX) },
        { "Object::get_Y",                    API_FN_PAIR(Object_GetY) },
        { "Object::set_Y",                    API_FN_PAIR(Object_SetY) },
        { "Object::get_HasExplicitLight",     API_FN_PAIR(Object_HasExplicitLight) },
        { "Object::get_HasExplicitTint",      API_FN_PAIR(Object_HasExplicitTint) },
        { "Object::get_LightLevel",           API_FN_PAIR(Object_GetLightLevel) },
        { "Object::set_LightLevel",           API_FN_PAIR(Object_SetLightLevel) },
        { "Object::get_TintBlue",             API_FN_PAIR(Object_GetTintBlue) },
        { "Object::get_TintGreen",            API_FN_PAIR(Object_GetTintGreen) },
        { "Object::get_TintRed",              API_FN_PAIR(Object_GetTintRed) },
        { "Object::get_TintSaturation",       API_FN_PAIR(Object_GetTintSaturation) },
        { "Object::get_TintLuminance",        API_FN_PAIR(Object_GetTintLuminance) },
    };

    ccAddExternalFunctions(object_api);
}
