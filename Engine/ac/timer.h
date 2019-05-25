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
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__TIMER_H
#define __AGS_EE_AC__TIMER_H

#include <type_traits>
#include <chrono>

// use high resolution clock only if we know it is monotonic/steady.
// refer to https://stackoverflow.com/a/38253266/84262
using AGS_Clock = std::conditional<
        std::chrono::high_resolution_clock::is_steady,
        std::chrono::high_resolution_clock, std::chrono::steady_clock
      >::type;

extern void setTimerFps(int new_fps);
extern bool waitingForNextTick();  // store last tick time.
extern void skipMissedTicks();  // if more than N frames, just skip all, start a fresh.

extern std::chrono::microseconds getFrameDuration();
extern void DelayUntilEndOfFrame();

#endif // __AGS_EE_AC__TIMER_H
