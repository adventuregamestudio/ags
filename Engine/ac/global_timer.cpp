//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "ac/global_timer.h"
#include "ac/runtime_defines.h"
#include "ac/common.h"
#include "ac/gamestate.h"
#include "debug/debug_log.h"

bool AssertTimerID(const char *api_name, int tnum)
{
    if (tnum < 0)
    {
        debug_script_warn("%s: invalid timer id %d, must be a number >= 0", api_name, tnum);
        return false;
    }
    return true;
}

void SetTimer(int tnum, int timeout)
{
    if (!AssertTimerID("SetTimer", tnum))
        return;
    if (timeout <= 0)
    {
        debug_script_warn("SetTimer: invalid timeout %d, must be a positive number", timeout);
        return;
    }

    play.StartScriptTimer(tnum, timeout);
}

int GetTimerPos(int tnum)
{
    if (!AssertTimerID("GetTimerPos", tnum))
        return -1;

    return play.GetScriptTimerPos(tnum);
}

int IsTimerExpired(int tnum)
{
    if (!AssertTimerID("IsTimerExpired", tnum))
        return 0;

    return play.CheckScriptTimer(tnum);
}
