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

enum class TouchPhase
{
    Undefined,
    Up,
    Motion,
    Down
};

// TouchInput defines the instant change in touch input (touch event)
struct TouchInput
{
    int PointerID = -1;
    TouchPhase Phase = TouchPhase::Undefined;
    Point Position;
};

void OnTouchPointerDown(int pointer_id, const Point &position);
void OnTouchPointerMotion(int pointer_id, const Point &position);
void OnTouchPointerUp(int pointer_id, const Point &position);
// Reset all the registered touch pointer states; use this when resetting input state
void ResetAllTouchPointers();
// Removes all registered touch pointers, invalidate any script objects
void RemoveAllTouchPointers();

#endif //__AGS_EE_AC_TOUCH_H
