/* Adventure Creator v2 Run-time engine
   Started 27-May-99 (c) 1999-2011 Chris Jones

  Adventure Game Studio source code Copyright 1999-2011 Chris Jones.
  All rights reserved.

  The AGS Editor Source Code is provided under the Artistic License 2.0
  http://www.opensource.org/licenses/artistic-license-2.0.php

  You MAY NOT compile your own builds of the engine without making it EXPLICITLY
  CLEAR that the code has been altered from the Standard Version.

*/

//
// Game initialization
//

#include "util/wgt2allg.h"
#include "ac/common.h"
#include "ac/characterinfo.h"
#include "ac/game.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_game.h"
#include "ac/mouse.h"
#include "ac/record.h"
#include "ac/room.h"
#include "ac/screen.h"
#include "debug/debug_log.h"
#include "debug/debugger.h"
#include "main/mainheader.h"
#include "main/game_run.h"
#include "main/game_start.h"
#include "script/script.h"

extern int our_eip, displayed_room;
extern const char *load_game_errors[9];
extern volatile char want_exit, abort_engine;
extern unsigned int load_new_game;
extern GameSetupStruct game;
extern GameState play;
extern volatile int timerloop;
extern const char *loadSaveGameOnStartup;
extern ccInstance *moduleInst[MAX_SCRIPT_MODULES];
extern int numScriptModules;
extern CharacterInfo*playerchar;
extern int convert_16bit_bgr;


void start_game_check_replay()
{
    write_log_debug("Checking replay status");

    if (play.recording) {
        start_recording();
    }
    else if (play.playback) {
        start_playback();
    }
}

void start_game_init_editor_debugging()
{
    if (editor_debugging_enabled)
    {
        SetMultitasking(1);
        if (init_editor_debugging())
        {
            timerloop = 0;
            while (timerloop < 20)
            {
                // pick up any breakpoints in game_start
                check_for_messages_from_editor();
            }

            ccSetDebugHook(scriptDebugHook);
        }
    }
}

void start_game_load_savegame_on_startup()
{
    if (loadSaveGameOnStartup != NULL)
    {
        int saveGameNumber = 1000;
        const char *sgName = strstr(loadSaveGameOnStartup, "agssave.");
        if (sgName != NULL)
        {
            sscanf(sgName, "agssave.%03d", &saveGameNumber);
        }
        current_fade_out_effect();
        int loadGameErrorCode = do_game_load(loadSaveGameOnStartup, saveGameNumber, NULL, NULL);
        if (loadGameErrorCode)
        {
            quitprintf("Unable to resume the save game. Try starting the game over. (Error: %s)", load_game_errors[-loadGameErrorCode]);
        }
    }
}

void start_game() {
    set_cursor_mode(MODE_WALK);
    filter->SetMousePosition(160,100);
    newmusic(0);

    our_eip = -42;

    for (int kk = 0; kk < numScriptModules; kk++)
        run_text_script(moduleInst[kk], "game_start");

    run_text_script(gameinst,"game_start");

    our_eip = -43;

    SetRestartPoint();

    our_eip=-3;

    if (displayed_room < 0) {
        current_fade_out_effect();
        load_new_room(playerchar->room,playerchar);
        // load_new_room updates it, but it should be -1 in the first room
        playerchar->prevroom = -1;
    }

    first_room_initialization();
}

void do_start_game()
{
    // only start if replay playback hasn't loaded a game
    if (displayed_room < 0)
        start_game();
}

void do_play_game()
{
    while (!abort_engine) {
        main_game_loop();

        if (load_new_game) {
            RunAGSGame (NULL, load_new_game, 0);
            load_new_game = 0;
        }
    }
}

void initialize_start_and_play_game(int override_start_room, const char *loadSaveGameOnStartup)
{
    try { // BEGIN try for ALI3DEXception

        set_cursor_mode (MODE_WALK);

        if (convert_16bit_bgr) {
            // Disable text as speech while displaying the warning message
            // This happens if the user's graphics card does BGR order 16-bit colour
            int oldalways = game.options[OPT_ALWAYSSPCH];
            game.options[OPT_ALWAYSSPCH] = 0;
            // PSP: This is normal. Don't show a warning.
            //Display ("WARNING: AGS has detected that you have an incompatible graphics card for this game. You may experience colour problems during the game. Try running the game with \"--15bit\" command line parameter and see if that helps.[[Click the mouse to continue.");
            game.options[OPT_ALWAYSSPCH] = oldalways;
        }

        srand (play.randseed);
        play.gamestep = 0;
        if (override_start_room)
            playerchar->room = override_start_room;

        start_game_check_replay();

        write_log_debug("Engine initialization complete");
        write_log_debug("Starting game");

        start_game_init_editor_debugging();

        start_game_load_savegame_on_startup();

        do_start_game();

        do_play_game();

    } catch (Ali3DException gfxException)
    {
        quit((char*)gfxException._message);
    }
}
