#ifndef __AC_PUSHBUTTON_H
#define __AC_PUSHBUTTON_H

#include "gui/newcontrol.h"

#ifdef DJGPP
#pragma warn -inl
#endif
struct MyPushButton:public NewControl
{
  char text[50];
  MyPushButton(int xx, int yy, int wi, int hi, char *tex);
  void draw();
  int pressedon();
  int processmessage(int mcode, int wParam, long lParam);
};
#ifdef DJGPP
#pragma warn +inl
#endif

#endif // __AC_PUSHBUTTON_H