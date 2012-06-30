
#include "wgt2allg.h"
#include "ac/object.h"
#include "ali3d.h"
#include "ac/ac_common.h"
#include "ac/gamesetupstruct.h"
#include "ac/ac_object.h"
#include "ac/ac_roomstruct.h"
#include "ac/character.h"
#include "ac/global_object.h"
#include "ac/objectcache.h"
#include "ac/path.h"
#include "ac/roomstatus.h"
#include "acmain/ac_customproperties.h"
#include "acmain/ac_draw.h"
#include "acmain/ac_string.h"
#include "acmain/ac_translation.h"
#include "acmain/ac_walkablearea.h"
#include "ac/rundefines.h"
#include "debug/debug.h"
#include "main/game_run.h"
#include "routefnd.h"


extern ScriptObject scrObj[MAX_INIT_SPR];
extern RoomStatus*croom;
extern RoomObject*objs;
extern roomstruct thisroom;
extern ObjectCache objcache[MAX_INIT_SPR];
extern int loaded_game_file_version;
extern int final_scrn_wid,final_scrn_hit,final_col_dep;
extern MoveList *mls;
extern GameSetupStruct game;


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

    if (objj->obj->transparent == 0)
        return 0;
    if (objj->obj->transparent == 255)
        return 100;

    return 100 - ((objj->obj->transparent * 10) / 25);

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
    if (objj->obj->view < 0)
        return 0;
    return objj->obj->view + 1;
}

int Object_GetLoop(ScriptObject *objj) {
    if (objj->obj->view < 0)
        return 0;
    return objj->obj->loop;
}

int Object_GetFrame(ScriptObject *objj) {
    if (objj->obj->view < 0)
        return 0;
    return objj->obj->frame;
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
    SetObjectPosition(objj->id, xx, objj->obj->y);
}

void Object_SetY(ScriptObject *objj, int yy) {
    SetObjectPosition(objj->id, objj->obj->x, yy);
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
        do_main_cycle(UNTIL_SHORTIS0,(int)&objj->obj->moving);
    else if ((blocking != IN_BACKGROUND) && (blocking != 0))
        quit("Object.Move: invalid BLOCKING paramter");
}

void Object_SetClickable(ScriptObject *objj, int clik) {
    SetObjectClickable(objj->id, clik);
}

int Object_GetClickable(ScriptObject *objj) {
    if (!is_valid_object(objj->id))
        quit("!Object.Clickable: Invalid object specified");

    if (objj->obj->flags & OBJF_NOINTERACT)
        return 0;
    return 1;
}

void Object_SetIgnoreScaling(ScriptObject *objj, int newval) {
    if (!is_valid_object(objj->id))
        quit("!Object.IgnoreScaling: Invalid object specified");

    objj->obj->flags &= ~OBJF_USEROOMSCALING;
    if (!newval)
        objj->obj->flags |= OBJF_USEROOMSCALING;

    // clear the cache
    objcache[objj->id].ywas = -9999;
}

int Object_GetIgnoreScaling(ScriptObject *objj) {
    if (!is_valid_object(objj->id))
        quit("!Object.IgnoreScaling: Invalid object specified");

    if (objj->obj->flags & OBJF_USEROOMSCALING)
        return 0;
    return 1;
}

void Object_SetSolid(ScriptObject *objj, int solid) {
    objj->obj->flags &= ~OBJF_SOLID;
    if (solid)
        objj->obj->flags |= OBJF_SOLID;
}

int Object_GetSolid(ScriptObject *objj) {
    if (objj->obj->flags & OBJF_SOLID)
        return 1;
    return 0;
}

void Object_SetBlockingWidth(ScriptObject *objj, int bwid) {
    objj->obj->blocking_width = bwid;
}

int Object_GetBlockingWidth(ScriptObject *objj) {
    return objj->obj->blocking_width;
}

void Object_SetBlockingHeight(ScriptObject *objj, int bhit) {
    objj->obj->blocking_height = bhit;
}

int Object_GetBlockingHeight(ScriptObject *objj) {
    return objj->obj->blocking_height;
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

    if (chaa->obj->flags & OBJF_NOWALKBEHINDS)
        return 1;
    return 0;
}

void move_object(int objj,int tox,int toy,int spee,int ignwal) {

    if (!is_valid_object(objj))
        quit("!MoveObject: invalid object number");

    // AGS <= 2.61 uses MoveObject with spp=-1 internally instead of SetObjectPosition
    if ((loaded_game_file_version <= 26) && (spee == -1))
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
    if (fromx + cwidth >= convert_back_to_high_res(walkable_areas_temp->w))
        cwidth = convert_back_to_high_res(walkable_areas_temp->w) - fromx;

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
int is_pos_in_sprite(int xx,int yy,int arx,int ary, block sprit, int spww,int sphh, int flipped) {
    if (spww==0) spww = divide_down_coordinate(sprit->w) - 1;
    if (sphh==0) sphh = divide_down_coordinate(sprit->h) - 1;

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

            if (spww != sprit->w)
                xpos = (xpos * sprit->w) / spww;
            if (sphh != sprit->h)
                ypos = (ypos * sprit->h) / sphh;
        }

        if (flipped)
            xpos = (sprit->w - 1) - xpos;

        int gpcol = my_getpixel(sprit, xpos, ypos);

        if ((gpcol == bitmap_mask_color(sprit)) || (gpcol == -1))
            return FALSE;
    }
    return TRUE;
}
