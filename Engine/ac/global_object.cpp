
#include "util/wgt2allg.h"
#include "gfx/ali3d.h"
#include "ac/global_object.h"
#include "ac/common.h"
#include "ac/object.h"
#include "ac/view.h"
#include "ac/character.h"
#include "ac/draw.h"
#include "ac/event.h"
#include "ac/gamesetupstruct.h"
#include "ac/global_character.h"
#include "ac/global_translation.h"
#include "ac/object.h"
#include "ac/objectcache.h"
#include "ac/properties.h"
#include "ac/roomobject.h"
#include "ac/roomstatus.h"
#include "ac/roomstruct.h"
#include "ac/string.h"
#include "ac/viewframe.h"
#include "debug/debug.h"
#include "main/game_run.h"
#include "script/script.h"
#include "ac/spritecache.h"
#include "gfx/graphicsdriver.h"
#include "gfx/bitmap.h"

using AGS::Common::IBitmap;

#define OVERLAPPING_OBJECT 1000

extern RoomStatus*croom;
extern RoomObject*objs;
extern ViewStruct*views;
extern GameSetupStruct game;
extern ObjectCache objcache[MAX_INIT_SPR];
extern int loaded_game_file_version;
extern roomstruct thisroom;
extern CharacterInfo*playerchar;
extern int displayed_room;
extern SpriteCache spriteset;
extern int offsetx, offsety;
extern int actSpsCount;
extern IBitmap **actsps;
extern IDriverDependantBitmap* *actspsbmp;
extern IGraphicsDriver *gfxDriver;

// Used for deciding whether a char or obj was closer
int obj_lowest_yp;

int GetObjectAt(int xx,int yy) {
    int aa,bestshotyp=-1,bestshotwas=-1;
    // translate screen co-ordinates to room co-ordinates
    xx += divide_down_coordinate(offsetx);
    yy += divide_down_coordinate(offsety);
    // Iterate through all objects in the room
    for (aa=0;aa<croom->numobj;aa++) {
        if (objs[aa].on != 1) continue;
        if (objs[aa].flags & OBJF_NOINTERACT)
            continue;
        int xxx=objs[aa].x,yyy=objs[aa].y;
        int isflipped = 0;
        int spWidth = divide_down_coordinate(objs[aa].get_width());
        int spHeight = divide_down_coordinate(objs[aa].get_height());
        if (objs[aa].view >= 0)
            isflipped = views[objs[aa].view].loops[objs[aa].loop].frames[objs[aa].frame].flags & VFLG_FLIPSPRITE;

        IBitmap *theImage = GetObjectImage(aa, &isflipped);

        if (is_pos_in_sprite(xx, yy, xxx, yyy - spHeight, theImage,
            spWidth, spHeight, isflipped) == FALSE)
            continue;

        int usebasel = objs[aa].get_baseline();   
        if (usebasel < bestshotyp) continue;

        bestshotwas = aa;
        bestshotyp = usebasel;
    }
    obj_lowest_yp = bestshotyp;
    return bestshotwas;
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



void SetObjectBaseline (int obn, int basel) {
    if (!is_valid_object(obn)) quit("!SetObjectBaseline: invalid object number specified");
    // baseline has changed, invalidate the cache
    if (objs[obn].baseline != basel) {
        objcache[obn].ywas = -9999;
        objs[obn].baseline = basel;
    }
}

int GetObjectBaseline(int obn) {
    if (!is_valid_object(obn)) quit("!GetObjectBaseline: invalid object number specified");

    if (objs[obn].baseline < 1)
        return 0;

    return objs[obn].baseline;
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


void AnimateObject(int obn,int loopn,int spdd,int rept) {
    AnimateObjectEx (obn, loopn, spdd, rept, 0, 0);
}

void MergeObject(int obn) {
    if (!is_valid_object(obn)) quit("!MergeObject: invalid object specified");
    int theHeight;

    construct_object_gfx(obn, NULL, &theHeight, true);

    IBitmap *oldabuf = abuf;
    abuf = thisroom.ebscene[play.bg_frame];
    if (abuf->GetColorDepth() != actsps[obn]->GetColorDepth())
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

void StopObjectMoving(int objj) {
    if (!is_valid_object(objj))
        quit("!StopObjectMoving: invalid object number");
    objs[objj].moving = 0;

    DEBUG_CONSOLE("Object %d stop moving", objj);
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

int IsObjectOn (int objj) {
    if (!is_valid_object(objj)) quit("!IsObjectOn: invalid object number");

    // ==1 is on, ==2 is merged into background
    if (objs[objj].on == 1)
        return 1;

    return 0;
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

int GetObjectGraphic(int obn) {
    if (!is_valid_object(obn)) quit("!GetObjectGraphic: invalid object specified");
    return objs[obn].num;
}

int GetObjectY (int objj) {
    if (!is_valid_object(objj)) quit("!GetObjectY: invalid object number");
    return objs[objj].y;
}

int IsObjectAnimating(int objj) {
    if (!is_valid_object(objj)) quit("!IsObjectAnimating: invalid object number");
    return (objs[objj].cycling != 0) ? 1 : 0;
}

int IsObjectMoving(int objj) {
    if (!is_valid_object(objj)) quit("!IsObjectMoving: invalid object number");
    return (objs[objj].moving > 0) ? 1 : 0;
}

void SetObjectPosition(int objj, int tox, int toy) {
    if (!is_valid_object(objj))
        quit("!SetObjectPosition: invalid object number");

    if (objs[objj].moving > 0)
        quit("!Object.SetPosition: cannot set position while object is moving");

    objs[objj].x = tox;
    objs[objj].y = toy;
}

void GetObjectName(int obj, char *buffer) {
    VALIDATE_STRING(buffer);
    if (!is_valid_object(obj))
        quit("!GetObjectName: invalid object number");

    strcpy(buffer, get_translation(thisroom.objectnames[obj]));
}

void MoveObject(int objj,int xx,int yy,int spp) {
    move_object(objj,xx,yy,spp,0);
}
void MoveObjectDirect(int objj,int xx,int yy,int spp) {
    move_object(objj,xx,yy,spp,1);
}

void SetObjectClickable (int cha, int clik) {
    if (!is_valid_object(cha))
        quit("!SetObjectClickable: Invalid object specified");
    objs[cha].flags&=~OBJF_NOINTERACT;
    if (clik == 0)
        objs[cha].flags|=OBJF_NOINTERACT;
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

void RunObjectInteraction (int aa, int mood) {
    if (!is_valid_object(aa))
        quit("!RunObjectInteraction: invalid object number for current room");
    int passon=-1,cdata=-1;
    if (mood==MODE_LOOK) passon=0;
    else if (mood==MODE_HAND) passon=1;
    else if (mood==MODE_TALK) passon=2;
    else if (mood==MODE_PICKUP) passon=5;
    else if (mood==MODE_CUSTOM1) passon = 6;
    else if (mood==MODE_CUSTOM2) passon = 7;
    else if (mood==MODE_USE) { passon=3;
    cdata=playerchar->activeinv;
    play.usedinv=cdata; }
    evblockbasename="object%d"; evblocknum=aa;

    if (thisroom.objectScripts != NULL) 
    {
        if (passon>=0) 
        {
            if (run_interaction_script(thisroom.objectScripts[aa], passon, 4, (passon == 3)))
                return;
        }
        run_interaction_script(thisroom.objectScripts[aa], 4);  // any click on obj
    }
    else
    {
        if (passon>=0) {
            if (run_interaction_event(&croom->intrObject[aa],passon, 4, (passon == 3)))
                return;
        }
        run_interaction_event(&croom->intrObject[aa],4);  // any click on obj
    }
}

int AreObjectsColliding(int obj1,int obj2) {
    if ((!is_valid_object(obj1)) | (!is_valid_object(obj2)))
        quit("!AreObjectsColliding: invalid object specified");

    return (AreThingsOverlapping(obj1 + OVERLAPPING_OBJECT, obj2 + OVERLAPPING_OBJECT)) ? 1 : 0;
}

int GetThingRect(int thing, Rect *rect) {
    if (is_valid_character(thing)) {
        if (game.chars[thing].room != displayed_room)
            return 0;

        int charwid = divide_down_coordinate(GetCharacterWidth(thing));
        rect->x1 = game.chars[thing].x - (charwid / 2);
        rect->x2 = rect->x1 + charwid;
        rect->y1 = game.chars[thing].get_effective_y() - divide_down_coordinate(GetCharacterHeight(thing));
        rect->y2 = game.chars[thing].get_effective_y();
    }
    else if (is_valid_object(thing - OVERLAPPING_OBJECT)) {
        int objid = thing - OVERLAPPING_OBJECT;
        if (objs[objid].on != 1)
            return 0;
        rect->x1 = objs[objid].x;
        rect->x2 = objs[objid].x + divide_down_coordinate(objs[objid].get_width());
        rect->y1 = objs[objid].y - divide_down_coordinate(objs[objid].get_height());
        rect->y2 = objs[objid].y;
    }
    else
        quit("!AreThingsOverlapping: invalid parameter");

    return 1;
}

int AreThingsOverlapping(int thing1, int thing2) {
    Rect r1, r2;
    // get the bounding rectangles, and return 0 if the object/char
    // is currently turned off
    if (GetThingRect(thing1, &r1) == 0)
        return 0;
    if (GetThingRect(thing2, &r2) == 0)
        return 0;

    if ((r1.x2 > r2.x1) && (r1.x1 < r2.x2) &&
        (r1.y2 > r2.y1) && (r1.y1 < r2.y2)) {
            // determine how far apart they are
            // take the smaller of the X distances as the overlapping amount
            int xdist = abs(r1.x2 - r2.x1);
            if (abs(r1.x1 - r2.x2) < xdist)
                xdist = abs(r1.x1 - r2.x2);
            // take the smaller of the Y distances
            int ydist = abs(r1.y2 - r2.y1);
            if (abs(r1.y1 - r2.y2) < ydist)
                ydist = abs(r1.y1 - r2.y2);
            // the overlapping amount is the smaller of the X and Y ovrlap
            if (xdist < ydist)
                return xdist;
            else
                return ydist;
            //    return 1;
    }
    return 0;
}

int GetObjectProperty (int hss, const char *property) {
    if (!is_valid_object(hss))
        quit("!GetObjectProperty: invalid object");
    return get_int_property (&thisroom.objProps[hss], property);
}

void GetObjectPropertyText (int item, const char *property, char *bufer) {
    get_text_property (&thisroom.objProps[item], property, bufer);
}

IBitmap *GetObjectImage(int obj, int *isFlipped) 
{
    if (!gfxDriver->HasAcceleratedStretchAndFlip())
    {
        if (actsps[obj] != NULL) {
            // the actsps image is pre-flipped, so no longer register the image as such
            if (isFlipped)
                *isFlipped = 0;

            return actsps[obj];
        }
    }
    return spriteset[objs[obj].num];
}
