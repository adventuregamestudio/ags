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

#ifndef __AC_GUIBUTTON_H
#define __AC_GUIBUTTON_H

#include "gui/guiobject.h"
#include "gui/dynamicarray.h"
#include "util/file.h"

#define GBUT_ALIGN_TOPMIDDLE    0
#define GBUT_ALIGN_TOPLEFT      1
#define GBUT_ALIGN_TOPRIGHT     2
#define GBUT_ALIGN_MIDDLELEFT   3 
#define GBUT_ALIGN_CENTRED      4
#define GBUT_ALIGN_MIDDLERIGHT  5
#define GBUT_ALIGN_BOTTOMLEFT   6
#define GBUT_ALIGN_BOTTOMMIDDLE 7
#define GBUT_ALIGN_BOTTOMRIGHT  8

struct GUIButton:public GUIObject
{
  char text[50];
  int pic, overpic, pushedpic;
  int usepic, ispushed, isover;
  int font, textcol;
  int leftclick, rightclick;
  int lclickdata, rclickdata;
  int textAlignment, reserved1;

  virtual void WriteToFile(FILE * ooo);
  virtual void ReadFromFile(FILE * ooo, int);
  void Draw();
  void MouseUp();

  void MouseMove(int x, int y)
  {
  }

  void MouseOver()
  {
    if (ispushed)
      usepic = pushedpic;
    else
      usepic = overpic;

    isover = 1;
  }

  void MouseLeave()
  {
    usepic = pic;
    isover = 0;
  }

  virtual int MouseDown()
  {
    if (pushedpic > 0)
      usepic = pushedpic;

    ispushed = 1;
    return 0;
  }

  void KeyPress(int keycode)
  {
  }

  void reset()
  {
    GUIObject::init();
    usepic = -1;
    pic = -1;
    overpic = -1;
    pushedpic = -1;
    ispushed = 0;
    isover = 0;
    text[0] = 0;
    font = 0;
    textcol = 0;
    leftclick = 2;
    rightclick = 0;
    activated = 0;
    numSupportedEvents = 1;
    supportedEvents[0] = "Click";
    supportedEventArgs[0] = "GUIControl *control, MouseButton button";
  }

  GUIButton() {
    reset();
  }

private:
  void Draw_set_oritext(char *oritext, const char *text);
};

extern DynamicArray<GUIButton> guibuts;
extern int numguibuts;

int UpdateAnimatingButton(int bu);
void StopButtonAnimation(int idxn);

#endif // __AC_GUIBUTTON_H