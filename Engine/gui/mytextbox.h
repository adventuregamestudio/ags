#ifndef __AC_MYTEXTBOX_H
#define __AC_MYTEXTBOX_H

#include "gui/newcontrol.h"

#define TEXTBOX_MAXLEN 49
#ifdef DJGPP
#pragma warn -inl
#endif
struct MyTextBox:public NewControl
{
  char text[TEXTBOX_MAXLEN + 1];
  MyTextBox(int xx, int yy, int wii, char *tee);
  void draw();
  int pressedon();
  int processmessage(int mcode, int wParam, long lParam);
};
#ifdef DJGPP
#pragma warn +inl
#endif

#endif // __AC_MYTEXTBOX_H