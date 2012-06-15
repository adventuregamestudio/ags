
#include "acmain/ac_maindefines.h"


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
