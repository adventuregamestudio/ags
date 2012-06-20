#ifndef __AC_COLLISION_H
#define __AC_COLLISION_H

#include "ac/dynobj/scriptobject.h"

#define OVERLAPPING_OBJECT 1000
struct Rect {
    int x1,y1,x2,y2;
};

void get_char_blocking_rect(int charid, int *x1, int *y1, int *width, int *y2);
// Check whether the source char has walked onto character ww
int is_char_on_another (int sourceChar, int ww, int*fromxptr, int*cwidptr);
void get_object_blocking_rect(int objid, int *x1, int *y1, int *width, int *y2);
int is_point_in_rect(int x, int y, int left, int top, int right, int bottom);


int GetThingRect(int thing, Rect *rect);
int AreThingsOverlapping(int thing1, int thing2);
int AreObjectsColliding(int obj1,int obj2);
int Object_IsCollidingWithObject(ScriptObject *objj, ScriptObject *obj2);
int my_getpixel(BITMAP *blk, int x, int y);
block GetCharacterImage(int charid, int *isFlipped);
block GetObjectImage(int obj, int *isFlipped);
int AreCharObjColliding(int charid,int objid);
int AreCharactersColliding(int cchar1,int cchar2);

#endif // __AC_COLLISION_H