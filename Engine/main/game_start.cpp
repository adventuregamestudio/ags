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
// Game initialization
//

#include <stdio.h>
#include "ac/common.h"
#include "ac/characterinfo.h"
#include "ac/game.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_game.h"
#include "ac/mouse.h"
#include "ac/room.h"
#include "ac/screen.h"
#include "ac/timer.h"
#include "debug/debug_log.h"
#include "debug/debugger.h"
#include "debug/out.h"
#include "device/mousew32.h"
#include "gfx/ali3dexception.h"
#include "main/game_run.h"
#include "main/game_start.h"
#include "media/audio/audio_system.h"
#include "script/script_runtime.h"
#include "script/script.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern int our_eip, displayed_room;
extern volatile char want_exit, abort_engine;
extern GameSetupStruct game;
extern GameState play;
extern std::vector<ccInstance *> moduleInst;
extern CharacterInfo*playerchar;

void start_game_init_editor_debugging()
{
    if (editor_debugging_enabled)
    {
        SetMultitasking(1);
        if (init_editor_debugging())
        {
            auto waitUntil = AGS_Clock::now() + std::chrono::milliseconds(500);
            while (waitUntil > AGS_Clock::now())
            {
                // pick up any breakpoints in game_start
                check_for_messages_from_editor();
            }

            ccSetDebugHook(scriptDebugHook);
        }
    }
}

void start_game_load_savegame_on_startup(const String &load_save)
{
    if (!load_save.IsEmpty())
    {
        int slot = 1000;
        get_save_slotnum(load_save, slot);
        current_fade_out_effect();
        try_restore_save(load_save, slot);
    }
}

void start_game() {
    set_room_placeholder();
    set_cursor_mode(MODE_WALK);
    Mouse::SetPosition(Point(160, 100));
    newmusic(0);

    our_eip = -42;

    // skip ticks to account for initialisation or a restored game.
    skipMissedTicks();

    RunScriptFunctionInModules("game_start");

    our_eip = -43;

    SetRestartPoint();

    our_eip=-3;

    if (displayed_room < 0) {
        current_fade_out_effect();
        load_new_room(playerchar->room,playerchar);
    }

    first_room_initialization();
}

void initialize_start_and_play_game(int override_start_room, const String &load_save)
{
    try { // BEGIN try for ALI3DEXception

        set_cursor_mode (MODE_WALK);

        srand (play.randseed);
        if (override_start_room)
            playerchar->room = override_start_room;

        Debug::Printf(kDbgMsg_Info, "Engine initialization complete");
        Debug::Printf(kDbgMsg_Info, "Starting game");

        start_game_init_editor_debugging();

        start_game_load_savegame_on_startup(load_save);

        // only start if not restored a save
        if (displayed_room < 0)
            start_game();

        RunGameUntilAborted();

    } catch (Ali3DException gfxException)
    {
        quit((char*)gfxException._message);
    }
}
