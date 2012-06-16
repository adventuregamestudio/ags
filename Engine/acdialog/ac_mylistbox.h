#ifndef __AC_MYLISTBOX_H
#define __AC_MYLISTBOX_H

#include "acdialog/ac_newcontrol.h"

#define MAXLISTITEM 300
#define ARROWWIDTH 8

#ifdef DJGPP
#pragma warn -inl
#endif
struct MyListBox:public NewControl
{
  int items, topitem, numonscreen, selected;
  char *itemnames[MAXLISTITEM];
  MyListBox(int xx, int yy, int wii, int hii);
  void clearlist();
  ~MyListBox();

  void draw();
  int pressedon();
  void additem(char *texx);
  int processmessage(int mcode, int wParam, long lParam);
};
#ifdef DJGPP
#pragma warn +inl
#endif

#endif // __AC_MYLISTBOX_H