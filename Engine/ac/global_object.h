//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// FIXME: these functions are now redundant, mostly just used to delegate
// work from object.cpp. Move their impl to object.cpp, and remove this unit.
//
//=============================================================================
#ifndef __AGS_EE_AC__GLOBALOBJECT_H
#define __AGS_EE_AC__GLOBALOBJECT_H

#include "gfx/bitmap.h"

// Get object at the given screen coordinates
int  GetObjectIDAtScreen(int xx, int yy);
// Get object at the given room coordinates
int  GetObjectIDAtRoom(int roomx, int roomy);
void SetObjectTint(int obj, int red, int green, int blue, int opacity, int luminance);
void RemoveObjectTint(int obj);
// Assigns given object to the view's frame
bool SetObjectFrameSimple(int obn, int viw, int lop, int fra);
// pass trans=0 for fully solid, trans=100 for fully transparent
void SetObjectTransparency(int obn,int trans);
void SetObjectBaseline (int obn, int basel);
int  GetObjectBaseline(int obn);
void AnimateObjectImpl(int obn, int loopn, int spdd, int rept, int direction, int blocking, int sframe, int volume = 100);
void StopObjectMoving(int objj);
void SetObjectGraphic(int obn,int slott);
int  GetObjectGraphic(int obn);
int  GetObjectX (int objj);
int  GetObjectY (int objj);
int  IsObjectAnimating(int objj);
int  IsObjectMoving(int objj);
void SetObjectPosition(int objj, int tox, int toy);
void GetObjectName(int obj, char *buffer);
void SetObjectClickable (int cha, int clik);
void SetObjectIgnoreWalkbehinds (int cha, int clik);
void RunObjectInteraction (int aa, int mood);
int  AreObjectsColliding(int obj1,int obj2);
int  AreThingsOverlapping(int thing1, int thing2);

int  GetObjectProperty (int hss, const char *property);
void GetObjectPropertyText (int item, const char *property, char *bufer);

// Gets last cached object's image; this may be a source image if there was no transformation
AGS::Common::Bitmap *GetObjectImage(int obj, bool *is_original = nullptr);
// Gets current source image (untransformed) for the room object
AGS::Common::Bitmap *GetObjectSourceImage(int obj);

#endif // __AGS_EE_AC__GLOBALOBJECT_H
