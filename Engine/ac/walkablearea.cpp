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
#include "ac/common.h"
#include "ac/object.h"
#include "ac/roomstruct.h"
#include "ac/character.h"
#include "ac/draw.h"
#include "ac/gamestate.h"
#include "ac/gamesetupstruct.h"
#include "ac/object.h"
#include "ac/roomobject.h"
#include "ac/roomstatus.h"
#include "ac/walkablearea.h"
#include "gfx/bitmap.h"

using AGS::Common::Bitmap;

extern roomstruct thisroom;
extern GameState play;
extern GameSetupStruct game;
extern int spritewidth[MAX_SPRITES],spriteheight[MAX_SPRITES];
extern int displayed_room;
extern RoomStatus*croom;
extern RoomObject*objs;

Bitmap *walkareabackup=NULL, *walkable_areas_temp = NULL;

void redo_walkable_areas() {

    // since this is an 8-bit memory bitmap, we can just use direct 
    // memory access
    if ((!thisroom.walls->IsLinearBitmap()) || (thisroom.walls->GetColorDepth() != 8))
        quit("Walkable areas bitmap not linear");

    thisroom.walls->Blit(walkareabackup, 0, 0, 0, 0, thisroom.walls->GetWidth(), thisroom.walls->GetHeight());

    int hh,ww;
    for (hh=0;hh<walkareabackup->GetHeight();hh++) {
        for (ww=0;ww<walkareabackup->GetWidth();ww++) {
            //      if (play.walkable_areas_on[_getpixel(thisroom.walls,ww,hh)]==0)
            if (play.walkable_areas_on[thisroom.walls->GetScanLine(hh)[ww]]==0)
                thisroom.walls->PutPixel(ww,hh,0);
        }
    }

}

int get_walkable_area_pixel(int x, int y)
{
    return thisroom.walls->GetPixel(convert_to_low_res(x), convert_to_low_res(y));
}

int get_area_scaling (int onarea, int xx, int yy) {

    int zoom_level = 100;
    xx = convert_to_low_res(xx);
    yy = convert_to_low_res(yy);

    if ((onarea >= 0) && (onarea <= MAX_WALK_AREAS) &&
        (thisroom.walk_area_zoom2[onarea] != NOT_VECTOR_SCALED)) {
            // We have vector scaling!
            // In case the character is off the screen, limit the Y co-ordinate
            // to within the area range (otherwise we get silly zoom levels
            // that cause Out Of Memory crashes)
            if (yy > thisroom.walk_area_bottom[onarea])
                yy = thisroom.walk_area_bottom[onarea];
            if (yy < thisroom.walk_area_top[onarea])
                yy = thisroom.walk_area_top[onarea];
            // Work it all out without having to use floats
            // Percent = ((y - top) * 100) / (areabottom - areatop)
            // Zoom level = ((max - min) * Percent) / 100
            int percent = ((yy - thisroom.walk_area_top[onarea]) * 100)
                / (thisroom.walk_area_bottom[onarea] - thisroom.walk_area_top[onarea]);

            zoom_level = ((thisroom.walk_area_zoom2[onarea] - thisroom.walk_area_zoom[onarea]) * (percent)) / 100 + thisroom.walk_area_zoom[onarea];
            zoom_level += 100;
    }
    else if ((onarea >= 0) & (onarea <= MAX_WALK_AREAS))
        zoom_level = thisroom.walk_area_zoom[onarea] + 100;

    if (zoom_level == 0)
        zoom_level = 100;

    return zoom_level;
}

void scale_sprite_size(int sppic, int zoom_level, int *newwidth, int *newheight) {
    newwidth[0] = (spritewidth[sppic] * zoom_level) / 100;
    newheight[0] = (spriteheight[sppic] * zoom_level) / 100;
    if (newwidth[0] < 1)
        newwidth[0] = 1;
    if (newheight[0] < 1)
        newheight[0] = 1;
}

void remove_walkable_areas_from_temp(int fromx, int cwidth, int starty, int endy) {

    fromx = convert_to_low_res(fromx);
    cwidth = convert_to_low_res(cwidth);
    starty = convert_to_low_res(starty);
    endy = convert_to_low_res(endy);

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

Bitmap *prepare_walkable_areas (int sourceChar) {
    // copy the walkable areas to the temp bitmap
    walkable_areas_temp->Blit (thisroom.walls, 0,0,0,0,thisroom.walls->GetWidth(),thisroom.walls->GetHeight());
    // if the character who's moving doesn't Bitmap *, don't bother checking
    if (sourceChar < 0) ;
    else if (game.chars[sourceChar].flags & CHF_NOBLOCKING)
        return walkable_areas_temp;

    int ww;
    // for each character in the current room, make the area under
    // them unwalkable
    for (ww = 0; ww < game.numcharacters; ww++) {
        if (game.chars[ww].on != 1) continue;
        if (game.chars[ww].room != displayed_room) continue;
        if (ww == sourceChar) continue;
        if (game.chars[ww].flags & CHF_NOBLOCKING) continue;
        if (convert_to_low_res(game.chars[ww].y) >= walkable_areas_temp->GetHeight()) continue;
        if (convert_to_low_res(game.chars[ww].x) >= walkable_areas_temp->GetWidth()) continue;
        if ((game.chars[ww].y < 0) || (game.chars[ww].x < 0)) continue;

        CharacterInfo *char1 = &game.chars[ww];
        int cwidth, fromx;

        if (is_char_on_another(sourceChar, ww, &fromx, &cwidth))
            continue;
        if ((sourceChar >= 0) && (is_char_on_another(ww, sourceChar, NULL, NULL)))
            continue;

        remove_walkable_areas_from_temp(fromx, cwidth, char1->get_blocking_top(), char1->get_blocking_bottom());
    }

    // check for any blocking objects in the room, and deal with them
    // as well
    for (ww = 0; ww < croom->numobj; ww++) {
        if (objs[ww].on != 1) continue;
        if ((objs[ww].flags & OBJF_SOLID) == 0)
            continue;
        if (convert_to_low_res(objs[ww].y) >= walkable_areas_temp->GetHeight()) continue;
        if (convert_to_low_res(objs[ww].x) >= walkable_areas_temp->GetWidth()) continue;
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
        if (xx >= thisroom.width)
            onarea = get_walkable_area_pixel(thisroom.width-1, yy);
        else if (xx < 0)
            onarea = get_walkable_area_pixel(0, yy);
        else if (yy >= thisroom.height)
            onarea = get_walkable_area_pixel(xx, thisroom.height - 1);
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
