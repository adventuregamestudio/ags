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
#ifndef __AGS_EE_AC_TOUCH_H
#define __AGS_EE_AC_TOUCH_H

#include "util/geometry.h"

void on_touch_pointer_down(int pointer_id, Point position);
void on_touch_pointer_motion(int pointer_id, Point position);
void on_touch_pointer_up(int pointer_id, Point position);


#endif //__AGS_EE_AC_TOUCH_H
