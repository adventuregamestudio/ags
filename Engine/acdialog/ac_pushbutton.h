#ifndef __AC_PUSHBUTTON_H
#define __AC_PUSHBUTTON_H

#include "acdialog/ac_newcontrol.h"

#ifdef DJGPP
#pragma warn -inl
#endif
struct PushButton:public NewControl
{
  char text[50];
  PushButton(int xx, int yy, int wi, int hi, char *tex);
  void draw();
  int pressedon();
  int processmessage(int mcode, int wParam, long lParam);
};
#ifdef DJGPP
#pragma warn +inl
#endif

#endif // __AC_PUSHBUTTON_H