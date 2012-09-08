/*
** Adventure Game Studio GUI routines
** Copyright (C) 2000-2005, Chris Jones
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE;
** the contents of this file may not be disclosed to third parties,
** copied or duplicated in any form, in whole or in part, without
** prior express permission from Chris Jones.
**
*/

#ifndef __AC_GUISLIDER_H
#define __AC_GUISLIDER_H

#include "gui/guiobject.h"
#include "gui/dynamicarray.h"

struct GUISlider:public GUIObject
{
  int min, max;
  int value, mpressed;
  int handlepic, handleoffset, bgimage;
  // The following variables are not persisted on disk
  // Cached (x1, x2, y1, y2) co-ordinates of slider handle
  int cached_handtlx, cached_handbrx;
  int cached_handtly, cached_handbry;

  virtual void WriteToFile(Common::CDataStream *out);
  virtual void ReadFromFile(Common::CDataStream *in, int version);
  void Draw();
  void MouseMove(int xp, int yp);

  void MouseOver()
  {
  }

  void MouseLeave()
  {
  }

  virtual int MouseDown()
  {
    mpressed = 1;
    // lock focus to ourselves
    return 1;
  }

  void MouseUp()
  {
    mpressed = 0;
  }

  void KeyPress(int kp)
  {
  }

  virtual int IsOverControl(int p_x, int p_y, int p_extra) {
    // check the overall boundary
    if (GUIObject::IsOverControl(p_x, p_y, p_extra))
      return 1;
    // now check the handle too
    if ((p_x >= cached_handtlx) && (p_y >= cached_handtly) &&
        (p_x < cached_handbrx) && (p_y < cached_handbry))
      return 1;
    return 0;
  }

  void reset()
  {
    GUIObject::init();
    min = 0;
    max = 10;
    value = 0;
    activated = 0;
    cached_handtlx = cached_handbrx = 0;
    cached_handtly = cached_handbry = 0;
    numSupportedEvents = 1;
    supportedEvents[0] = "Change";
    supportedEventArgs[0] = "GUIControl *control";
  }

  GUISlider() {
    reset();
  }
};

extern DynamicArray<GUISlider> guislider;
extern int numguislider;

#endif // __AC_GUISLIDER_H