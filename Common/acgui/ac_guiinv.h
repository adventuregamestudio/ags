#ifndef __AC_GUIINV_H
#define __AC_GUIINV_H

#include "acgui/ac_guiobject.h"
#include "acgui/ac_dynamicarray.h"

struct GUIInv:public GUIObject
{
  int isover;
  int charId;   // whose inventory (-1 = current player)
  int itemWidth, itemHeight;
  int topIndex;

  int itemsPerLine, numLines;  // not persisted

  virtual void WriteToFile(FILE * ooo);
  virtual void ReadFromFile(FILE * ooo, int version);

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