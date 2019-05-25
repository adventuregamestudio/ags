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

#include "ac/common.h"
#include "ac/gamesetup.h"
#include "ac/gamestate.h"
#include "core/def_version.h"
#include "debug/agseditordebugger.h"
#include "debug/debug_log.h"
#include "debug/out.h"
#include "main/config.h"
#include "main/engine.h"
#include "main/mainheader.h"
#include "main/main.h"
#include "platform/base/agsplatformdriver.h"
#include "ac/route_finder.h"
#include "core/assetmanager.h"
#include "util/directory.h"
#include "util/path.h"

#ifdef _DEBUG
#include "test/test_all.h"
#endif

using namespace AGS::Common;
using namespace AGS::Engine;

String appDirectory; // Needed for library loading
String cmdGameDataPath;

#ifdef MAC_VERSION
extern "C"
{
    int osx_sys_question(const char *msg, const char *but1, const char *but2);
}
#endif


#ifdef WINDOWS_VERSION

int wArgc;
LPWSTR *wArgv;

#endif

char **global_argv = nullptr;
int    global_argc = 0;


extern GameSetup usetup;
extern GameState play;
extern int our_eip;
extern AGSPlatformDriver *platform;
extern int debug_flags;
extern int convert_16bit_bgr; // CLNUP most likely remove
extern int display_fps;
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
const char *loadSaveGameOnStartup = nullptr;

#if !defined(MAC_VERSION) && !defined(IOS_VERSION) && !defined(PSP_VERSION) && !defined(ANDROID_VERSION)
int psp_video_framedrop = 1;
int psp_audio_enabled = 1;
int psp_midi_enabled = 1;
int psp_ignore_acsetup_cfg_file = 0;
int psp_clear_cache_on_room_change = 0;

int psp_midi_preload_patches = 0;
int psp_audio_cachesize = 10;
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
    Common::AssetManager::SetSearchPriority(Common::kAssetPriorityDir);
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

void main_init()
{
    EngineVersion = Version(ACI_VERSION_STR " " SPECIAL_VERSION);
#if defined (BUILD_STR)
    EngineVersion.BuildInfo = BUILD_STR;
#endif
    SavedgameLowestBackwardCompatVersion = Version(SVG_VERSION_BWCOMPAT_MAJOR, SVG_VERSION_BWCOMPAT_MINOR, SVG_VERSION_BWCOMPAT_RELEASE, SVG_VERSION_BWCOMPAT_REVISION);
    SavedgameLowestForwardCompatVersion = Version(SVG_VERSION_FWCOMPAT_MAJOR, SVG_VERSION_FWCOMPAT_MINOR, SVG_VERSION_FWCOMPAT_RELEASE, SVG_VERSION_FWCOMPAT_REVISION);

    Common::AssetManager::CreateInstance();
    main_pre_init();
    main_create_platform_driver();
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

int main_preprocess_cmdline(int argc,char*argv[])
{
#ifdef WINDOWS_VERSION
    wArgv = CommandLineToArgvW(GetCommandLineW(), &wArgc);
    if (wArgv == NULL)
    {
        platform->DisplayAlert("CommandLineToArgvW failed, unable to start the game.");
        return 9;
    }
#endif
    global_argv = argv;
    global_argc = argc;
    return RETURN_CONTINUE;
}

extern char return_to_roomedit[30];
extern char return_to_room[150];

void main_print_help() {
    platform->WriteStdOut(
           "Usage: ags [OPTIONS] [GAMEFILE or DIRECTORY]\n\n"
           "Options:\n"
           "  --windowed                   Force display mode to windowed\n"
           "  --fullscreen                 Force display mode to fullscreen\n"
           "  --hicolor                    Downmix 32bit colors to 16bit\n"
           "  --gfxfilter <filter> [<scaling>]\n"
           "                               Request graphics filter. Available options:\n"
           "                                 none, stdscale, hqx;\n"
           "                                 scaling is specified by integer number\n"
           "  --log                        Enable program output to the log file\n"
           "  --no-log                     Disable program output to the log file,\n"
           "                                 overriding configuration file setting\n"
           "  --help                       Print this help message\n"
           "\n"
           "Gamefile options:\n"
           "  /dir/path/game/              Launch the game in specified directory\n"
           "  /dir/path/game/penguin.exe   Launch penguin.exe\n"
           "  [nothing]                    Launch the game in the current directory\n"
    );
}

int main_process_cmdline(int argc,char*argv[])
{
    int datafile_argv = 0;
    for (int ee=1;ee<argc;ee++) {
        if (stricmp(argv[ee],"--help") == 0 || stricmp(argv[ee],"/?") == 0 || stricmp(argv[ee],"-?") == 0)
        {
            justDisplayHelp = true;
            return RETURN_CONTINUE;
        }
        if (stricmp(argv[ee],"-v") == 0 || stricmp(argv[ee],"--version") == 0)
            justDisplayVersion = true;
        else if (stricmp(argv[ee],"-updatereg") == 0)
            debug_flags |= DBG_REGONLY;
        else if (stricmp(argv[ee],"-windowed") == 0 || stricmp(argv[ee],"--windowed") == 0)
            force_window = 1;
        else if (stricmp(argv[ee],"-fullscreen") == 0 || stricmp(argv[ee],"--fullscreen") == 0)
            force_window = 2;
        else if ((stricmp(argv[ee],"-gfxfilter") == 0 || stricmp(argv[ee],"--gfxfilter") == 0) && (argc > ee + 1))
        {
            // TODO: we make an assumption here that if user provides scaling factor,
            // this factor means to be applied to windowed mode only.
            usetup.Screen.Filter.ID = argv[++ee];
            if (argc > ee + 1 && argv[ee + 1][0] != '-')
            {
                parse_scaling_option(argv[++ee], usetup.Screen.WinGameFrame);
            }
            else
            {
                usetup.Screen.WinGameFrame.ScaleDef = kFrame_MaxRound;
            }
            
        }
#ifdef _DEBUG
        else if ((stricmp(argv[ee],"--startr") == 0) && (ee < argc-1)) {
            override_start_room = atoi(argv[ee+1]);
            ee++;
        }
#endif
        else if ((stricmp(argv[ee],"--testre") == 0) && (ee < argc-2)) {
            strcpy(return_to_roomedit, argv[ee+1]);
            strcpy(return_to_room, argv[ee+2]);
            ee+=2;
        }
		// CLNUP most likely remove xxbit_mode flags
        else if (stricmp(argv[ee],"--fps")==0) display_fps = 2;
        else if (stricmp(argv[ee],"--test")==0) debug_flags|=DBG_DEBUGMODE;
        else if (stricmp(argv[ee],"-noiface")==0) debug_flags|=DBG_NOIFACE;
        else if (stricmp(argv[ee],"-nosprdisp")==0) debug_flags|=DBG_NODRAWSPRITES;
        else if (stricmp(argv[ee],"-nospr")==0) debug_flags|=DBG_NOOBJECTS;
        else if (stricmp(argv[ee],"-noupdate")==0) debug_flags|=DBG_NOUPDATE;
        else if (stricmp(argv[ee],"-nosound")==0) debug_flags|=DBG_NOSFX;
        else if (stricmp(argv[ee],"-nomusic")==0) debug_flags|=DBG_NOMUSIC;
        else if (stricmp(argv[ee],"-noscript")==0) debug_flags|=DBG_NOSCRIPT;
        else if (stricmp(argv[ee],"-novideo")==0) debug_flags|=DBG_NOVIDEO;
        else if (stricmp(argv[ee],"-noexceptionhandler")==0) usetup.disable_exception_handling = true;
        else if (stricmp(argv[ee],"-dbgscript")==0) debug_flags|=DBG_DBGSCRIPT;
        else if (stricmp(argv[ee], "--setup") == 0)
        {
            justRunSetup = true;
        }
        else if (stricmp(argv[ee],"-registergame") == 0)
        {
            justRegisterGame = true;
        }
        else if (stricmp(argv[ee],"-unregistergame") == 0)
        {
            justUnRegisterGame = true;
        }
        else if ((stricmp(argv[ee],"-loadsavedgame") == 0) && (argc > ee + 1))
        {
            loadSaveGameOnStartup = argv[ee + 1];
            ee++;
        }
        else if ((stricmp(argv[ee],"--enabledebugger") == 0) && (argc > ee + 1))
        {
            strcpy(editor_debugger_instance_token, argv[ee + 1]);
            editor_debugging_enabled = 1;
            force_window = 1;
            ee++;
        }
        else if (stricmp(argv[ee], "--runfromide") == 0 && (argc > ee + 3))
        {
            usetup.install_dir = argv[ee + 1];
            usetup.install_audio_dir = argv[ee + 2];
            usetup.install_voice_dir = argv[ee + 3];
            ee += 3;
        }
        else if (stricmp(argv[ee],"--takeover")==0) {
            if (argc < ee+2)
                break;
            play.takeover_data = atoi (argv[ee + 1]);
            strncpy (play.takeover_from, argv[ee + 2], 49);
            play.takeover_from[49] = 0;
            ee += 2;
        }
        else if (stricmp(argv[ee], "--log") == 0)
        {
            enable_log_file = true;
        }
        else if (stricmp(argv[ee], "--no-log") == 0)
        {
            disable_log_file = true;
        }
        else if (argv[ee][0]!='-') datafile_argv=ee;
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

    return RETURN_CONTINUE;
}

void main_init_crt_report()
{
#ifdef _DEBUG
    /* logfile=fopen("g:\\ags.log","at");
    //_CrtSetReportHook( OurReportingFunction );
    int tmpDbgFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    //tmpDbgFlag |= _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_DELAY_FREE_MEM_DF;

    tmpDbgFlag = (tmpDbgFlag & 0x0000FFFF) | _CRTDBG_CHECK_EVERY_16_DF | _CRTDBG_DELAY_FREE_MEM_DF;

    _CrtSetDbgFlag(tmpDbgFlag);

    /*
    //  _CrtMemState memstart,memnow;
    _CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_WNDW );
    _CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_WNDW );
    _CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_WNDW );
    /*
    //   _CrtSetReportFile( _CRT_WARN, _CRTDBG_FILE_STDERR );
    //   _CrtSetReportFile( _CRT_ERROR, _CRTDBG_FILE_STDERR );
    //   _CrtSetReportFile( _CRT_ASSERT, _CRTDBG_FILE_STDERR );

    //  _CrtMemCheckpoint(&memstart);
    //  _CrtMemDumpStatistics( &memstart );*/
#endif
}

#if defined (WINDOWS_VERSION)
String GetPathInASCII(const String &path)
{
    char ascii_buffer[MAX_PATH];
    if (GetShortPathNameA(path, ascii_buffer, MAX_PATH) == 0)
    {
        Debug::Printf(kDbgMsg_Error, "Unable to determine path: GetShortPathNameA failed.\nArg: %s", path.GetCStr());
        return "";
    }
    return Path::MakeAbsolutePath(ascii_buffer);
}
#endif

void main_set_gamedir(int argc, char*argv[])
{
    appDirectory = Path::GetDirectoryPath(GetPathFromCmdArg(0));

    if ((loadSaveGameOnStartup != nullptr) && (argv[0] != nullptr))
    {
        // When launched by double-clicking a save game file, the curdir will
        // be the save game folder unless we correct it
        Directory::SetCurrentDirectory(appDirectory);
    }
#if defined (WINDOWS_VERSION)
    else
    {
        // It looks like Allegro library does not like ANSI (ACP) paths.
        // When *not* working in U_UNICODE filepath mode, whenever it gets
        // current directory for its own operations, it "fixes" it by
        // substituting non-ASCII symbols with '^'.
        // Here we explicitly set current directory to ASCII path.
        Directory::SetCurrentDirectory(GetPathInASCII(Directory::GetCurrentDirectory()));
    }
#endif
}

String GetPathFromCmdArg(int arg_index)
{
    if (arg_index < 0 || arg_index >= global_argc)
    {
        return "";
    }

    String path;
#if defined (WINDOWS_VERSION)
    // Hack for Windows in case there are unicode chars in the path.
    // The normal argv[] array has ????? instead of the unicode chars
    // and fails, so instead we manually get the short file name, which
    // is always using ASCII chars.
    WCHAR short_path[MAX_PATH];
    char ascii_buffer[MAX_PATH];
    LPCWSTR arg_path = wArgv[arg_index];
    if (GetShortPathNameW(arg_path, short_path, MAX_PATH) == 0)
    {
        Debug::Printf(kDbgMsg_Error, "Unable to determine path: GetShortPathNameW failed.\nCommand line argument %i: %s", arg_index, global_argv[arg_index]);
        return global_argv[arg_index];
    }
    WideCharToMultiByte(CP_ACP, 0, short_path, -1, ascii_buffer, MAX_PATH, NULL, NULL);
    path = ascii_buffer;
#else
    path = global_argv[arg_index];
#endif
    return Path::MakeAbsolutePath(path);
}

const char *get_allegro_error()
{
    return allegro_error;
}

const char *set_allegro_error(const char *format, ...)
{
    va_list argptr;
    va_start(argptr, format);
    uvszprintf(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text(format), argptr);
    va_end(argptr);
    return allegro_error;
}

#if defined(WINDOWS_VERSION)
#include <new.h>

#ifndef _DEBUG
extern void CreateMiniDump( EXCEPTION_POINTERS* pep );
#endif

char tempmsg[100];
char*printfworkingspace;
int malloc_fail_handler(size_t amountwanted) {
#ifndef _DEBUG
  CreateMiniDump(NULL);
#endif
  free(printfworkingspace);
  sprintf(tempmsg,"Out of memory: failed to allocate %ld bytes (at PP=%d)",amountwanted, our_eip);
  quit(tempmsg);
  return 0;
}
#endif

int main(int argc,char*argv[]) { 

#ifdef _DEBUG
    Test_DoAllTests();
#endif
    
    int res;
    main_init();
    
    res = main_preprocess_cmdline(argc, argv);
    if (res != RETURN_CONTINUE) {
        return res;
    }

#if defined(WINDOWS_VERSION)
    _set_new_handler(malloc_fail_handler);
    _set_new_mode(1);
    printfworkingspace=(char*)malloc(7000);
#endif
    debug_flags=0;

    res = main_process_cmdline(argc, argv);
    if (res != RETURN_CONTINUE) {
        return res;
    }

    if (justDisplayVersion)
    {
        platform->WriteStdOut(get_engine_string());
        return 0;
    }

    if (justDisplayHelp)
    {
        main_print_help();
        return 0;
    }

    init_debug();
    Debug::Printf(kDbgMsg_Init, get_engine_string());

    main_init_crt_report();

    main_set_gamedir(argc, argv);    

    // Update shell associations and exit
    if (debug_flags & DBG_REGONLY)
        exit(0);

#ifndef USE_CUSTOM_EXCEPTION_HANDLER
    usetup.disable_exception_handling = true;
#endif

    if (usetup.disable_exception_handling)
    {
        int result = initialize_engine(argc, argv);
        // TODO: refactor engine shutdown routine (must shutdown and delete everything started and created)
        allegro_exit();
        platform->PostAllegroExit();
        return result;
    }
    else
    {
        return initialize_engine_with_exception_handling(argc, argv);
    }
}

END_OF_MAIN()
