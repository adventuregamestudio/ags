//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================

#ifndef __AC_MYLISTBOX_H
#define __AC_MYLISTBOX_H

#include "gui/newcontrol.h"

#define MAXLISTITEM 300
#define ARROWWIDTH 8

struct MyListBox:public NewControl
{
  int items, topitem, numonscreen, selected;
  char *itemnames[MAXLISTITEM];
  MyListBox(int xx, int yy, int wii, int hii);
  void clearlist();
  ~MyListBox() override;

  void draw(Common::Bitmap *ds) override;
  int pressedon(int mx, int my) override;
  void additem(char *texx);
  int processmessage(int mcode, int wParam, intptr_t ipParam) override;
};

#endif // __AC_MYLISTBOX_H