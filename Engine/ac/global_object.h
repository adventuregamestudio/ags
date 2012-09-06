
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__GLOBALOBJECT_H
#define __AGS_EE_AC__GLOBALOBJECT_H

// Should be moved to math or some utils unit?
struct Rect {
    int x1,y1,x2,y2;
};

int  GetObjectAt(int xx,int yy);
void SetObjectTint(int obj, int red, int green, int blue, int opacity, int luminance);
void RemoveObjectTint(int obj);
void SetObjectView(int obn,int vii);
void SetObjectFrame(int obn,int viw,int lop,int fra);
// pass trans=0 for fully solid, trans=100 for fully transparent
void SetObjectTransparency(int obn,int trans);
void SetObjectBaseline (int obn, int basel);
int  GetObjectBaseline(int obn);
void AnimateObjectEx(int obn,int loopn,int spdd,int rept, int direction, int blocking);
void AnimateObject(int obn,int loopn,int spdd,int rept);
void MergeObject(int obn);
void StopObjectMoving(int objj);
void ObjectOff(int obn);
void ObjectOn(int obn);
int  IsObjectOn (int objj);
void SetObjectGraphic(int obn,int slott);
int  GetObjectGraphic(int obn);
int  GetObjectX (int objj);
int  GetObjectY (int objj);
int  IsObjectAnimating(int objj);
int  IsObjectMoving(int objj);
void SetObjectPosition(int objj, int tox, int toy);
void GetObjectName(int obj, char *buffer);
void MoveObject(int objj,int xx,int yy,int spp);
void MoveObjectDirect(int objj,int xx,int yy,int spp);
void SetObjectClickable (int cha, int clik);
void SetObjectIgnoreWalkbehinds (int cha, int clik);
void RunObjectInteraction (int aa, int mood);
int  AreObjectsColliding(int obj1,int obj2);
int  GetThingRect(int thing, Rect *rect);
int  AreThingsOverlapping(int thing1, int thing2);

int  GetObjectProperty (int hss, const char *property);
void GetObjectPropertyText (int item, const char *property, char *bufer);

Common::IBitmap *GetObjectImage(int obj, int *isFlipped);

#endif // __AGS_EE_AC__GLOBALOBJECT_H
