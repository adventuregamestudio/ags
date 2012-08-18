#ifndef __AC_MYLABEL_H
#define __AC_MYLABEL_H

#include "gui/newcontrol.h"

#ifdef DJGPP
#pragma warn -inl
#endif
struct MyLabel:public NewControl
{
  char text[150];
  MyLabel(int xx, int yy, int wii, char *tee);

  void draw();

  int pressedon();

  int processmessage(int mcode, int wParam, long lParam);
};
#ifdef DJGPP
#pragma warn +inl
#endif

#endif // __AC_MYLABEL_H