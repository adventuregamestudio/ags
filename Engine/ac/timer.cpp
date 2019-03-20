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

#include <stdio.h>
#include <execinfo.h>
#include <unistd.h>

#include "ac/timer.h"
#include "platform/base/agsplatformdriver.h"

namespace {

const auto MAXIMUM_FALL_BEHIND = 3;

auto last_tick_time = AGS_Clock::now();
auto tick_duration = std::chrono::milliseconds(1000/40);
auto framerate_maxed = false;

}

void setTimerFps(int new_fps) {
    tick_duration = std::chrono::milliseconds(1000/new_fps);
    last_tick_time = AGS_Clock::now();
    framerate_maxed = new_fps >= 1000;
}

bool waitingForNextTick() {
    auto now = AGS_Clock::now();

    if (framerate_maxed) {
        last_tick_time = now;
        return false;
    }

    auto is_lagging = (now - last_tick_time) > (MAXIMUM_FALL_BEHIND*tick_duration);
    if (is_lagging) {
#ifdef _DEBUG
        auto missed_ticks = ((now - last_tick_time)/tick_duration);
        printf("Lagging! Missed %lld ticks!\n", missed_ticks);
        void *array[10];
        auto size = backtrace(array, 10);
        backtrace_symbols_fd(array, size, STDOUT_FILENO);
        printf("\n");
#endif
        last_tick_time = now;
        return false;
    }

    auto next_tick_time = last_tick_time + tick_duration;
    if (next_tick_time <= now) {
        last_tick_time = next_tick_time;
        return false;
    }

    platform->YieldCPU();
    return true;
}

void skipMissedTicks() {
    last_tick_time = AGS_Clock::now();
}
