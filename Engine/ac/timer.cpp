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

#include <algorithm>
#include <thread>

#include "core/platform.h"

#if AGS_PLATFORM_DEBUG && defined (__GNUC__)
#include <stdio.h>
#include <execinfo.h>
#include <unistd.h>
#endif
#include "platform/base/agsplatformdriver.h"
#include "ac/common.h"

namespace {

const auto MAXIMUM_FALL_BEHIND = 3;

auto last_tick_time = AGS_Clock::now();
auto tick_duration = std::chrono::microseconds(1000000LL/40);
auto framerate_maxed = false;

}

std::chrono::microseconds getFrameDuration()
{
    if (framerate_maxed) {
        return std::chrono::microseconds(0);
    }
    return tick_duration;
}

void setTimerFps(int new_fps) {
    tick_duration = std::chrono::microseconds(1000000LL/new_fps);
    last_tick_time = AGS_Clock::now();
    framerate_maxed = new_fps >= 1000;
}

bool waitingForNextTick() {
    auto now = AGS_Clock::now();

    if (framerate_maxed) {
        last_tick_time = now;
        return false;
    }

    // Not quite compatible with the extra delay we have after rendering:
    // After the rendering delay, the lag detection can kick in and reset the next frame time here.
    auto is_lagging = (now - last_tick_time) > (MAXIMUM_FALL_BEHIND*tick_duration);
    if (is_lagging) {
#if AGS_PLATFORM_DEBUG && defined (__GNUC__)
        auto missed_ticks = ((now - last_tick_time)/tick_duration);
        printf("Lagging! Missed %lld ticks!\n", (long long)missed_ticks);
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

auto next_frame_timestamp = AGS_Clock::now();

// to be called after drawing a frame (or after finishing partial updates)
void DelayUntilEndOfFrame() 
{
    // fake vsync
    // if we've rendered the frame and we're still within the frame period "budget", wait a little bit.
    // note: this will still be needed if gfx driver vsync is enabled because video refresh rate may differ
    auto now = AGS_Clock::now();
    auto frameDuration = getFrameDuration();
    if (frameDuration > std::chrono::milliseconds::zero()) {

        auto frame_time_remaining = next_frame_timestamp - now;
        if (frame_time_remaining > std::chrono::milliseconds(2)) {
            std::this_thread::sleep_for(frame_time_remaining);
            update_polled_stuff_if_runtime();
            now = AGS_Clock::now(); // update "now"
        }

        next_frame_timestamp += frameDuration;

        // jump ahead if we're super behind because of loading resources
        if (next_frame_timestamp < (now - 2*frameDuration)) {
            next_frame_timestamp = now;
        }
    } else {
        next_frame_timestamp = now;
    }
}
