//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// Game update timer control.
//
//=============================================================================
#ifndef __AGS_EE_AC__TIMER_H
#define __AGS_EE_AC__TIMER_H

#include "util/time_util.h"

// Sleeps for time remaining until the next game frame, updates next frame timestamp
extern void WaitForNextFrame();

// Sets real FPS to the given number of frames per second
extern int setTimerFps(int new_fps, bool max_fps_mode);
// Tells whether maxed FPS mode is currently set
extern bool isTimerFpsMaxed();
// If more than N frames, just skip all, start a fresh.
extern void skipMissedTicks();

#endif // __AGS_EE_AC__TIMER_H
