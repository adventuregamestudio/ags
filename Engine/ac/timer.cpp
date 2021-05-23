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
#include "core/platform.h"
#include <thread>
#include "platform/base/agsplatformdriver.h"

extern volatile int game_update_suspend;

namespace {

const auto MAXIMUM_FALL_BEHIND = 3;

auto tick_duration = std::chrono::microseconds(1000000LL/40);
auto framerate_maxed = false;

auto last_tick_time = AGS_Clock::now();
auto next_frame_timestamp = AGS_Clock::now();

}

std::chrono::microseconds GetFrameDuration()
{
    if (framerate_maxed) {
        return std::chrono::microseconds(0);
    }
    return tick_duration;
}

void setTimerFps(int new_fps) 
{
    tick_duration = std::chrono::microseconds(1000000LL/new_fps);
    framerate_maxed = new_fps >= 1000;

    last_tick_time = AGS_Clock::now();
    next_frame_timestamp = AGS_Clock::now();
}

bool isTimerFpsMaxed()
{
    return framerate_maxed;
}

void WaitForNextFrame()
{
    auto now = AGS_Clock::now();
    auto frameDuration = GetFrameDuration();

    // early exit if we're trying to maximise framerate
    if (frameDuration <= std::chrono::milliseconds::zero()) {
        next_frame_timestamp = now;
        // suspend while the game is being switched out
        while (game_update_suspend > 0) {
            platform->YieldCPU();
        }
        return;
    }

    // jump ahead if we're lagging
    if (next_frame_timestamp < (now - MAXIMUM_FALL_BEHIND*frameDuration)) {
        next_frame_timestamp = now;
    }

    auto frame_time_remaining = next_frame_timestamp - now;
    if (frame_time_remaining > std::chrono::milliseconds::zero()) {
        std::this_thread::sleep_for(frame_time_remaining);
    }
    
    next_frame_timestamp += frameDuration;

    // suspend while the game is being switched out
    while (game_update_suspend > 0) {
        platform->YieldCPU();
    }
}

void skipMissedTicks() 
{
    last_tick_time = AGS_Clock::now();
    next_frame_timestamp = AGS_Clock::now();
}
