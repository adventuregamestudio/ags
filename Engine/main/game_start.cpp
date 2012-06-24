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

#include "main/mainheader.h"

void start_playback()
{
    FILE *in = fopen(replayfile, "rb");
    if (in != NULL) {
        char buffer [100];
        fread (buffer, 12, 1, in);
        buffer[12] = 0;
        if (strcmp (buffer, "AGSRecording") != 0) {
            Display("ERROR: Invalid recorded data file");
            play.playback = 0;
        }
        else {
            fgetstring_limit (buffer, in, 12);
            if (buffer[0] != '2') 
                quit("!Replay file is from an old version of AGS");
            if (strcmp (buffer, "2.55.553") < 0)
                quit("!Replay file was recorded with an older incompatible version");

            if (strcmp (buffer, ACI_VERSION_TEXT)) {
                // Disable text as speech while displaying the warning message
                // This happens if the user's graphics card does BGR order 16-bit colour
                int oldalways = game.options[OPT_ALWAYSSPCH];
                game.options[OPT_ALWAYSSPCH] = 0;
                play.playback = 0;
                Display("Warning! replay is from a different version of AGS (%s) - it may not work properly.", buffer);
                play.playback = 1;
                srand (play.randseed);
                play.gamestep = 0;
                game.options[OPT_ALWAYSSPCH] = oldalways;
            }

            int replayver = getw(in);

            if ((replayver < 1) || (replayver > 3))
                quit("!Unsupported Replay file version");

            if (replayver >= 2) {
                fgetstring_limit (buffer, in, 99);
                int uid = getw (in);
                if ((strcmp (buffer, game.gamename) != 0) || (uid != game.uniqueid)) {
                    char msg[150];
                    sprintf (msg, "!This replay is meant for the game '%s' and will not work correctly with this game.", buffer);
                    quit (msg);
                }
                // skip the total time
                getw (in);
                // replay description, maybe we'll use this later
                fgetstring_limit (buffer, in, 99);
            }

            play.randseed = getw(in);
            int flen = filelength(fileno(in)) - ftell (in);
            if (replayver >= 3) {
                flen = getw(in) * sizeof(short);
            }
            recordbuffer = (short*)malloc (flen);
            fread (recordbuffer, flen, 1, in);
            srand (play.randseed);
            recbuffersize = flen / sizeof(short);
            recsize = 0;
            disable_mgetgraphpos = 1;
            replay_time = 0;
            replay_last_second = loopcounter;
            if (replayver >= 3) {
                int issave = getw(in);
                if (issave) {
                    if (restore_game_data (in, replayfile))
                        quit("!Error running replay... could be incorrect game version");
                    replay_last_second = loopcounter;
                }
            }
            fclose (in);
        }
    }
    else // file not found
        play.playback = 0;
}

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
