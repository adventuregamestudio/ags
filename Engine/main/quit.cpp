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

//
// Quit game procedure
//
#include <stdio.h>
#include "platform/platform.h"
#include <allegro.h> // find files, allegro_exit
#include "ac/cdaudio.h"
#include "ac/common.h"
#include "ac/game.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/roomstatus.h"
#include "ac/route_finder.h"
#include "ac/translation.h"
#include "ac/dynobj/dynobj_manager.h"
#include "debug/agseditordebugger.h"
#include "debug/debug_log.h"
#include "debug/debugger.h"
#include "debug/out.h"
#include "font/fonts.h"
#include "main/config.h"
#include "main/engine.h"
#include "main/main.h"
#include "main/quit.h"
#include "ac/spritecache.h"
#include "gfx/graphicsdriver.h"
#include "gfx/bitmap.h"
#include "data/assetmanager.h"
#include "platform/base/agsplatformdriver.h"
#include "platform/base/sys_main.h"
#include "plugin/plugin_engine.h"
#include "script/cc_common.h"
#include "media/audio/audio_system.h"
#include "media/video/video.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern GameSetupStruct game;
extern SpriteCache spriteset;
extern RoomStruct thisroom;
extern RoomStatus troom;    // used for non-saveable rooms, eg. intro
extern int proper_exit;
extern char check_dynamic_sprites_at_exit;
extern int editor_debugging_initialized;
extern IAGSEditorDebugger *editor_debugger;
extern int need_to_stop_cd;
extern int use_cdplayer;
extern IGraphicsDriver *gfxDriver;

bool handledErrorInEditor;

void quit_tell_editor_debugger(const String &qmsg, QuitReason qreason)
{
    if (editor_debugging_initialized)
    {
        if (qreason & kQuitKind_GameException)
            handledErrorInEditor = send_exception_to_debugger(qmsg.GetCStr());
        send_state_to_debugger("EXIT");
        editor_debugger->Shutdown();
    }
}

void quit_stop_cd()
{
    if (need_to_stop_cd)
        cd_manager(3,0);
}

void quit_check_dynamic_sprites(QuitReason qreason)
{
    if ((qreason & kQuitKind_NormalExit) && (check_dynamic_sprites_at_exit) && 
        (game.options[OPT_DEBUGMODE] != 0))
    {
        // Check that the dynamic sprites have been deleted;
        // ignore those that are owned by the game objects.
        for (size_t i = 1; i < spriteset.GetSpriteSlotCount(); i++)
        {
            if ((game.SpriteInfos[i].Flags & SPF_DYNAMICALLOC) &&
                ((game.SpriteInfos[i].Flags & SPF_OBJECTOWNED) == 0))
            {
                debug_script_warn("Dynamic sprite %d was never deleted", i);
            }
        }
    }
}

void quit_shutdown_audio()
{
    set_our_eip(9917);
    game.options[OPT_CROSSFADEMUSIC] = 0;
    shutdown_sound();
}

// Parses the quit message; returns:
// * QuitReason - which is a code of the reason we're quitting (game error, etc);
// * errmsg  - a simple error message.
// * fullmsg - a combined message to post into the engine output (stdout, log),
//             may include custom user text, explanation of error, callstack, etc.
QuitReason quit_check_for_error_state(const char *qmsg, const String &error_text,
    const String &game_version, String &errmsg, String &fullmsg)
{
    QuitReason qreason;
    String error_is;
    String error_kind;
    if (qmsg[0]=='|')
    {
        qreason = kQuit_GameRequest; // no error
    }
    else if (qmsg[0]=='!')
    {
        qmsg++;

        if (qmsg[0] == '|')
        {
            qmsg++;
            qreason = kQuit_UserAbort;
            error_is = "Abort key pressed.";
        }
        else if (qmsg[0] == '?')
        {
            qmsg++;
            qreason = kQuit_ScriptAbort;
            error_is = !error_text.IsEmpty() ? error_text :
                "A fatal error was reported by the game script. "
                "Please contact the game author for support.";
            error_kind = "Game script requested abort.";
        }
        else
        {
            qreason = kQuit_GameError;
            error_is = !error_text.IsEmpty() ? error_text :
                "A game error has occurred. Please contact the game author for support, as this is "
                "likely to be a mistake in game logic or script and not an error in AGS engine.";
            error_kind = "Game script or data error.";
        }

        errmsg = qmsg;
    }
    else if (qmsg[0] == '%')
    {
        qmsg++;
        error_is = !error_text.IsEmpty() ? error_text :
                "A game warning has occured, while this game is configured to treat warnings as errors.";
        error_kind = "Game script or data error.";
        errmsg = qmsg;
        qreason = kQuit_GameWarning;
    }
    else
    {
        error_is = !error_text.IsEmpty() ? error_text :
                "An internal error has occurred in AGS engine. Please note down the following information.\n"
                "If the problem persists, contact the game author for support or post these details on the AGS Technical Forum.\n";
        error_kind = "Internal engine error.";
        errmsg = qmsg;
        qreason = kQuit_FatalError;
    }

    // Display an error, unless it's a normal game exit request
    if (qreason != kQuit_GameRequest)
    {
        fullmsg = error_is;
        // Combine error with extra information, unless it was aborted by player's request
        if (qreason != kQuit_UserAbort)
        {
            // Add extra note about the nature of error if there's a custom error text;
            // this is in case game's or engine's devs will require extra clarification
            if (!error_text.IsEmpty())
                fullmsg.AppendFmt("\n\n%s", error_kind.GetCStr());
            else
                fullmsg.AppendFmt("\n");

            fullmsg.AppendFmt("\nAGS engine version: %s", EngineVersion.LongString.GetCStr());
            if (!game_version.IsEmpty())
                fullmsg.AppendFmt("\nGame version: %s", game_version.GetCStr());
            if (!errmsg.IsEmpty())
                fullmsg.AppendFmt("\n\nError: %s", errmsg.GetCStr());
        }

        // Print callstack even when player aborted the game by a hotkey,
        // because it may be done e.g. when the script is stuck in an endless loop somewhere.
        String callstack = cc_get_err_callstack();
        if (!callstack.IsEmpty())
            fullmsg.AppendFmt("\n\n%s", callstack.GetCStr());
    }

    return qreason;
}

// quit - exits the engine, shutting down everything gracefully
// The parameter is the message to print. If this message begins with
// an '!' character, then it is printed as a "contact game author" error.
// If it begins with a '|' then it is treated as a "thanks for playing" type
// message. If it begins with anything else, it is treated as an internal
// error.
// "!|" is a special code used to mean that the player has aborted (Alt+X)
void quit(const char *quitmsg)
{
    Debug::Printf(kDbgMsg_Info, "Quitting the game...");

    // Generate two error message: simple and full.
    // Simple is used to pass into debugger (IDE), full is reported with alert
    // popup and printed to the log.
    String errmsg, fullmsg;
    QuitReason qreason = quit_check_for_error_state(quitmsg, play.GetGameErrorText(),
        game.GameInfo["version"], errmsg, fullmsg);

#if defined (AGS_AUTO_WRITE_USER_CONFIG)
    if (qreason & kQuitKind_NormalExit)
        save_config_file();
#endif // AGS_AUTO_WRITE_USER_CONFIG

    handledErrorInEditor = false;

    quit_tell_editor_debugger(errmsg, qreason);

    set_our_eip(9900);

    quit_stop_cd();
    if (use_cdplayer)
        platform->ShutdownCDPlayer();
    video_shutdown();
    quit_shutdown_audio();

    set_our_eip(9908);

    // Release game data and unregister assets
    quit_check_dynamic_sprites(qreason);
    shutdown_game_state();
    unload_game();
    AssetMgr.reset();

    // Be sure to unlock mouse on exit, or users will hate us
    sys_window_lock_mouse(false);
    engine_shutdown_gfxmode();

    platform->PreBackendExit();

    // On abnormal exit: display the message (at this point the window still exists)
    if ((qreason & kQuitKind_NormalExit) == 0 && !handledErrorInEditor)
    {
        platform->DisplayAlert("%s", fullmsg.GetCStr());
    }

    // release backed library
    // WARNING: no Allegro objects should remain in memory after this,
    // if their destruction is called later, program will crash!
    shutdown_font_renderer();
    allegro_exit();
    sys_main_shutdown();

    platform->PostBackendExit();

    set_our_eip(9903);

    proper_exit=1;

    Debug::Printf(kDbgMsg_Alert, "***** ENGINE HAS SHUTDOWN");

    shutdown_debug();
    AGSPlatformDriver::Shutdown();

    set_our_eip(9904);
    exit(EXIT_NORMAL);
}

extern "C" {
    void quit_c(char*msg) {
        quit(msg);
    }
}
