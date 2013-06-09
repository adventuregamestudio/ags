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

#include "ac/timer.h"
#include "util/wgt2allg.h" // END_OF_FUNCTION macro

extern volatile int mvolcounter;

volatile unsigned long globalTimerCounter = 0;

volatile int timerloop=0;
int time_between_timers=25;  // in milliseconds
// our timer, used to keep game running at same speed on all systems
#if defined(WINDOWS_VERSION)
void __cdecl dj_timer_handler() {
#elif defined(DOS_VERSION)
void dj_timer_handler(...) {
#else
extern "C" void dj_timer_handler() {
#endif
    timerloop++;
    globalTimerCounter++;
    if (mvolcounter > 0) mvolcounter++;
}
END_OF_FUNCTION(dj_timer_handler);
