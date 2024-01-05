//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__GLOBALOBJECT_H
#define __AGS_EE_AC__GLOBALOBJECT_H

namespace AGS { namespace Common { class Bitmap; } }
using namespace AGS; // FIXME later

// TODO: merge with other Rect declared in bitmap unit
struct _Rect {
    int x1,y1,x2,y2;
};

// Get object at the given screen coordinates
int  GetObjectIDAtScreen(int xx,int yy);
// Get object at the given room coordinates
int  GetObjectIDAtRoom(int roomx, int roomy);
void SetObjectTint(int obj, int red, int green, int blue, int opacity, int luminance);
void RemoveObjectTint(int obj);
void SetObjectView(int obn,int vii);
// Assigns given object to the view's frame, and activates frame (plays linked sound, etc)
void SetObjectFrame(int obn,int viw,int lop,int fra);
// Assigns given object to the view's frame
bool SetObjectFrameSimple(int obn, int viw, int lop, int fra);
// pass trans=0 for fully solid, trans=100 for fully transparent
void SetObjectTransparency(int obn,int trans);
void SetObjectBaseline (int obn, int basel);
int  GetObjectBaseline(int obn);
void AnimateObject6(int obn, int loopn, int spdd, int rept, int direction, int blocking);
void AnimateObject4(int obn, int loopn, int spdd, int rept);
void AnimateObjectImpl(int obn, int loopn, int spdd, int rept, int direction, int blocking, int sframe, int volume = 100);
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
int  GetThingRect(int thing, _Rect *rect);
int  AreThingsOverlapping(int thing1, int thing2);

int  GetObjectProperty (int hss, const char *property);
void GetObjectPropertyText (int item, const char *property, char *bufer);

Common::Bitmap *GetObjectImage(int obj, bool *is_original = nullptr);

#endif // __AGS_EE_AC__GLOBALOBJECT_H
