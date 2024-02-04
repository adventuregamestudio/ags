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

#ifndef __AC_MYTEXTBOX_H
#define __AC_MYTEXTBOX_H

#include "gui/newcontrol.h"

#define TEXTBOX_MAXLEN 49
struct MyTextBox:public NewControl
{
  char text[TEXTBOX_MAXLEN + 1];
  MyTextBox(int xx, int yy, int wii, const char *tee);
  void draw(Common::Bitmap *ds) override;
  int pressedon(int mx, int my) override;
  int processmessage(int mcode, int wParam, intptr_t ipParam) override;
};

#endif // __AC_MYTEXTBOX_H