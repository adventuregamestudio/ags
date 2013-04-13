//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================

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
  void draw(Common::Graphics *g);
  int pressedon();
  int processmessage(int mcode, int wParam, long lParam);
};
#ifdef DJGPP
#pragma warn +inl
#endif

#endif // __AC_PUSHBUTTON_H