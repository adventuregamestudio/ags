//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2023 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================

//
// Entry point of the application.
//

#include "core/platform.h"
#include <set>
#include <stdio.h>
#include <allegro.h> // allegro_exit
#include "ac/common.h"
#include "ac/gamesetup.h"
#include "ac/gamestate.h"
#include "core/def_version.h"
#include "debug/debugger.h"
#include "debug/debug_log.h"
#include "debug/out.h"
#include "main/config.h"
#include "main/engine.h"
#include "main/main.h"
#include "platform/base/agsplatformdriver.h"
#include "platform/base/sys_main.h"
#include "ac/route_finder.h"
#include "core/assetmanager.h"
#include "util/directory.h"
#include "util/path.h"
#include "util/string_compat.h"
#include "util/string_utils.h"

#if AGS_PLATFORM_OS_WINDOWS
#include "platform/windows/win_ex_handling.h"
#endif

#if AGS_PLATFORM_OS_WINDOWS && (!AGS_PLATFORM_DEBUG) && !AGS_PLATFORM_WINDOWS_MINGW
#define USE_CUSTOM_EXCEPTION_HANDLER
#endif

using namespace AGS::Common;
using namespace AGS::Engine;

String appPath; // engine exe path
String appDirectory; // engine dir
String cmdGameDataPath; // game path received from cmdline

extern GameState play;
extern int our_eip;
extern int editor_debugging_initialized;
extern char editor_debugger_instance_token[100];


// Startup flags, set from parameters to engine
int override_start_room = 0;
bool justDisplayHelp = false;
bool justDisplayVersion = false;
bool justRunSetup = false;
bool justTellInfo = false;
bool attachToParentConsole = false;
bool hideMessageBoxes = false;
bool logScriptRTTI = false;
std::set<String> tellInfoKeys;
String loadSaveGameOnStartup;

// CLNUP check this stuff
// Current engine version
AGS::Common::Version EngineVersion;

void main_init(int argc, char*argv[])
{
    our_eip = -999;

    // Init libraries: set text encoding
    set_uformat(U_UTF8);
    set_filename_encoding(U_UNICODE);

    EngineVersion = Version(ACI_VERSION_STR " " SPECIAL_VERSION);
#if defined (BUILD_STR)
    EngineVersion.BuildInfo = BUILD_STR;
#endif

    platform = AGSPlatformDriver::GetDriver();
    platform->SetCommandArgs(argv, argc);
    platform->MainInit();

    AssetMgr.reset(new AssetManager());
    AssetMgr->SetSearchPriority(Common::kAssetPriorityDir);
}

String get_engine_string()
{
    return String::FromFormat("Adventure Game Studio v%s Interpreter\n"
        "Copyright (c) 1999-2011 Chris Jones and " ACI_COPYRIGHT_YEARS " others\n"
        "Engine version %s\n",
        EngineVersion.ShortString.GetCStr(),
        get_engine_version_and_build().GetCStr());
}

void main_print_help() {
    platform->WriteStdOut("%s",
        "Usage: ags [OPTIONS] [GAMEFILE or DIRECTORY]\n\n"
          //--------------------------------------------------------------------------------|
           "Options:\n"
           "  --background                 Keeps game running in background\n"
           "                               (this does not work in exclusive fullscreen)\n"
           "  --clear-cache-on-room-change Clears sprite cache on every room change\n"
           "  --conf FILEPATH              Specify explicit config file to read on startup\n"
#if AGS_PLATFORM_OS_WINDOWS
           "  --console-attach             Write output to the parent process's console\n"
#endif
           "  --fps                        Display fps counter\n"
           "  --fullscreen                 Force display mode to fullscreen\n"
           "  --gfxdriver <id>             Request graphics driver. Available options:\n"
#if AGS_PLATFORM_OS_WINDOWS
           "                                 d3d9, ogl, software\n"
#else
           "                                 ogl, software\n"
#endif
           "  --gfxfilter FILTER [SCALING]\n"
           "                               Request graphics filter. Available options:\n"           
           "                                 stdscale, linear\n"
           "                               (support may differ between graphic drivers);\n"
           "                               Scaling is specified as:\n"
           "                                 proportional, round, stretch,\n"
           "                                 or an explicit integer multiplier.\n"
           "  --help                       Print this help message and stop\n"
           "  --loadsavedgame FILEPATH     Load savegame on startup\n"
           "  --localuserconf              Read and write user config in the game's \n"
           "                               directory rather than using standard system path.\n"
           "                               Game directory must be writeable.\n"
           "  --log-OUTPUT=GROUP[:LEVEL][,GROUP[:LEVEL]][,...]\n"
           "  --log-OUTPUT=+GROUPLIST[:LEVEL]\n"
           "                               Setup logging to the chosen OUTPUT with given\n"
           "                               log groups and verbosity levels. Groups may\n"
           "                               be also defined by a LIST of one-letter IDs,\n"
           "                               preceded by '+', e.g. +ABCD:LEVEL. Verbosity may\n"
           "                               be also defined by a numberic ID.\n"
           "                               OUTPUTs are\n"
           "                                 stdout, file, console\n"
           "                               (where \"console\" is internal engine's console)\n"
           "                               GROUPs are:\n"
           "                                 all, main (m), game (g), manobj (o),\n"
           "                                 script (s), sprcache (c)\n"
           "                               LEVELs are:\n"
           "                                 all, alert (1), fatal (2), error (3), warn (4),\n"
           "                                 info (5), debug (6)\n"
           "                               Examples:\n"
           "                                 --log-stdout=+mgs:debug\n"
           "                                 --log-file=all:warn\n"
           "  --log-file-path=PATH         Define custom path for the log file\n"
          //--------------------------------------------------------------------------------|
#if AGS_PLATFORM_OS_WINDOWS
           "  --no-message-box             Disable alerts as modal message boxes\n"
#endif
           "  --no-translation             Use default game language on start\n"
           "  --noiface                    Don't draw game GUI\n"
           "  --noscript                   Don't run room scripts; *WARNING:* unreliable\n"
           "  --nospr                      Don't draw room objects and characters\n"
           "  --noupdate                   Don't run game update\n"
           "  --novideo                    Don't play game videos\n"
           "  --rotation <MODE>            Screen rotation preferences. MODEs are:\n"
           "                                 unlocked (0), portrait (1), landscape (2)\n"
           "  --sdl-log=LEVEL              Setup SDL backend logging level\n"
           "                               LEVELs are:\n"
           "                                 verbose (1), debug (2), info (3), warn (4),\n"
           "                                 error (5), critical (6)\n"
#if AGS_PLATFORM_OS_WINDOWS
           "  --setup                      Run setup application\n"
#endif
           "  --shared-data-dir DIR        Set the shared game data directory\n"
           "  --startr <room_number>       Start game by loading certain room.\n"
           "  --tell                       Print various information concerning engine\n"
           "                                 and the game; for selected output use:\n"
           "  --tell-config                Print contents of merged game config\n"
           "  --tell-configpath            Print paths to available config files\n"
           "  --tell-data                  Print information on game data and its location\n"
           "  --tell-gameproperties        Print information on game general settings\n"
           "  --tell-engine                Print engine name and version\n"
           "  --tell-filepath              Print all filepaths engine uses for the game\n"
           "  --tell-graphicdriver         Print list of supported graphic drivers\n"
           "\n"
           "  --test                       Run game in the test mode\n"
           "  --translation <name>         Select the given translation on start\n"
           "  --version                    Print engine's version and stop\n"
           "  --user-data-dir DIR          Set the save game directory\n"
           "  --windowed                   Force display mode to windowed\n"
           "\n"
           "Gamefile options:\n"
           "  /dir/path/game/              Launch the game in specified directory\n"
           "  /dir/path/game/penguin.exe   Launch penguin.exe\n"
           "  [nothing]                    Launch the game in the current directory\n"
          //--------------------------------------------------------------------------------|
    );
}

static int main_process_cmdline(ConfigTree &cfg, int argc, char *argv[])
{
    int datafile_argv = 0;
    for (int ee = 1; ee < argc; ++ee)
    {
        const char *arg = argv[ee];
        //
        // Startup options
        //
        if (ags_stricmp(arg,"--help") == 0 || ags_stricmp(arg,"/?") == 0 || ags_stricmp(arg,"-?") == 0)
        {
            justDisplayHelp = true;
        }
        if (ags_stricmp(arg, "-v") == 0 || ags_stricmp(arg, "--version") == 0)
        {
            justDisplayVersion = true;
        }
        else if (ags_stricmp(arg,"--updatereg") == 0)
            debug_flags |= DBG_REGONLY;
        else if ((ags_stricmp(arg,"--startr") == 0) && (ee < argc-1)) {
            override_start_room = atoi(argv[ee+1]);
            ee++;
        }
        else if (ags_stricmp(arg,"--noexceptionhandler")==0) usetup.disable_exception_handling = true;
        else if (ags_stricmp(arg, "--setup") == 0)
        {
            justRunSetup = true;
        }
        else if ((ags_stricmp(arg,"--loadsavedgame") == 0) && (argc > ee + 1))
        {
            loadSaveGameOnStartup = argv[ee + 1];
            ee++;
        }
        else if ((ags_stricmp(arg,"--enabledebugger") == 0) && (argc > ee + 1))
        {
            snprintf(editor_debugger_instance_token, sizeof(editor_debugger_instance_token), "%s", argv[ee + 1]);
            editor_debugging_enabled = 1;
            cfg["graphics"]["windowed"] = "1";
            ee++;
        }
        else if (ags_stricmp(arg, "--conf") == 0 && (argc > ee + 1))
        {
            usetup.conf_path = argv[++ee];
        }
        else if (ags_stricmp(arg, "--localuserconf") == 0)
        {
            usetup.user_conf_dir = ".";
        }
        else if ((ags_stricmp(arg, "--user-conf-dir") == 0) && (argc > ee + 1))
        {
            usetup.user_conf_dir = argv[++ee];
        }
        else if (ags_stricmp(arg, "--runfromide") == 0 && (argc > ee + 4))
        {
            usetup.install_dir = argv[ee + 1];
            usetup.opt_data_dir = argv[ee + 2];
            usetup.opt_audio_dir = argv[ee + 3];
            usetup.opt_voice_dir = argv[ee + 4];
            ee += 4;
        }
        else if (ags_stricmp(arg, "--clear-cache-on-room-change") == 0)
            cfg["misc"]["clear_cache_on_room_change"] = "1";
        else if (ags_strnicmp(arg, "--tell", 6) == 0) {
            if (arg[6] == 0)
                tellInfoKeys.insert(String("all"));
            else if (arg[6] == '-' && arg[7] != 0)
                tellInfoKeys.insert(String(arg + 7));
        }
        //
        // Config overrides
        //
        else if ((ags_stricmp(arg, "--user-data-dir") == 0) && (argc > ee + 1))
            cfg["misc"]["user_data_dir"] = argv[++ee];
        else if ((ags_stricmp(arg, "--shared-data-dir") == 0) && (argc > ee + 1))
            cfg["misc"]["shared_data_dir"] = argv[++ee];
        else if (ags_stricmp(arg, "--windowed") == 0)
            cfg["graphics"]["windowed"] = "1";
        else if (ags_stricmp(arg, "--fullscreen") == 0)
            cfg["graphics"]["windowed"] = "0";
        else if ((ags_stricmp(arg, "--gfxdriver") == 0) && (argc > ee + 1))
            cfg["graphics"]["driver"] = argv[++ee];
        else if ((ags_stricmp(arg, "--gfxfilter") == 0) && (argc > ee + 1))
        {
            cfg["graphics"]["filter"] = argv[++ee];
            if (argc > ee + 1 && argv[ee + 1][0] != '-')
            {
                // NOTE: we make an assumption here that if user provides scaling
                // multiplier, then it's meant to be applied to windowed mode only;
                // Otherwise the scaling style is applied to both.
                String scale_value = argv[++ee];
                int scale_mul = StrUtil::StringToInt(scale_value);
                if (scale_mul > 0)
                {
                    cfg["graphics"]["window"] = String::FromFormat("x%d", scale_mul);
                    cfg["graphics"]["game_scale_win"] = "round";
                }
                else
                {
                    cfg["graphics"]["game_scale_fs"] = scale_value;
                    cfg["graphics"]["game_scale_win"] = scale_value;
                }
            }
        }
        else if ((ags_stricmp(arg, "--translation") == 0) && (argc > ee + 1))
            cfg["language"]["translation"] = argv[++ee];
        else if (ags_stricmp(arg, "--no-translation") == 0)
            cfg["language"]["translation"] = "";
        else if (ags_stricmp(arg, "--background") == 0)
            cfg["override"]["multitasking"] = "1";
        else if (ags_stricmp(arg, "--fps") == 0)
            cfg["misc"]["show_fps"] = "1";
        else if (ags_stricmp(arg, "--test") == 0) debug_flags |= DBG_DEBUGMODE;
        else if (ags_stricmp(arg, "--noiface") == 0) debug_flags |= DBG_NOIFACE;
        else if (ags_stricmp(arg, "--nosprdisp") == 0) debug_flags |= DBG_NODRAWSPRITES;
        else if (ags_stricmp(arg, "--nospr") == 0) debug_flags |= DBG_NOOBJECTS;
        else if (ags_stricmp(arg, "--noupdate") == 0) debug_flags |= DBG_NOUPDATE;
        else if (ags_stricmp(arg, "--nosound") == 0) debug_flags |= DBG_NOSFX;
        else if (ags_stricmp(arg, "--nomusic") == 0) debug_flags |= DBG_NOMUSIC;
        else if (ags_stricmp(arg, "--noscript") == 0) debug_flags |= DBG_NOSCRIPT;
        else if (ags_stricmp(arg, "--novideo") == 0) debug_flags |= DBG_NOVIDEO;
        else if (ags_stricmp(arg, "--rotation") == 0 && (argc > ee + 1))
            cfg["graphics"]["rotation"] = argv[++ee];
        else if (ags_strnicmp(arg, "--log-", 6) == 0 && arg[6] != 0)
        {
            String logarg = arg + 6;
            size_t split_at = logarg.FindChar('=');
            if (split_at != String::NoIndex)
                cfg["log"][logarg.Left(split_at)] = logarg.Mid(split_at + 1);
            else
                cfg["log"][logarg] = "";
        }
        else if (ags_strnicmp(arg, "--sdl-log", 9) == 0 && arg[9] == '=')
        {
            cfg["log"]["sdl"] = arg + 10;
        }
        else if (ags_stricmp(arg, "--console-attach") == 0) attachToParentConsole = true;
        else if (ags_stricmp(arg, "--no-message-box") == 0) hideMessageBoxes = true;
        else if (ags_stricmp(arg, "--print-rtti") == 0) logScriptRTTI = true;
        //
        // Special case: data file location
        //
        else if (arg[0]!='-') datafile_argv=ee;
    }

    if (datafile_argv > 0)
    {
        cmdGameDataPath = platform->GetCommandArg(datafile_argv);
    }

    if (tellInfoKeys.size() > 0)
        justTellInfo = true;

    return 0;
}

int ags_entry_point(int argc, char *argv[]) { 
    main_init(argc, argv);

#if AGS_PLATFORM_OS_WINDOWS && !AGS_PLATFORM_WINDOWS_MINGW
    setup_malloc_handling();
#endif
    debug_flags=0;
    // FIXME: this should be in game data init, but currently also set by cmdline
    play.takeover_data = 0;

    ConfigTree startup_opts;
    int res = main_process_cmdline(startup_opts, argc, argv);
    if (res != 0)
        return res;

    if (attachToParentConsole)
        platform->AttachToParentConsole();

    if (justDisplayVersion)
    {
        platform->WriteStdOut(get_engine_string().GetCStr());
        return EXIT_NORMAL;
    }

    if (justDisplayHelp)
    {
        main_print_help();
        return EXIT_NORMAL;
    }

    if (!justTellInfo && !hideMessageBoxes)
        platform->SetGUIMode(true);

    init_debug(startup_opts, justTellInfo);
    Debug::Printf(kDbgMsg_Alert, get_engine_string());

    appPath = Path::MakeAbsolutePath(platform->GetCommandArg(0));
    appDirectory = Path::GetDirectoryPath(appPath);

    // Update shell associations and exit
    if (debug_flags & DBG_REGONLY)
        exit(EXIT_NORMAL);

    int result = 0;
#ifdef USE_CUSTOM_EXCEPTION_HANDLER
    if (usetup.disable_exception_handling)
#endif
    {
        result = initialize_engine(startup_opts);
    }
#ifdef USE_CUSTOM_EXCEPTION_HANDLER
    else
    {
        result = initialize_engine_with_exception_handling(initialize_engine, startup_opts);
    }
#endif
    quit("|bye!");
    return result;
}
