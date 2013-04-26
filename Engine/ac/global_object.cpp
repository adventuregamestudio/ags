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

#include <stdio.h>
#include "gfx/ali3d.h"
#include "ac/global_object.h"
#include "ac/common.h"
#include "ac/object.h"
#include "ac/view.h"
#include "ac/character.h"
#include "ac/draw.h"
#include "ac/event.h"
#include "ac/global_character.h"
#include "ac/global_translation.h"
#include "ac/object.h"
#include "ac/objectcache.h"
#include "ac/properties.h"
#include "ac/string.h"
#include "ac/viewframe.h"
#include "debug/debug_log.h"
#include "game/game_objects.h"
#include "main/game_run.h"
#include "script/script.h"
#include "ac/spritecache.h"
#include "gfx/graphicsdriver.h"
#include "gfx/bitmap.h"

using AGS::Common::Bitmap;

#define OVERLAPPING_OBJECT 1000

extern ViewStruct*views;
extern CharacterInfo*playerchar;
extern int displayed_room;
extern SpriteCache spriteset;
extern int offsetx, offsety;
extern IGraphicsDriver *gfxDriver;

// Used for deciding whether a char or obj was closer
int obj_lowest_yp;

int GetObjectAt(int xx,int yy) {
    int aa,bestshotyp=-1,bestshotwas=-1;
    // translate screen co-ordinates to room co-ordinates
    xx += divide_down_coordinate(offsetx);
    yy += divide_down_coordinate(offsety);
    // Iterate through all objects in the room
    for (aa=0;aa<croom->ObjectCount;aa++) {
        if (objs[aa].IsOn != 1) continue;
        if (objs[aa].Flags & OBJF_NOINTERACT)
            continue;
        int xxx=objs[aa].X,yyy=objs[aa].Y;
        int isflipped = 0;
        int spWidth = divide_down_coordinate(objs[aa].GetWidth());
        int spHeight = divide_down_coordinate(objs[aa].GetHeight());
        if (objs[aa].View >= 0)
            isflipped = views[objs[aa].View].loops[objs[aa].Loop].frames[objs[aa].Frame].flags & VFLG_FLIPSPRITE;

        Bitmap *theImage = GetObjectImage(aa, &isflipped);

        if (is_pos_in_sprite(xx, yy, xxx, yyy - spHeight, theImage,
            spWidth, spHeight, isflipped) == FALSE)
            continue;

        int usebasel = objs[aa].GetBaseline();   
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

    objs[obj].TintR = red;
    objs[obj].TintG = green;
    objs[obj].TintB = blue;
    objs[obj].TintLevel = opacity;
    objs[obj].TintLight = (luminance * 25) / 10;
    objs[obj].Flags |= OBJF_HASTINT;
}

void RemoveObjectTint(int obj) {
    if (!is_valid_object(obj))
        quit("!RemoveObjectTint: invalid object");

    if (objs[obj].Flags & OBJF_HASTINT) {
        DEBUG_CONSOLE("Un-tint object %d", obj);
        objs[obj].Flags &= ~OBJF_HASTINT;
    }
    else {
        debug_log("RemoveObjectTint called but object was not tinted");
    }
}

void SetObjectView(int obn,int vii) {
    if (!is_valid_object(obn)) quit("!SetObjectView: invalid object number specified");
    DEBUG_CONSOLE("Object %d set to view %d", obn, vii);
    if ((vii < 1) || (vii > game.ViewCount)) {
        char buffer[150];
        sprintf (buffer, "!SetObjectView: invalid view number (You said %d, max is %d)", vii, game.ViewCount);
        quit(buffer);
    }
    vii--;

    objs[obn].View=vii;
    objs[obn].Frame=0;
    if (objs[obn].Loop >= views[vii].numLoops)
        objs[obn].Loop=0;
    objs[obn].Cycling=0;
    objs[obn].SpriteIndex = views[vii].loops[0].frames[0].pic;
}

void SetObjectFrame(int obn,int viw,int lop,int fra) {
    if (!is_valid_object(obn)) quit("!SetObjectFrame: invalid object number specified");
    viw--;
    if (viw>=game.ViewCount) quit("!SetObjectFrame: invalid view number used");
    if (lop>=views[viw].numLoops) quit("!SetObjectFrame: invalid loop number used");
    objs[obn].View=viw;
    if (fra >= 0)
        objs[obn].Frame=fra;
    if (lop >= 0)
        objs[obn].Loop=lop;

    if (objs[obn].Loop >= views[viw].numLoops)
        objs[obn].Loop = 0;
    if (objs[obn].Frame >= views[viw].loops[objs[obn].Loop].numFrames)
        objs[obn].Frame = 0;

    if (loaded_game_file_version > kGameVersion_272) // Skip check on 2.x
    {
        if (views[viw].loops[objs[obn].Loop].numFrames == 0) 
            quit("!SetObjectFrame: specified loop has no frames");
    }

    objs[obn].Cycling=0;
    objs[obn].SpriteIndex = views[viw].loops[objs[obn].Loop].frames[objs[obn].Frame].pic;
    CheckViewFrame(viw, objs[obn].Loop, objs[obn].Frame);
}

// pass trans=0 for fully solid, trans=100 for fully transparent
void SetObjectTransparency(int obn,int trans) {
    if (!is_valid_object(obn)) quit("!SetObjectTransparent: invalid object number specified");
    if ((trans < 0) || (trans > 100)) quit("!SetObjectTransparent: transparency value must be between 0 and 100");
    if (trans == 0)
        objs[obn].Transparency=0;
    else if (trans == 100)
        objs[obn].Transparency = 255;
    else
        objs[obn].Transparency=((100-trans) * 25) / 10;
}



void SetObjectBaseline (int obn, int basel) {
    if (!is_valid_object(obn)) quit("!SetObjectBaseline: invalid object number specified");
    // baseline has changed, invalidate the cache
    if (objs[obn].Baseline != basel) {
        objcache[obn].ywas = -9999;
        objs[obn].Baseline = basel;
    }
}

int GetObjectBaseline(int obn) {
    if (!is_valid_object(obn)) quit("!GetObjectBaseline: invalid object number specified");

    if (objs[obn].Baseline < 1)
        return 0;

    return objs[obn].Baseline;
}

void AnimateObjectEx(int obn,int loopn,int spdd,int rept, int direction, int blocking) {
    if (obn>=MANOBJNUM) {
        scAnimateCharacter(obn - 100,loopn,spdd,rept);
        return;
    }
    if (!is_valid_object(obn))
        quit("!AnimateObject: invalid object number specified");
    if (objs[obn].View < 0)
        quit("!AnimateObject: object has not been assigned a view");
    if (loopn >= views[objs[obn].View].numLoops)
        quit("!AnimateObject: invalid loop number specified");
    if ((direction < 0) || (direction > 1))
        quit("!AnimateObjectEx: invalid direction");
    if ((rept < 0) || (rept > 2))
        quit("!AnimateObjectEx: invalid repeat value");
    if (views[objs[obn].View].loops[loopn].numFrames < 1)
        quit("!AnimateObject: no frames in the specified view loop");

    DEBUG_CONSOLE("Obj %d start anim view %d loop %d, speed %d, repeat %d", obn, objs[obn].View+1, loopn, spdd, rept);

    objs[obn].Cycling = rept+1 + (direction * 10);
    objs[obn].Loop=loopn;
    if (direction == 0)
        objs[obn].Frame = 0;
    else {
        objs[obn].Frame = views[objs[obn].View].loops[loopn].numFrames - 1;
    }

    objs[obn].OverallSpeed=spdd;
    objs[obn].Wait = spdd+views[objs[obn].View].loops[loopn].frames[objs[obn].Frame].speed;
    objs[obn].SpriteIndex = views[objs[obn].View].loops[loopn].frames[objs[obn].Frame].pic;
    CheckViewFrame (objs[obn].View, loopn, objs[obn].Frame);

    if (blocking)
        GameLoopUntilEvent(UNTIL_CHARIS0,(long)&objs[obn].Cycling);
}


void AnimateObject(int obn,int loopn,int spdd,int rept) {
    AnimateObjectEx (obn, loopn, spdd, rept, 0, 0);
}

void MergeObject(int obn) {
    if (!is_valid_object(obn)) quit("!MergeObject: invalid object specified");
    int theHeight;

    construct_object_gfx(obn, NULL, &theHeight, true);

    //Bitmap *oldabuf = graphics->bmp;
    //abuf = thisroom.Backgrounds[].Graphic[play.RoomBkgFrameIndex];
    Common::Graphics graphics(thisroom.Backgrounds[play.RoomBkgFrameIndex].Graphic);
    if (graphics.GetBitmap()->GetColorDepth() != actsps[obn]->GetColorDepth())
        quit("!MergeObject: unable to merge object due to color depth differences");

    int xpos = multiply_up_coordinate(objs[obn].X);
    int ypos = (multiply_up_coordinate(objs[obn].Y) - theHeight);

    draw_sprite_support_alpha(&graphics, xpos, ypos, actsps[obn], objs[obn].SpriteIndex);
    invalidate_screen();
    mark_current_background_dirty();

    //abuf = oldabuf;
    // mark the sprite as merged
    objs[obn].IsOn = 2;
    DEBUG_CONSOLE("Object %d merged into background", obn);
}

void StopObjectMoving(int objj) {
    if (!is_valid_object(objj))
        quit("!StopObjectMoving: invalid object number");
    objs[objj].Moving = 0;

    DEBUG_CONSOLE("Object %d stop moving", objj);
}

void ObjectOff(int obn) {
    if (!is_valid_object(obn)) quit("!ObjectOff: invalid object specified");
    // don't change it if on == 2 (merged)
    if (objs[obn].IsOn == 1) {
        objs[obn].IsOn = 0;
        DEBUG_CONSOLE("Object %d turned off", obn);
        StopObjectMoving(obn);
    }
}

void ObjectOn(int obn) {
    if (!is_valid_object(obn)) quit("!ObjectOn: invalid object specified");
    if (objs[obn].IsOn == 0) {
        objs[obn].IsOn = 1;
        DEBUG_CONSOLE("Object %d turned on", obn);
    }
}

int IsObjectOn (int objj) {
    if (!is_valid_object(objj)) quit("!IsObjectOn: invalid object number");

    // ==1 is on, ==2 is merged into background
    if (objs[objj].IsOn == 1)
        return 1;

    return 0;
}

void SetObjectGraphic(int obn,int slott) {
    if (!is_valid_object(obn)) quit("!SetObjectGraphic: invalid object specified");

    if (objs[obn].SpriteIndex != slott) {
        objs[obn].SpriteIndex = slott;
        DEBUG_CONSOLE("Object %d graphic changed to slot %d", obn, slott);
    }
    objs[obn].Cycling=0;
    objs[obn].Frame = 0;
    objs[obn].Loop = 0;
    objs[obn].View = -1;
}

int GetObjectGraphic(int obn) {
    if (!is_valid_object(obn)) quit("!GetObjectGraphic: invalid object specified");
    return objs[obn].SpriteIndex;
}

int GetObjectY (int objj) {
    if (!is_valid_object(objj)) quit("!GetObjectY: invalid object number");
    return objs[objj].Y;
}

int IsObjectAnimating(int objj) {
    if (!is_valid_object(objj)) quit("!IsObjectAnimating: invalid object number");
    return (objs[objj].Cycling != 0) ? 1 : 0;
}

int IsObjectMoving(int objj) {
    if (!is_valid_object(objj)) quit("!IsObjectMoving: invalid object number");
    return (objs[objj].Moving > 0) ? 1 : 0;
}

void SetObjectPosition(int objj, int tox, int toy) {
    if (!is_valid_object(objj))
        quit("!SetObjectPosition: invalid object number");

    if (objs[objj].Moving > 0)
        quit("!Object.SetPosition: cannot set position while object is moving");

    objs[objj].X = tox;
    objs[objj].Y = toy;
}

void GetObjectName(int obj, char *buffer) {
    VALIDATE_STRING(buffer);
    if (!is_valid_object(obj))
        quit("!GetObjectName: invalid object number");

    strcpy(buffer, get_translation(thisroom.Objects[obj].Name));
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
    objs[cha].Flags&=~OBJF_NOINTERACT;
    if (clik == 0)
        objs[cha].Flags|=OBJF_NOINTERACT;
}

void SetObjectIgnoreWalkbehinds (int cha, int clik) {
    if (!is_valid_object(cha))
        quit("!SetObjectIgnoreWalkbehinds: Invalid object specified");
    objs[cha].Flags&=~OBJF_NOWALKBEHINDS;
    if (clik)
        objs[cha].Flags|=OBJF_NOWALKBEHINDS;
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
    play.UsedInvItemIndex=cdata; }
    evblockbasename="object%d"; evblocknum=aa;

    if (thisroom.Objects[aa].EventHandlers.ScriptFnRef)
    {
        if (passon>=0) 
        {
            if (run_interaction_script(thisroom.Objects[aa].EventHandlers.ScriptFnRef, passon, 4, (passon == 3)))
                return;
        }
        run_interaction_script(thisroom.Objects[aa].EventHandlers.ScriptFnRef, 4);  // any click on obj
    }
    else
    {
        if (passon>=0) {
            if (run_interaction_event(&croom->Objects[aa].Interaction,passon, 4, (passon == 3)))
                return;
        }
        run_interaction_event(&croom->Objects[aa].Interaction,4);  // any click on obj
    }
}

int AreObjectsColliding(int obj1,int obj2) {
    if ((!is_valid_object(obj1)) | (!is_valid_object(obj2)))
        quit("!AreObjectsColliding: invalid object specified");

    return (AreThingsOverlapping(obj1 + OVERLAPPING_OBJECT, obj2 + OVERLAPPING_OBJECT)) ? 1 : 0;
}

int GetThingRect(int thing, _Rect *rect) {
    if (is_valid_character(thing)) {
        if (game.Characters[thing].room != displayed_room)
            return 0;

        int charwid = divide_down_coordinate(GetCharacterWidth(thing));
        rect->x1 = game.Characters[thing].x - (charwid / 2);
        rect->x2 = rect->x1 + charwid;
        rect->y1 = game.Characters[thing].get_effective_y() - divide_down_coordinate(GetCharacterHeight(thing));
        rect->y2 = game.Characters[thing].get_effective_y();
    }
    else if (is_valid_object(thing - OVERLAPPING_OBJECT)) {
        int objid = thing - OVERLAPPING_OBJECT;
        if (objs[objid].IsOn != 1)
            return 0;
        rect->x1 = objs[objid].X;
        rect->x2 = objs[objid].X + divide_down_coordinate(objs[objid].GetWidth());
        rect->y1 = objs[objid].Y - divide_down_coordinate(objs[objid].GetHeight());
        rect->y2 = objs[objid].Y;
    }
    else
        quit("!AreThingsOverlapping: invalid parameter");

    return 1;
}

int AreThingsOverlapping(int thing1, int thing2) {
    _Rect r1, r2;
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
    return get_int_property (&thisroom.Objects[hss].Properties, property);
}

void GetObjectPropertyText (int item, const char *property, char *bufer) {
    get_text_property (&thisroom.Objects[item].Properties, property, bufer);
}

Bitmap *GetObjectImage(int obj, int *isFlipped) 
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
    return spriteset[objs[obj].SpriteIndex];
}
