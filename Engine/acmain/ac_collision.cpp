
#include "wgt2allg.h"
#include "acmain/ac_maindefines.h"
#include "acmain/ac_commonheaders.h"


void get_char_blocking_rect(int charid, int *x1, int *y1, int *width, int *y2) {
    CharacterInfo *char1 = &game.chars[charid];
    int cwidth, fromx;

    if (char1->blocking_width < 1)
        cwidth = divide_down_coordinate(GetCharacterWidth(charid)) - 4;
    else
        cwidth = char1->blocking_width;

    fromx = char1->x - cwidth/2;
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
    if (y1)
        *y1 = char1->get_blocking_top();
    if (y2)
        *y2 = char1->get_blocking_bottom();
}

// Check whether the source char has walked onto character ww
int is_char_on_another (int sourceChar, int ww, int*fromxptr, int*cwidptr) {

    int fromx, cwidth;
    int y1, y2;
    get_char_blocking_rect(ww, &fromx, &y1, &cwidth, &y2);

    if (fromxptr)
        fromxptr[0] = fromx;
    if (cwidptr)
        cwidptr[0] = cwidth;

    // if the character trying to move is already on top of
    // this char somehow, allow them through
    if ((sourceChar >= 0) &&
        // x/width are left and width co-ords, so they need >= and <
        (game.chars[sourceChar].x >= fromx) &&
        (game.chars[sourceChar].x < fromx + cwidth) &&
        // y1/y2 are the top/bottom co-ords, so they need >= / <=
        (game.chars[sourceChar].y >= y1 ) &&
        (game.chars[sourceChar].y <= y2 ))
        return 1;

    return 0;
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

int is_point_in_rect(int x, int y, int left, int top, int right, int bottom) {
    if ((x >= left) && (x < right) && (y >= top ) && (y <= bottom))
        return 1;
    return 0;
}




#define OVERLAPPING_OBJECT 1000
struct Rect {
    int x1,y1,x2,y2;
};

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

int AreObjectsColliding(int obj1,int obj2) {
    if ((!is_valid_object(obj1)) | (!is_valid_object(obj2)))
        quit("!AreObjectsColliding: invalid object specified");

    return (AreThingsOverlapping(obj1 + OVERLAPPING_OBJECT, obj2 + OVERLAPPING_OBJECT)) ? 1 : 0;
}

int Object_IsCollidingWithObject(ScriptObject *objj, ScriptObject *obj2) {
    return AreObjectsColliding(objj->id, obj2->id);
}

int my_getpixel(BITMAP *blk, int x, int y) {
    if ((x < 0) || (y < 0) || (x >= blk->w) || (y >= blk->h))
        return -1;

    // strip the alpha channel
    return blk->vtable->getpixel(blk, x, y) & 0x00ffffff;
}

block GetCharacterImage(int charid, int *isFlipped) 
{
    if (!gfxDriver->HasAcceleratedStretchAndFlip())
    {
        if (actsps[charid + MAX_INIT_SPR] != NULL) 
        {
            // the actsps image is pre-flipped, so no longer register the image as such
            if (isFlipped)
                *isFlipped = 0;
            return actsps[charid + MAX_INIT_SPR];
        }
    }
    CharacterInfo*chin=&game.chars[charid];
    int sppic = views[chin->view].loops[chin->loop].frames[chin->frame].pic;
    return spriteset[sppic];
}

block GetObjectImage(int obj, int *isFlipped) 
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

int AreCharObjColliding(int charid,int objid) {
    if (!is_valid_character(charid))
        quit("!AreCharObjColliding: invalid character");
    if (!is_valid_object(objid))
        quit("!AreCharObjColliding: invalid object number");

    return Character_IsCollidingWithObject(&game.chars[charid], &scrObj[objid]);
}

int AreCharactersColliding(int cchar1,int cchar2) {
    if (!is_valid_character(cchar1))
        quit("!AreCharactersColliding: invalid char1");
    if (!is_valid_character(cchar2))
        quit("!AreCharactersColliding: invalid char2");

    return Character_IsCollidingWithChar(&game.chars[cchar1], &game.chars[cchar2]);
}
