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

#ifndef __AC_GUIINV_H
#define __AC_GUIINV_H

#include "gui/guiobject.h"
#include "gui/dynamicarray.h"

struct GUIInv:public GUIObject
{
  int isover;
  int charId;   // whose inventory (-1 = current player)
  int itemWidth, itemHeight;
  int topIndex;

  int itemsPerLine, numLines;  // not persisted

  virtual void WriteToFile(Common::DataStream *out);
  virtual void ReadFromFile(Common::DataStream *in, int version);

  void CalculateNumCells();

  virtual void Resized() {
    CalculateNumCells();
  }

  int CharToDisplay();

  void Draw();

  void MouseMove(int x, int y)
  {
  }

  void MouseOver()
  {
    isover = 1;
  }

  void MouseLeave()
  {
    isover = 0;
  }

  void MouseUp()
  {
    if (isover)
      activated = 1;
  }

  void KeyPress(int kp)
  {
  }

  GUIInv() {
    isover = 0;
    numSupportedEvents = 0;
    charId = -1;
    itemWidth = 40;
    itemHeight = 22;
    topIndex = 0;
    CalculateNumCells();
  }
};

extern DynamicArray<GUIInv> guiinv;
extern int numguiinv;

#endif // __AC_GUIINV_H