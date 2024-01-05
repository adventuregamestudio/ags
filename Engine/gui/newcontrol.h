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
//
//
//
//=============================================================================
#ifndef __AGS_EE_GUI__NEWCONTROL_H
#define __AGS_EE_GUI__NEWCONTROL_H

#include "gfx/bitmap.h"

using namespace AGS; // FIXME later

struct NewControl
{
  int x, y, wid, hit, state, typeandflags, wlevel;
  char visible, enabled;        // not implemented
  char needredraw;
  virtual void draw(Common::Bitmap *ds) = 0;
  virtual int pressedon(int mx, int my) = 0;
  virtual int processmessage(int, int, intptr_t) = 0;

  NewControl(int xx, int yy, int wi, int hi);
  NewControl();
  virtual ~NewControl() = default;
  int mouseisinarea(int mx, int my);
  void drawifneeded();
  void drawandmouse();
};

#endif // __AGS_EE_GUI__NEWCONTROL_H