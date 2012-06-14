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

#ifndef __AC_GUITEXTBOX_H
#define __AC_GUITEXTBOX_H

#include "acgui/ac_guiobject.h"
#include "acgui/ac_dynamicarray.h"

#define GTF_NOBORDER  1
struct GUITextBox:public GUIObject
{
  char text[200];
  int font, textcol, exflags;

  virtual void WriteToFile(FILE *);
  virtual void ReadFromFile(FILE *, int);
  void Draw();
  void KeyPress(int);

  void MouseMove(int x, int y)
  {
  }

  void MouseOver()
  {
  }

  void MouseLeave()
  {
  }

  void MouseUp()
  {
  }

  void reset()
  {
    GUIObject::init();
    font = 0;
    textcol = 0;
    text[0] = 0;
    numSupportedEvents = 1;
    supportedEvents[0] = "Activate";
    supportedEventArgs[0] = "GUIControl *control";
  }

  GUITextBox() {
    reset();
  }

private:
  void Draw_text_box_contents();
};

extern DynamicArray<GUITextBox> guitext;
extern int numguitext;

#endif // __AC_GUITEXTBOX_H