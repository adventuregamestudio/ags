
#include "wgt2allg.h"
#include "acmain/ac_maindefines.h"
#include "acmain/ac_walkablearea.h"
#include "acmain/ac_commonheaders.h"
#include "ac/ac_object.h"

block walkareabackup=NULL, walkable_areas_temp = NULL;

void redo_walkable_areas() {

    // since this is an 8-bit memory bitmap, we can just use direct 
    // memory access
    if ((!is_linear_bitmap(thisroom.walls)) || (bitmap_color_depth(thisroom.walls) != 8))
        quit("Walkable areas bitmap not linear");

    blit(walkareabackup, thisroom.walls, 0, 0, 0, 0, thisroom.walls->w, thisroom.walls->h);

    int hh,ww;
    for (hh=0;hh<walkareabackup->h;hh++) {
        for (ww=0;ww<walkareabackup->w;ww++) {
            //      if (play.walkable_areas_on[_getpixel(thisroom.walls,ww,hh)]==0)
            if (play.walkable_areas_on[thisroom.walls->line[hh][ww]]==0)
                _putpixel(thisroom.walls,ww,hh,0);
        }
    }

}


int get_walkable_area_pixel(int x, int y)
{
    return getpixel(thisroom.walls, convert_to_low_res(x), convert_to_low_res(y));
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



int GetScalingAt (int x, int y) {
    int onarea = get_walkable_area_pixel(x, y);
    if (onarea < 0)
        return 100;

    return get_area_scaling (onarea, x, y);
}

void SetAreaScaling(int area, int min, int max) {
    if ((area < 0) || (area > MAX_WALK_AREAS))
        quit("!SetAreaScaling: invalid walkalbe area");

    if (min > max)
        quit("!SetAreaScaling: min > max");

    if ((min < 5) || (max < 5) || (min > 200) || (max > 200))
        quit("!SetAreaScaling: min and max must be in range 5-200");

    // the values are stored differently
    min -= 100;
    max -= 100;

    if (min == max) {
        thisroom.walk_area_zoom[area] = min;
        thisroom.walk_area_zoom2[area] = NOT_VECTOR_SCALED;
    }
    else {
        thisroom.walk_area_zoom[area] = min;
        thisroom.walk_area_zoom2[area] = max;
    }
}



void remove_walkable_areas_from_temp(int fromx, int cwidth, int starty, int endy) {

    fromx = convert_to_low_res(fromx);
    cwidth = convert_to_low_res(cwidth);
    starty = convert_to_low_res(starty);
    endy = convert_to_low_res(endy);

    int yyy;
    if (endy >= walkable_areas_temp->h)
        endy = walkable_areas_temp->h - 1;
    if (starty < 0)
        starty = 0;

    for (; cwidth > 0; cwidth --) {
        for (yyy = starty; yyy <= endy; yyy++)
            _putpixel (walkable_areas_temp, fromx, yyy, 0);
        fromx ++;
    }

}

int is_point_in_rect(int x, int y, int left, int top, int right, int bottom) {
    if ((x >= left) && (x < right) && (y >= top ) && (y <= bottom))
        return 1;
    return 0;
}

block prepare_walkable_areas (int sourceChar) {
    // copy the walkable areas to the temp bitmap
    blit (thisroom.walls, walkable_areas_temp, 0,0,0,0,thisroom.walls->w,thisroom.walls->h);
    // if the character who's moving doesn't block, don't bother checking
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
        if (convert_to_low_res(game.chars[ww].y) >= walkable_areas_temp->h) continue;
        if (convert_to_low_res(game.chars[ww].x) >= walkable_areas_temp->w) continue;
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
        if (convert_to_low_res(objs[ww].y) >= walkable_areas_temp->h) continue;
        if (convert_to_low_res(objs[ww].x) >= walkable_areas_temp->w) continue;
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


void RemoveWalkableArea(int areanum) {
  if ((areanum<1) | (areanum>15))
    quit("!RemoveWalkableArea: invalid area number specified (1-15).");
  play.walkable_areas_on[areanum]=0;
  redo_walkable_areas();
  DEBUG_CONSOLE("Walkable area %d removed", areanum);
}

void RestoreWalkableArea(int areanum) {
  if ((areanum<1) | (areanum>15))
    quit("!RestoreWalkableArea: invalid area number specified (1-15).");
  play.walkable_areas_on[areanum]=1;
  redo_walkable_areas();
  DEBUG_CONSOLE("Walkable area %d restored", areanum);
}


int GetWalkableAreaAt(int xxx,int yyy) {
  xxx += divide_down_coordinate(offsetx);
  yyy += divide_down_coordinate(offsety);
  if ((xxx>=thisroom.width) | (xxx<0) | (yyy<0) | (yyy>=thisroom.height))
    return 0;
  int result = get_walkable_area_pixel(xxx, yyy);
  if (result <= 0)
    return 0;
  return result;
}
