
#include <stdio.h>
#include "wgt2allg.h"
#include "acmain/ac_maindefines.h"
#include "acmain/ac_object.h"
#include "acmain/ac_commonheaders.h"
#include "ac/ac_object.h"
#include "routefnd.h"
#include "acrun/ac_rundefines.h" //AGS_INLINE

AGS_INLINE int is_valid_object(int obtest) {
    if ((obtest < 0) || (obtest >= croom->numobj)) return 0;
    return 1;
}


void SetObjectTint(int obj, int red, int green, int blue, int opacity, int luminance) {
    if ((red < 0) || (green < 0) || (blue < 0) ||
        (red > 255) || (green > 255) || (blue > 255) ||
        (opacity < 0) || (opacity > 100) ||
        (luminance < 0) || (luminance > 100))
        quit("!SetObjectTint: invalid parameter. R,G,B must be 0-255, opacity & luminance 0-100");

    if (!is_valid_object(obj))
        quit("!SetObjectTint: invalid object number specified");

    DEBUG_CONSOLE("Set object %d tint RGB(%d,%d,%d) %d%%", obj, red, green, blue, opacity);

    objs[obj].tint_r = red;
    objs[obj].tint_g = green;
    objs[obj].tint_b = blue;
    objs[obj].tint_level = opacity;
    objs[obj].tint_light = (luminance * 25) / 10;
    objs[obj].flags |= OBJF_HASTINT;
}

void Object_Tint(ScriptObject *objj, int red, int green, int blue, int saturation, int luminance) {
    SetObjectTint(objj->id, red, green, blue, saturation, luminance);
}

void RemoveObjectTint(int obj) {
    if (!is_valid_object(obj))
        quit("!RemoveObjectTint: invalid object");

    if (objs[obj].flags & OBJF_HASTINT) {
        DEBUG_CONSOLE("Un-tint object %d", obj);
        objs[obj].flags &= ~OBJF_HASTINT;
    }
    else {
        debug_log("RemoveObjectTint called but object was not tinted");
    }
}

void Object_RemoveTint(ScriptObject *objj) {
    RemoveObjectTint(objj->id);
}


void SetObjectView(int obn,int vii) {
    if (!is_valid_object(obn)) quit("!SetObjectView: invalid object number specified");
    DEBUG_CONSOLE("Object %d set to view %d", obn, vii);
    if ((vii < 1) || (vii > game.numviews)) {
        char buffer[150];
        sprintf (buffer, "!SetObjectView: invalid view number (You said %d, max is %d)", vii, game.numviews);
        quit(buffer);
    }
    vii--;

    objs[obn].view=vii;
    objs[obn].frame=0;
    if (objs[obn].loop >= views[vii].numLoops)
        objs[obn].loop=0;
    objs[obn].cycling=0;
    objs[obn].num = views[vii].loops[0].frames[0].pic;
}

void SetObjectFrame(int obn,int viw,int lop,int fra) {
    if (!is_valid_object(obn)) quit("!SetObjectFrame: invalid object number specified");
    viw--;
    if (viw>=game.numviews) quit("!SetObjectFrame: invalid view number used");
    if (lop>=views[viw].numLoops) quit("!SetObjectFrame: invalid loop number used");
    objs[obn].view=viw;
    if (fra >= 0)
        objs[obn].frame=fra;
    if (lop >= 0)
        objs[obn].loop=lop;

    if (objs[obn].loop >= views[viw].numLoops)
        objs[obn].loop = 0;
    if (objs[obn].frame >= views[viw].loops[objs[obn].loop].numFrames)
        objs[obn].frame = 0;

    if (loaded_game_file_version > 32) // Skip check on 2.x
    {
        if (views[viw].loops[objs[obn].loop].numFrames == 0) 
            quit("!SetObjectFrame: specified loop has no frames");
    }

    objs[obn].cycling=0;
    objs[obn].num = views[viw].loops[objs[obn].loop].frames[objs[obn].frame].pic;
    CheckViewFrame(viw, objs[obn].loop, objs[obn].frame);
}

void Object_SetView(ScriptObject *objj, int view, int loop, int frame) {
    SetObjectFrame(objj->id, view, loop, frame);
}

// pass trans=0 for fully solid, trans=100 for fully transparent
void SetObjectTransparency(int obn,int trans) {
    if (!is_valid_object(obn)) quit("!SetObjectTransparent: invalid object number specified");
    if ((trans < 0) || (trans > 100)) quit("!SetObjectTransparent: transparency value must be between 0 and 100");
    if (trans == 0)
        objs[obn].transparent=0;
    else if (trans == 100)
        objs[obn].transparent = 255;
    else
        objs[obn].transparent=((100-trans) * 25) / 10;
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

void SetObjectBaseline (int obn, int basel) {
    if (!is_valid_object(obn)) quit("!SetObjectBaseline: invalid object number specified");
    // baseline has changed, invalidate the cache
    if (objs[obn].baseline != basel) {
        objcache[obn].ywas = -9999;
        objs[obn].baseline = basel;
    }
}

void Object_SetBaseline(ScriptObject *objj, int basel) {
    SetObjectBaseline(objj->id, basel);
}

int GetObjectBaseline(int obn) {
    if (!is_valid_object(obn)) quit("!GetObjectBaseline: invalid object number specified");

    if (objs[obn].baseline < 1)
        return 0;

    return objs[obn].baseline;
}

int Object_GetBaseline(ScriptObject *objj) {
    return GetObjectBaseline(objj->id);
}


void AnimateObjectEx(int obn,int loopn,int spdd,int rept, int direction, int blocking) {
    if (obn>=MANOBJNUM) {
        scAnimateCharacter(obn - 100,loopn,spdd,rept);
        return;
    }
    if (!is_valid_object(obn))
        quit("!AnimateObject: invalid object number specified");
    if (objs[obn].view < 0)
        quit("!AnimateObject: object has not been assigned a view");
    if (loopn >= views[objs[obn].view].numLoops)
        quit("!AnimateObject: invalid loop number specified");
    if ((direction < 0) || (direction > 1))
        quit("!AnimateObjectEx: invalid direction");
    if ((rept < 0) || (rept > 2))
        quit("!AnimateObjectEx: invalid repeat value");
    if (views[objs[obn].view].loops[loopn].numFrames < 1)
        quit("!AnimateObject: no frames in the specified view loop");

    DEBUG_CONSOLE("Obj %d start anim view %d loop %d, speed %d, repeat %d", obn, objs[obn].view+1, loopn, spdd, rept);

    objs[obn].cycling = rept+1 + (direction * 10);
    objs[obn].loop=loopn;
    if (direction == 0)
        objs[obn].frame = 0;
    else {
        objs[obn].frame = views[objs[obn].view].loops[loopn].numFrames - 1;
    }

    objs[obn].overall_speed=spdd;
    objs[obn].wait = spdd+views[objs[obn].view].loops[loopn].frames[objs[obn].frame].speed;
    objs[obn].num = views[objs[obn].view].loops[loopn].frames[objs[obn].frame].pic;
    CheckViewFrame (objs[obn].view, loopn, objs[obn].frame);

    if (blocking)
        do_main_cycle(UNTIL_CHARIS0,(int)&objs[obn].cycling);
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

void AnimateObject(int obn,int loopn,int spdd,int rept) {
    AnimateObjectEx (obn, loopn, spdd, rept, 0, 0);
}

void MergeObject(int obn) {
    if (!is_valid_object(obn)) quit("!MergeObject: invalid object specified");
    int theHeight;

    construct_object_gfx(obn, NULL, &theHeight, true);

    block oldabuf = abuf;
    abuf = thisroom.ebscene[play.bg_frame];
    if (bitmap_color_depth(abuf) != bitmap_color_depth(actsps[obn]))
        quit("!MergeObject: unable to merge object due to color depth differences");

    int xpos = multiply_up_coordinate(objs[obn].x);
    int ypos = (multiply_up_coordinate(objs[obn].y) - theHeight);

    draw_sprite_support_alpha(xpos, ypos, actsps[obn], objs[obn].num);
    invalidate_screen();
    mark_current_background_dirty();

    abuf = oldabuf;
    // mark the sprite as merged
    objs[obn].on = 2;
    DEBUG_CONSOLE("Object %d merged into background", obn);
}

void Object_MergeIntoBackground(ScriptObject *objj) {
    MergeObject(objj->id);
}

void StopObjectMoving(int objj) {
    if (!is_valid_object(objj))
        quit("!StopObjectMoving: invalid object number");
    objs[objj].moving = 0;

    DEBUG_CONSOLE("Object %d stop moving", objj);
}

void Object_StopMoving(ScriptObject *objj) {
    StopObjectMoving(objj->id);
}

void ObjectOff(int obn) {
    if (!is_valid_object(obn)) quit("!ObjectOff: invalid object specified");
    // don't change it if on == 2 (merged)
    if (objs[obn].on == 1) {
        objs[obn].on = 0;
        DEBUG_CONSOLE("Object %d turned off", obn);
        StopObjectMoving(obn);
    }
}

void ObjectOn(int obn) {
    if (!is_valid_object(obn)) quit("!ObjectOn: invalid object specified");
    if (objs[obn].on == 0) {
        objs[obn].on = 1;
        DEBUG_CONSOLE("Object %d turned on", obn);
    }
}

void Object_SetVisible(ScriptObject *objj, int onoroff) {
    if (onoroff)
        ObjectOn(objj->id);
    else
        ObjectOff(objj->id);
}

int IsObjectOn (int objj) {
    if (!is_valid_object(objj)) quit("!IsObjectOn: invalid object number");

    // ==1 is on, ==2 is merged into background
    if (objs[objj].on == 1)
        return 1;

    return 0;
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

void SetObjectGraphic(int obn,int slott) {
    if (!is_valid_object(obn)) quit("!SetObjectGraphic: invalid object specified");

    if (objs[obn].num != slott) {
        objs[obn].num = slott;
        DEBUG_CONSOLE("Object %d graphic changed to slot %d", obn, slott);
    }
    objs[obn].cycling=0;
    objs[obn].frame = 0;
    objs[obn].loop = 0;
    objs[obn].view = -1;
}

void Object_SetGraphic(ScriptObject *objj, int slott) {
    SetObjectGraphic(objj->id, slott);
}

int GetObjectGraphic(int obn) {
    if (!is_valid_object(obn)) quit("!GetObjectGraphic: invalid object specified");
    return objs[obn].num;
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

int GetObjectY (int objj) {
    if (!is_valid_object(objj)) quit("!GetObjectY: invalid object number");
    return objs[objj].y;
}

int Object_GetY(ScriptObject *objj) {
    return GetObjectY(objj->id);
}

int IsObjectAnimating(int objj) {
    if (!is_valid_object(objj)) quit("!IsObjectAnimating: invalid object number");
    return (objs[objj].cycling != 0) ? 1 : 0;
}

int Object_GetAnimating(ScriptObject *objj) {
    return IsObjectAnimating(objj->id);
}

int IsObjectMoving(int objj) {
    if (!is_valid_object(objj)) quit("!IsObjectMoving: invalid object number");
    return (objs[objj].moving > 0) ? 1 : 0;
}

int Object_GetMoving(ScriptObject *objj) {
    return IsObjectMoving(objj->id);
}


void SetObjectPosition(int objj, int tox, int toy) {
    if (!is_valid_object(objj))
        quit("!SetObjectPosition: invalid object number");

    if (objs[objj].moving > 0)
        quit("!Object.SetPosition: cannot set position while object is moving");

    objs[objj].x = tox;
    objs[objj].y = toy;
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

void convert_move_path_to_high_res(MoveList *ml)
{
    ml->fromx *= current_screen_resolution_multiplier;
    ml->fromy *= current_screen_resolution_multiplier;
    ml->lastx *= current_screen_resolution_multiplier;
    ml->lasty *= current_screen_resolution_multiplier;

    for (int i = 0; i < ml->numstage; i++)
    {
        short lowPart = (ml->pos[i] & 0x0000ffff) * current_screen_resolution_multiplier;
        short highPart = ((ml->pos[i] >> 16) & 0x0000ffff) * current_screen_resolution_multiplier;
        ml->pos[i] = (highPart << 16) | lowPart;

        ml->xpermove[i] *= current_screen_resolution_multiplier;
        ml->ypermove[i] *= current_screen_resolution_multiplier;
    }
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


void GetObjectName(int obj, char *buffer) {
  VALIDATE_STRING(buffer);
  if (!is_valid_object(obj))
    quit("!GetObjectName: invalid object number");

  strcpy(buffer, get_translation(thisroom.objectnames[obj]));
}

void Object_GetName(ScriptObject *objj, char *buffer) {
  GetObjectName(objj->id, buffer);
}

const char* Object_GetName_New(ScriptObject *objj) {
  if (!is_valid_object(objj->id))
    quit("!Object.Name: invalid object number");

  return CreateNewScriptString(get_translation(thisroom.objectnames[objj->id]));
}


void MoveObject(int objj,int xx,int yy,int spp) {
  move_object(objj,xx,yy,spp,0);
  }
void MoveObjectDirect(int objj,int xx,int yy,int spp) {
  move_object(objj,xx,yy,spp,1);
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


void SetObjectClickable (int cha, int clik) {
  if (!is_valid_object(cha))
    quit("!SetObjectClickable: Invalid object specified");
  objs[cha].flags&=~OBJF_NOINTERACT;
  if (clik == 0)
    objs[cha].flags|=OBJF_NOINTERACT;
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

void SetObjectIgnoreWalkbehinds (int cha, int clik) {
  if (!is_valid_object(cha))
    quit("!SetObjectIgnoreWalkbehinds: Invalid object specified");
  objs[cha].flags&=~OBJF_NOWALKBEHINDS;
  if (clik)
    objs[cha].flags|=OBJF_NOWALKBEHINDS;
  // clear the cache
  objcache[cha].ywas = -9999;
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
