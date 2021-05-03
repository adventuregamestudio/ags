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
// Entry point of the application here.
//
//
// For Windows main() function is really called _mangled_main and is called
// not by system, but from insides of allegro library.
// (See allegro\platform\alwin.h)
// What about other platforms?
//

#include "core/platform.h"
#define AGS_PLATFORM_DEFINES_PSP_VARS (AGS_PLATFORM_OS_IOS || AGS_PLATFORM_OS_ANDROID)
#include <set>
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
#include "main/mainheader.h"
#include "main/main.h"
#include "platform/base/agsplatformdriver.h"
#include "platform/base/sys_main.h"
#include "ac/route_finder.h"
#include "core/assetmanager.h"
#include "util/directory.h"
#include "util/path.h"
#include "util/string_compat.h"

#if AGS_PLATFORM_OS_WINDOWS
#include "platform/windows/win_ex_handling.h"
#endif
#if AGS_PLATFORM_DEBUG
#include "test/test_all.h"
#endif

#if AGS_PLATFORM_OS_WINDOWS && !AGS_PLATFORM_DEBUG
#define USE_CUSTOM_EXCEPTION_HANDLER
#endif

using namespace AGS::Common;
using namespace AGS::Engine;

String appPath; // engine exe path
String appDirectory; // engine dir
String cmdGameDataPath; // game path received from cmdline

char **global_argv = nullptr;
int    global_argc = 0;


extern GameSetup usetup;
extern GameState play;
extern int our_eip;
extern int convert_16bit_bgr; // CLNUP most likely remove
extern int editor_debugging_enabled;
extern int editor_debugging_initialized;
extern char editor_debugger_instance_token[100];


// Startup flags, set from parameters to engine
int force_window = 0;
int override_start_room = 0;
bool justDisplayHelp = false;
bool justDisplayVersion = false;
bool justRunSetup = false;
bool justRegisterGame = false;
bool justUnRegisterGame = false;
bool justTellInfo = false;
bool attachToParentConsole = false;
bool hideMessageBoxes = false;
std::set<String> tellInfoKeys;
const char *loadSaveGameOnStartup = nullptr;

#if ! AGS_PLATFORM_DEFINES_PSP_VARS
int psp_video_framedrop = 1;
int psp_ignore_acsetup_cfg_file = 0;
int psp_clear_cache_on_room_change = 0; // clear --sprite cache-- when room is unloaded

char psp_game_file_name[] = "";
char psp_translation[] = "default";

int psp_gfx_renderer = 0;
int psp_gfx_scaling = 1;
int psp_gfx_smoothing = 0;
int psp_gfx_super_sampling = 1;
int psp_gfx_smooth_sprites = 0;
#endif


void main_pre_init()
{
    our_eip = -999;
    AssetMgr->SetSearchPriority(Common::kAssetPriorityDir);
    play.takeover_data = 0;
}

void main_create_platform_driver()
{
    platform = AGSPlatformDriver::GetDriver();
}

// CLNUP check this stuff
// this needs to be updated if the "play" struct changes
#define SVG_VERSION_BWCOMPAT_MAJOR      3
#define SVG_VERSION_BWCOMPAT_MINOR      2
#define SVG_VERSION_BWCOMPAT_RELEASE    0
#define SVG_VERSION_BWCOMPAT_REVISION   1103
// CHECKME: we may lower this down, if we find that earlier versions may still
// load new savedgames
#define SVG_VERSION_FWCOMPAT_MAJOR      3
#define SVG_VERSION_FWCOMPAT_MINOR      2
#define SVG_VERSION_FWCOMPAT_RELEASE    1
#define SVG_VERSION_FWCOMPAT_REVISION   1111

// Current engine version
AGS::Common::Version EngineVersion;
// Lowest savedgame version, accepted by this engine
AGS::Common::Version SavedgameLowestBackwardCompatVersion;
// Lowest engine version, which would accept current savedgames
AGS::Common::Version SavedgameLowestForwardCompatVersion;

void main_init(int argc, char*argv[])
{
    EngineVersion = Version(ACI_VERSION_STR " " SPECIAL_VERSION);
#if defined (BUILD_STR)
    EngineVersion.BuildInfo = BUILD_STR;
#endif
    SavedgameLowestBackwardCompatVersion = Version(SVG_VERSION_BWCOMPAT_MAJOR, SVG_VERSION_BWCOMPAT_MINOR, SVG_VERSION_BWCOMPAT_RELEASE, SVG_VERSION_BWCOMPAT_REVISION);
    SavedgameLowestForwardCompatVersion = Version(SVG_VERSION_FWCOMPAT_MAJOR, SVG_VERSION_FWCOMPAT_MINOR, SVG_VERSION_FWCOMPAT_RELEASE, SVG_VERSION_FWCOMPAT_REVISION);

    AssetMgr.reset(new AssetManager());
    main_pre_init();
    main_create_platform_driver();
    platform->MainInitAdjustments();

    global_argv = argv;
    global_argc = argc;
}

String get_engine_string()
{
    return String::FromFormat("Adventure Game Studio v%s Interpreter\n"
        "Copyright (c) 1999-2011 Chris Jones and " ACI_COPYRIGHT_YEARS " others\n"
#ifdef BUILD_STR
        "ACI version %s (Build: %s)\n",
        EngineVersion.ShortString.GetCStr(), EngineVersion.LongString.GetCStr(), EngineVersion.BuildInfo.GetCStr());
#else
        "ACI version %s\n", EngineVersion.ShortString.GetCStr(), EngineVersion.LongString.GetCStr());
#endif
}

void main_print_help() {
    platform->WriteStdOut("%s",
        "Usage: ags [OPTIONS] [GAMEFILE or DIRECTORY]\n\n"
          //--------------------------------------------------------------------------------|
           "Options:\n"
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
           "                                 hqx, linear, none, stdscale\n"
           "                                 (support differs between graphic drivers);\n"
           "                                 scaling is specified by integer number\n"
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
           "  --noiface                    Don't draw game GUI\n"
           "  --noscript                   Don't run room scripts; *WARNING:* unreliable\n"
           "  --nospr                      Don't draw room objects and characters\n"
           "  --noupdate                   Don't run game update\n"
           "  --novideo                    Don't play game videos\n"
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
        else if (ags_stricmp(arg,"--registergame") == 0)
        {
            justRegisterGame = true;
        }
        else if (ags_stricmp(arg,"--unregistergame") == 0)
        {
            justUnRegisterGame = true;
        }
        else if ((ags_stricmp(arg,"--loadsavedgame") == 0) && (argc > ee + 1))
        {
            loadSaveGameOnStartup = argv[ee + 1];
            ee++;
        }
        else if ((ags_stricmp(arg,"--enabledebugger") == 0) && (argc > ee + 1))
        {
            strcpy(editor_debugger_instance_token, argv[ee + 1]);
            editor_debugging_enabled = 1;
            force_window = 1;
            ee++;
        }
        else if (ags_stricmp(arg, "--conf") == 0 && (argc > ee + 1))
        {
            usetup.conf_path = argv[++ee];
        }
        else if (ags_stricmp(arg, "--localuserconf") == 0)
        {
            usetup.local_user_conf = true;
        }
        else if (ags_stricmp(arg, "--runfromide") == 0 && (argc > ee + 4))
        {
            usetup.install_dir = argv[ee + 1];
            usetup.opt_data_dir = argv[ee + 2];
            usetup.opt_audio_dir = argv[ee + 3];
            usetup.opt_voice_dir = argv[ee + 4];
            ee += 4;
        }
        else if (ags_stricmp(arg,"--takeover")==0) {
            if (argc < ee+2)
                break;
            play.takeover_data = atoi (argv[ee + 1]);
            strncpy (play.takeover_from, argv[ee + 2], 49);
            play.takeover_from[49] = 0;
            ee += 2;
        }
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
            force_window = 1;
        else if (ags_stricmp(arg, "--fullscreen") == 0)
            force_window = 2;
        else if ((ags_stricmp(arg, "--gfxdriver") == 0) && (argc > ee + 1))
        {
            INIwritestring(cfg, "graphics", "driver", argv[++ee]);
        }
        else if ((ags_stricmp(arg, "--gfxfilter") == 0) && (argc > ee + 1))
        {
            // NOTE: we make an assumption here that if user provides scaling factor,
            // this factor means to be applied to windowed mode only.
            INIwritestring(cfg, "graphics", "filter", argv[++ee]);
            if (argc > ee + 1 && argv[ee + 1][0] != '-')
                INIwritestring(cfg, "graphics", "game_scale_win", argv[++ee]);
            else
                INIwritestring(cfg, "graphics", "game_scale_win", "max_round");
        }
        else if (ags_stricmp(arg, "--fps") == 0) display_fps = kFPS_Forced;
        else if (ags_stricmp(arg, "--test") == 0) debug_flags |= DBG_DEBUGMODE;
        else if (ags_stricmp(arg, "--noiface") == 0) debug_flags |= DBG_NOIFACE;
        else if (ags_stricmp(arg, "--nosprdisp") == 0) debug_flags |= DBG_NODRAWSPRITES;
        else if (ags_stricmp(arg, "--nospr") == 0) debug_flags |= DBG_NOOBJECTS;
        else if (ags_stricmp(arg, "--noupdate") == 0) debug_flags |= DBG_NOUPDATE;
        else if (ags_stricmp(arg, "--nosound") == 0) debug_flags |= DBG_NOSFX;
        else if (ags_stricmp(arg, "--nomusic") == 0) debug_flags |= DBG_NOMUSIC;
        else if (ags_stricmp(arg, "--noscript") == 0) debug_flags |= DBG_NOSCRIPT;
        else if (ags_stricmp(arg, "--novideo") == 0) debug_flags |= DBG_NOVIDEO;
        else if (ags_strnicmp(arg, "--log-", 6) == 0 && arg[6] != 0)
        {
            String logarg = arg + 6;
            size_t split_at = logarg.FindChar('=');
            if (split_at >= 0)
                cfg["log"][logarg.Left(split_at)] = logarg.Mid(split_at + 1);
            else
                cfg["log"][logarg] = "";
        }
        else if (ags_stricmp(arg, "--console-attach") == 0) attachToParentConsole = true;
        else if (ags_stricmp(arg, "--no-message-box") == 0) hideMessageBoxes = true;
        //
        // Special case: data file location
        //
        else if (arg[0]!='-') datafile_argv=ee;
    }

    if (datafile_argv > 0)
    {
        cmdGameDataPath = GetPathFromCmdArg(datafile_argv);
    }
    else
    {
        // assign standard path for mobile/consoles (defined in their own platform implementation)
        cmdGameDataPath = psp_game_file_name;
    }

    if (tellInfoKeys.size() > 0)
        justTellInfo = true;

    return 0;
}

void main_set_gamedir(int argc, char*argv[])
{
    appPath = GetPathFromCmdArg(0);
    appDirectory = Path::GetDirectoryPath(appPath);

    // TODO: remove following when supporting unicode paths
    {
        // It looks like Allegro library does not like ANSI (ACP) paths.
        // When *not* working in U_UNICODE filepath mode, whenever it gets
        // current directory for its own operations, it "fixes" it by
        // substituting non-ASCII symbols with '^'.
        // Here we explicitly set current directory to ASCII path.
        String cur_dir = Directory::GetCurrentDirectory();
        String path = Path::GetPathInASCII(cur_dir);
        if (!path.IsEmpty())
            Directory::SetCurrentDirectory(Path::MakeAbsolutePath(path));
        else
            Debug::Printf(kDbgMsg_Error, "Unable to determine current directory: GetPathInASCII failed.\nArg: %s", cur_dir.GetCStr());
    }
}

String GetPathFromCmdArg(int arg_index)
{
    if (arg_index < 0 || arg_index >= global_argc)
        return "";
    String path = Path::GetCmdLinePathInASCII(global_argv[arg_index], arg_index);
    if (!path.IsEmpty())
        return Path::MakeAbsolutePath(path);
    Debug::Printf(kDbgMsg_Error, "Unable to determine path: GetCmdLinePathInASCII failed.\nCommand line argument %i: %s", arg_index, global_argv[arg_index]);
    return global_argv[arg_index];
}

int ags_entry_point(int argc, char *argv[]) { 

#ifdef AGS_RUN_TESTS
    Test_DoAllTests();
#endif
    main_init(argc, argv);

#if AGS_PLATFORM_OS_WINDOWS
    setup_malloc_handling();
#endif
    debug_flags=0;

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

    main_set_gamedir(argc, argv);    

    // Update shell associations and exit
    if (debug_flags & DBG_REGONLY)
        exit(EXIT_NORMAL);

#ifdef USE_CUSTOM_EXCEPTION_HANDLER
    if (usetup.disable_exception_handling)
#endif
    {
        int result = initialize_engine(startup_opts);
        // TODO: refactor engine shutdown routine (must shutdown and delete everything started and created)
        sys_main_shutdown();
        allegro_exit();
        platform->PostBackendExit();
        return result;
    }
#ifdef USE_CUSTOM_EXCEPTION_HANDLER
    else
    {
        return initialize_engine_with_exception_handling(initialize_engine, startup_opts);
    }
#endif
}
