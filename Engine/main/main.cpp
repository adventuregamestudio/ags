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
#include "util/cmdlineopts.h"
#include "main/engine_cmdline.h"

#if AGS_PLATFORM_OS_WINDOWS
#include "platform/windows/win_ex_handling.h"
#endif

#if AGS_PLATFORM_OS_WINDOWS && !AGS_PLATFORM_DEBUG
#define USE_CUSTOM_EXCEPTION_HANDLER
#endif

using namespace AGS::Common;
using namespace AGS::Common::CmdLineOpts;
using namespace AGS::Engine;
using namespace AGS::Engine::CmdLineOpts;

String appPath; // engine exe path
String appDirectory; // engine dir
String cmdGameDataPath; // game path received from cmdline

extern GameState play;
extern int our_eip;
extern int editor_debugging_enabled;
extern int editor_debugging_initialized;
extern char editor_debugger_instance_token[100];


// Startup flags, set from parameters to engine
//int override_start_room = 0;
//bool justDisplayHelp = false;
//bool justDisplayVersion = false;
//bool justRunSetup = false;
bool justTellInfo = false;
bool hideMessageBoxes = false;
std::set<String> tellInfoKeys;
String loadSaveGameOnStartup;

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
    our_eip = -999;

    // Init libraries: set text encoding
    set_uformat(U_UTF8);
    set_filename_encoding(U_UNICODE);

    EngineVersion = Version(ACI_VERSION_STR " " SPECIAL_VERSION);
#if defined (BUILD_STR)
    EngineVersion.BuildInfo = BUILD_STR;
#endif
    SavedgameLowestBackwardCompatVersion = Version(SVG_VERSION_BWCOMPAT_MAJOR, SVG_VERSION_BWCOMPAT_MINOR, SVG_VERSION_BWCOMPAT_RELEASE, SVG_VERSION_BWCOMPAT_REVISION);
    SavedgameLowestForwardCompatVersion = Version(SVG_VERSION_FWCOMPAT_MAJOR, SVG_VERSION_FWCOMPAT_MINOR, SVG_VERSION_FWCOMPAT_RELEASE, SVG_VERSION_FWCOMPAT_REVISION);

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
#ifdef BUILD_STR
        "ACI version %s (Build: %s)\n",
        EngineVersion.ShortString.GetCStr(), EngineVersion.LongString.GetCStr(), EngineVersion.BuildInfo.GetCStr());
#else
        "ACI version %s\n", EngineVersion.ShortString.GetCStr(), EngineVersion.LongString.GetCStr());
#endif
}

void main_print_help() {
    platform->WriteStdOut("%s", EngineCmdLineOpts::GetHelpText());
}

static int main_process_cmdline(EngineParsedOptions& options, ParseResult& cmdLineOpts, int argc, char *argv[])
{

    // Options that are just values (position-based arguments)

    // Path to the game data file
    if (!cmdLineOpts.PosArgs.empty()) {
        options.CmdGameDataPath = cmdLineOpts.PosArgs.front();
    }
    

    // Options that are just flags

    if (cmdLineOpts.HelpRequested)
    {
        options.JustDisplayHelp = true;
        return 0;
    }

    if (cmdLineOpts.Opt.count("-v") || cmdLineOpts.Opt.count("--version")) {
        options.JustDisplayVersion = true;
        return 0;
    }

    if (cmdLineOpts.Opt.count("--noexceptionhandler")) {
        options.UserSetup_DisableExceptionHandling = true;
    }

    if (cmdLineOpts.Opt.count("--setup")) {
        options.JustRunSetup = true;
    }

    if (cmdLineOpts.Opt.count("--localuserconf")) {
        options.UserSetup_LocalUserConfPath = ".";
    }

    if (cmdLineOpts.Opt.count("--clear-cache-on-room-change")) {
        options.cfg["misc"]["clear_cache_on_room_change"] = "1";
    }

    if (cmdLineOpts.Opt.count("--windowed")) {
        options.cfg["graphics"]["windowed"] = "1";
    }

    if (cmdLineOpts.Opt.count("--fullscreen")) {
        options.cfg["graphics"]["windowed"] = "0";
    }

    if (cmdLineOpts.Opt.count("--no-translation")) {
        options.cfg["language"]["translation"] = "";
    }

    if (cmdLineOpts.Opt.count("--background")) {
        options.cfg["override"]["multitasking"] = "1";
    }

    if (cmdLineOpts.Opt.count("--fps")) {
        options.cfg["misc"]["show_fps"] = "1";
    }

    if (cmdLineOpts.Opt.count("--console-attach")) {
        options.AttachToParentConsole = true;
    }

    if (cmdLineOpts.Opt.count("--no-message-box")) {
        options.HideMessageBoxes = true;
    }


    // Debug flags

    if (cmdLineOpts.Opt.count("--updatereg")) {
        options.DebugFlags |= DBG_REGONLY;
    }

    if (cmdLineOpts.Opt.count("--test")) {
        options.DebugFlags |= DBG_DEBUGMODE;
    }

    if (cmdLineOpts.Opt.count("--noiface")) {
        options.DebugFlags |= DBG_NOIFACE;
    }

    if (cmdLineOpts.Opt.count("--nosprdisp")) {
        options.DebugFlags |= DBG_NODRAWSPRITES;
    }

    if (cmdLineOpts.Opt.count("--nospr")) {
        options.DebugFlags |= DBG_NOOBJECTS;
    }

    if (cmdLineOpts.Opt.count("--noupdate")) {
        options.DebugFlags |= DBG_NOUPDATE;
    }

    if (cmdLineOpts.Opt.count("--nosound")) {
        options.DebugFlags |= DBG_NOSFX;
    }

    if (cmdLineOpts.Opt.count("--nomusic")) {
        options.DebugFlags |= DBG_NOMUSIC;
    }

    if (cmdLineOpts.Opt.count("--noscript")) {
        options.DebugFlags |= DBG_NOSCRIPT;
    }

    if (cmdLineOpts.Opt.count("--novideo")) {
        options.DebugFlags |= DBG_NOVIDEO;
    }

    // "--tell"

    if (cmdLineOpts.Opt.count("--tell")) {
        options.TellInfoKeys.insert("all");
    }
    if (cmdLineOpts.Opt.count("--tell-config")) {
        options.TellInfoKeys.insert("config");
    }
    if (cmdLineOpts.Opt.count("--tell-configpath")) {
        options.TellInfoKeys.insert("configpath");
    }
    if (cmdLineOpts.Opt.count("--tell-data")) {
        options.TellInfoKeys.insert("data");
    }
    if (cmdLineOpts.Opt.count("--tell-gameproperties")) {
        options.TellInfoKeys.insert("gameproperties");
    }
    if (cmdLineOpts.Opt.count("--tell-engine")) {
        options.TellInfoKeys.insert("engine");
    }
    if (cmdLineOpts.Opt.count("--tell-filepath")) {
        options.TellInfoKeys.insert("filepath");
    }
    if (cmdLineOpts.Opt.count("--tell-graphicdriver")) {
        options.TellInfoKeys.insert("graphicdriver");
    }




    //Options that take one or more arguments

    for (const auto& opt_with_value : cmdLineOpts.OptWith1Value)
    {
        //Option to override game's starting room (number)
        if (opt_with_value.first == "--startr")
        {
            std::string startRoom_str = opt_with_value.second.GetCStr();
            options.OverrideStartRoom = atoi(startRoom_str.c_str());
        }

        //Option to choose saved game to load automatically (string)
        if (opt_with_value.first == "--loadsavedgame")
        {
            options.LoadSaveGameOnStartup = String(opt_with_value.second.GetCStr());
        }

        //Enable debugger (string, which defines the debugger instance token) (sets a boolean too)
        if (opt_with_value.first == "--enabledebugger")
        {
            options.EditorDebuggerInstanceToken = String(opt_with_value.second.GetCStr());
            options.EditorDebuggingEnabled = true;
            options.cfg["graphics"]["windowed"] = "1";
        }

        // Configuration path (string)
        if (opt_with_value.first == "--conf")
        {
            options.UserSetup_ConfPath = String(opt_with_value.second.GetCStr());
        }

        // User conf dir (string)
        if (opt_with_value.first == "--user-conf-dir")
        {
            options.UserSetup_UserConfPath = String(opt_with_value.second.GetCStr());
        }

        // Run From IDE / Take over (4 strings representing paths)
        if (opt_with_value.first == "--runfromide")
        {

            // TODO

            //else if (ags_stricmp(arg, "--runfromide") == 0 && (argc > ee + 4))
            //{
            //    usetup.install_dir = argv[ee + 1];
            //    usetup.opt_data_dir = argv[ee + 2];
            //    usetup.opt_audio_dir = argv[ee + 3];
            //    usetup.opt_voice_dir = argv[ee + 4];
            //    ee += 4;
            //}

        }



        
    }

    int datafile_argv = 0;
    for (int ee = 1; ee < argc; ++ee)
    {
        const char *arg = argv[ee];
        //
        // Startup options
        //

        //if (ags_stricmp(arg, "--help") == 0 || ags_stricmp(arg, "/?") == 0 || ags_stricmp(arg, "-?") == 0)
        //{
        //    options.JustDisplayHelp = true;
        //}

        //if (ags_stricmp(arg, "-v") == 0 || ags_stricmp(arg, "--version") == 0)
        //{
        //    options.JustDisplayVersion = true;
        //}

        //else if (ags_stricmp(arg,"--updatereg") == 0)
            //debug_flags |= DBG_REGONLY;
        //else if ((ags_stricmp(arg,"--startr") == 0) && (ee < argc-1)) {
        //    override_start_room = atoi(argv[ee+1]);
        //    ee++;
        //}
        //else if (ags_stricmp(arg,"--noexceptionhandler")==0) usetup.disable_exception_handling = true;
        //else if (ags_stricmp(arg, "--setup") == 0)
        //{
        //    justRunSetup = true;
        //}
        //else if ((ags_stricmp(arg,"--loadsavedgame") == 0) && (argc > ee + 1))
        //{
        //    loadSaveGameOnStartup = argv[ee + 1];
        //    ee++;
        //}
        //else if ((ags_stricmp(arg,"--enabledebugger") == 0) && (argc > ee + 1))
        //{
        //    snprintf(editor_debugger_instance_token, sizeof(editor_debugger_instance_token), "%s", argv[ee + 1]);
        //    editor_debugging_enabled = 1;
        //    options.cfg["graphics"]["windowed"] = "1";
        //    ee++;
        //}
        //else if (ags_stricmp(arg, "--conf") == 0 && (argc > ee + 1))
        //{
        //    usetup.conf_path = argv[++ee];
        //}
        //else if (ags_stricmp(arg, "--localuserconf") == 0)
        //{
        //    usetup.user_conf_dir = ".";
        //}
        //else if ((ags_stricmp(arg, "--user-conf-dir") == 0) && (argc > ee + 1))
        //{
        //    usetup.user_conf_dir = argv[++ee];
        //}
        //else if (ags_stricmp(arg, "--runfromide") == 0 && (argc > ee + 4))
        //{
        //    usetup.install_dir = argv[ee + 1];
        //    usetup.opt_data_dir = argv[ee + 2];
        //    usetup.opt_audio_dir = argv[ee + 3];
        //    usetup.opt_voice_dir = argv[ee + 4];
        //    ee += 4;
        //}
        if (ags_stricmp(arg,"--takeover")==0) {
            if (argc < ee+2)
                break;
            play.takeover_data = atoi (argv[ee + 1]);
            strncpy (play.takeover_from, argv[ee + 2], 49);
            play.takeover_from[49] = 0;
            ee += 2;
        }
        //else if (ags_stricmp(arg, "--clear-cache-on-room-change") == 0)
        //    options.cfg["misc"]["clear_cache_on_room_change"] = "1";
        //else if (ags_strnicmp(arg, "--tell", 6) == 0) {
        //    if (arg[6] == 0)
        //        tellInfoKeys.insert(String("all"));
        //    else if (arg[6] == '-' && arg[7] != 0)
        //        tellInfoKeys.insert(String(arg + 7));
        //}
        //
        // Config overrides
        //
        else if ((ags_stricmp(arg, "--user-data-dir") == 0) && (argc > ee + 1))
            options.cfg["misc"]["user_data_dir"] = argv[++ee];
        else if ((ags_stricmp(arg, "--shared-data-dir") == 0) && (argc > ee + 1))
            options.cfg["misc"]["shared_data_dir"] = argv[++ee];
        //else if (ags_stricmp(arg, "--windowed") == 0)
        //    options.cfg["graphics"]["windowed"] = "1";
        //else if (ags_stricmp(arg, "--fullscreen") == 0)
        //    options.cfg["graphics"]["windowed"] = "0";
        else if ((ags_stricmp(arg, "--gfxdriver") == 0) && (argc > ee + 1))
            options.cfg["graphics"]["driver"] = argv[++ee];
        else if ((ags_stricmp(arg, "--gfxfilter") == 0) && (argc > ee + 1))
        {
            // NOTE: we make an assumption here that if user provides scaling factor,
            // this factor means to be applied to windowed mode only.
            options.cfg["graphics"]["filter"] = argv[++ee];
            if (argc > ee + 1 && argv[ee + 1][0] != '-')
                options.cfg["graphics"]["game_scale_win"] = argv[++ee];
            else
                options.cfg["graphics"]["game_scale_win"] = "max_round";
        }
        else if ((ags_stricmp(arg, "--translation") == 0) && (argc > ee + 1))
            options.cfg["language"]["translation"] = argv[++ee];
        //else if (ags_stricmp(arg, "--no-translation") == 0)
        //    options.cfg["language"]["translation"] = "";
        //else if (ags_stricmp(arg, "--background") == 0)
        //    options.cfg["override"]["multitasking"] = "1";
        //else if (ags_stricmp(arg, "--fps") == 0)
        //    options.cfg["misc"]["show_fps"] = "1";
        //else if (ags_stricmp(arg, "--test") == 0) debug_flags |= DBG_DEBUGMODE;
        //else if (ags_stricmp(arg, "--noiface") == 0) debug_flags |= DBG_NOIFACE;
        //else if (ags_stricmp(arg, "--nosprdisp") == 0) debug_flags |= DBG_NODRAWSPRITES;
        //else if (ags_stricmp(arg, "--nospr") == 0) debug_flags |= DBG_NOOBJECTS;
        //else if (ags_stricmp(arg, "--noupdate") == 0) debug_flags |= DBG_NOUPDATE;
        //else if (ags_stricmp(arg, "--nosound") == 0) debug_flags |= DBG_NOSFX;
        //else if (ags_stricmp(arg, "--nomusic") == 0) debug_flags |= DBG_NOMUSIC;
        //else if (ags_stricmp(arg, "--noscript") == 0) debug_flags |= DBG_NOSCRIPT;
        //else if (ags_stricmp(arg, "--novideo") == 0) debug_flags |= DBG_NOVIDEO;
        else if (ags_stricmp(arg, "--rotation") == 0 && (argc > ee + 1))
            options.cfg["graphics"]["rotation"] = argv[++ee];
        else if (ags_strnicmp(arg, "--log-", 6) == 0 && arg[6] != 0)
        {
            String logarg = arg + 6;
            size_t split_at = logarg.FindChar('=');
            if (split_at != String::NoIndex)
                options.cfg["log"][logarg.Left(split_at)] = logarg.Mid(split_at + 1);
            else
                options.cfg["log"][logarg] = "";
        }
        else if (ags_strnicmp(arg, "--sdl-log", 9) == 0 && arg[9] == '=')
        {
            options.cfg["log"]["sdl"] = arg + 10;
        }
        //else if (ags_stricmp(arg, "--console-attach") == 0) attachToParentConsole = true;
        //else if (ags_stricmp(arg, "--no-message-box") == 0) hideMessageBoxes = true;
        //
        // Special case: data file location
        //
        else if (arg[0]!='-') datafile_argv=ee;
    }

    //if (datafile_argv > 0)
    //{
    //    cmdGameDataPath = platform->GetCommandArg(datafile_argv);
    //}

    if (tellInfoKeys.size() > 0)
        justTellInfo = true;

    return 0;
}

void main_set_gamedir()
{
    appPath = Path::MakeAbsolutePath(platform->GetCommandArg(0));
    appDirectory = Path::GetDirectoryPath(appPath);
}

//FIXME : We pulled out those options from the old command line processing function (where they were assigned as soon as read!),
//       but it's still not ideal that they're processed like that, via global variables.
//       The values and assignments should be trickled down to relevant code locations, all in good time.
void main_process_early_options(const EngineParsedOptions& engineOptions)
{
    debug_flags = engineOptions.DebugFlags;
    if (!engineOptions.LoadSaveGameOnStartup.IsEmpty()) { loadSaveGameOnStartup = engineOptions.LoadSaveGameOnStartup; }
    if (!engineOptions.EditorDebuggerInstanceToken.IsEmpty()) { snprintf(editor_debugger_instance_token, sizeof(editor_debugger_instance_token), "%s", engineOptions.EditorDebuggerInstanceToken.GetCStr()); }
    if (engineOptions.EditorDebuggingEnabled) { editor_debugging_enabled = 1; }
    tellInfoKeys.insert(engineOptions.TellInfoKeys.begin(), engineOptions.TellInfoKeys.end());
    hideMessageBoxes = engineOptions.HideMessageBoxes;

    usetup.disable_exception_handling = engineOptions.UserSetup_DisableExceptionHandling;
    if (!engineOptions.UserSetup_ConfPath.IsEmpty()) { usetup.conf_path = engineOptions.UserSetup_ConfPath; }
    if (!engineOptions.UserSetup_LocalUserConfPath.IsEmpty()) { usetup.user_conf_dir = engineOptions.UserSetup_LocalUserConfPath; }
    if (!engineOptions.UserSetup_UserConfPath.IsEmpty()) { usetup.user_conf_dir = engineOptions.UserSetup_UserConfPath; }

    cmdGameDataPath = engineOptions.CmdGameDataPath;
}

int ags_entry_point(int argc, char *argv[])
{
    main_init(argc, argv);

#if AGS_PLATFORM_OS_WINDOWS
    setup_malloc_handling();
#endif
    debug_flags=0;
    // FIXME: this should be in game data init, but currently also set by cmdline
    play.takeover_data = 0;

    auto cmdLineRawOptions = CmdLineOptsParser::Parse(argc, argv,
        // Options that always take exactly 1 argument
        {
            
            "--startr",
            "--loadsavedgame",
            "--enabledebugger",
            "--conf",
            "--user-conf-dir",
        },
        // Options that always take exactly 2 arguments
        {
        },
        // Options that always take exactly 3 arguments
        {
        },
        // Options that always take exactly 4 arguments
        {
            "--runfromide"
        }
    );

    if (cmdLineRawOptions.IsBadlyFormed) {
        platform->WriteStdErr("Command line parameters could not be parsed because of badly formed option '%s'", cmdLineRawOptions.BadlyFormedOption.GetCStr());
        return EXIT_FAILURE;
    }

    EngineParsedOptions engineOptions;
    EngineCmdLineOpts::Convert(engineOptions, cmdLineRawOptions);

    int res = main_process_cmdline(engineOptions, cmdLineRawOptions, argc, argv);

    if (engineOptions.CmdGameDataPath.IsEmpty()) {
        platform->WriteStdErr("No path to game data file was provided.");
    }

    //TODO: Process those early options only when needed instead of shoehorning it right there.
    main_process_early_options(engineOptions);

    if (res != 0)
        return res;

    if (engineOptions.AttachToParentConsole)
        platform->AttachToParentConsole();

    //if (justDisplayVersion)
    //{
    //    platform->WriteStdOut(get_engine_string().GetCStr());
    //    return EXIT_NORMAL;
    //}
    if (engineOptions.JustDisplayVersion) {
        platform->WriteStdOut(get_engine_string().GetCStr());
        return EXIT_NORMAL;
    }

    //if (justDisplayHelp)
    //{
    //    main_print_help();
    //    return EXIT_NORMAL;
    //}
    if (engineOptions.JustDisplayHelp) {
        main_print_help();
        return EXIT_NORMAL;
    }

    if (!justTellInfo && !hideMessageBoxes)
        platform->SetGUIMode(true);

    init_debug(engineOptions.cfg, justTellInfo);

    Debug::Printf(kDbgMsg_Alert, get_engine_string());

    main_set_gamedir();

    // Update shell associations and exit
    if (debug_flags & DBG_REGONLY)
        exit(EXIT_NORMAL);

    int result = 0;
#ifdef USE_CUSTOM_EXCEPTION_HANDLER
    if (usetup.disable_exception_handling)
#endif
    {
        result = initialize_engine(engineOptions);
    }
#ifdef USE_CUSTOM_EXCEPTION_HANDLER
    else
    {
        result = initialize_engine_with_exception_handling(initialize_engine, engineOptions);
    }
#endif
    quit("|bye!");
    return result;
}
