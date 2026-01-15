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
#include <string.h>
#include "ac/game_version.h"
#include "debug/debug_log.h"
#include "script/executingscript.h"
#include "script/script.h"

using namespace AGS::Common;

void ExecutingScript::QueueAction(PostScriptAction &&act)
{
    for (const auto &prev_act : PostScriptActions)
    {
        // if something that will terminate the room has already
        // been queued, don't allow a second thing to be queued
        switch (prev_act.Type)
        {
        // A number of scheduled commands prevent ANY other scheduled command to be added
        case ePSANewRoom:
        case ePSARestoreGame:
        case ePSARestoreGameDialog:
        case ePSARunAGSGame:
        case ePSARestartGame:
            debug_script_warn("!%s: Cannot run this command, since there was a %s command already queued to run in \"%s\", line %d",
                act.Name.GetCStr(), prev_act.Name.GetCStr(),
                prev_act.Position.Section.GetCStr(), prev_act.Position.Line);
            return;
        // Dialog-state changing commands are mutually exclusive
        case ePSARunDialog:
        case ePSAStopDialog:
            if (act.Type == ePSARunDialog || act.Type == ePSAStopDialog)
            {
                debug_script_warn("!%s: Cannot run this command, since there was a %s command already queued to run in \"%s\", line %d",
                    act.Name.GetCStr(), prev_act.Name.GetCStr(),
                    prev_act.Position.Section.GetCStr(), prev_act.Position.Line);
                return;
            }
        default:
            break;
        }
    }

    PostScriptAction act_pos = std::move(act);
    get_script_position(act_pos.Position);
    PostScriptActions.push_back(std::move(act_pos));
}
