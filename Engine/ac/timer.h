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
using AGS_FastClock = std::chrono::system_clock;

template <typename TDur>
inline int64_t ToMilliseconds(TDur dur)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
}

// Sleeps for time remaining until the next game frame, updates next frame timestamp
extern void WaitForNextFrame();

// Sets real FPS to the given number of frames per second; pass 1000+ for maxed FPS mode
extern int setTimerFps(int new_fps);
// Tells whether maxed FPS mode is currently set
extern bool isTimerFpsMaxed();
// If more than N frames, just skip all, start a fresh.
extern void skipMissedTicks();

#endif // __AGS_EE_AC__TIMER_H
