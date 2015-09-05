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

namespace Directory = AGS::Common::Directory;
namespace Out       = AGS::Common::Out;
namespace Path      = AGS::Common::Path;

char appDirectory[512]; // Needed for library loading

#ifdef MAC_VERSION
char dataDirectory[512];
extern "C"
{
    int osx_sys_question(const char *msg, const char *but1, const char *but2);
}
#endif


#ifdef WINDOWS_VERSION

int wArgc;
LPWSTR *wArgv;

#endif

char **global_argv = NULL;
int    global_argc = 0;


extern GameSetup usetup;
extern GameState play;
extern int our_eip;
extern AGSPlatformDriver *platform;
extern int debug_flags;
extern int debug_15bit_mode, debug_24bit_mode;
extern int convert_16bit_bgr;
extern int display_fps;
extern int editor_debugging_enabled;
extern int editor_debugging_initialized;
extern char editor_debugger_instance_token[100];


// Startup flags, set from parameters to engine
char force_gfxfilter[50];
int datafile_argv=0, change_to_game_dir = 0, force_window = 0;
int override_start_room = 0, force_16bit = 0;
bool justDisplayHelp = false;
bool justRunSetup = false;
bool justRegisterGame = false;
bool justUnRegisterGame = false;
const char *loadSaveGameOnStartup = NULL;

#if !defined(IOS_VERSION) && !defined(PSP_VERSION) && !defined(ANDROID_VERSION)
int psp_video_framedrop = 1;
int psp_audio_enabled = 1;
int psp_midi_enabled = 1;
int psp_ignore_acsetup_cfg_file = 0;
int psp_clear_cache_on_room_change = 0;

int psp_midi_preload_patches = 0;
int psp_audio_cachesize = 10;
char psp_game_file_name[] = "ac2game.dat";
int psp_gfx_smooth_sprites = 1;
char psp_translation[] = "default";
#endif


void main_pre_init()
{
    our_eip = -999;
    Common::AssetManager::SetSearchPriority(Common::kAssetPriorityDir);
    play.recording = 0;
    play.playback = 0;
    play.takeover_data = 0;
}

void main_create_platform_driver()
{
    platform = AGSPlatformDriver::GetDriver();
}

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
AGS::Engine::Version EngineVersion;
// Lowest savedgame version, accepted by this engine
AGS::Engine::Version SavedgameLowestBackwardCompatVersion;
// Lowest engine version, which would accept current savedgames
AGS::Engine::Version SavedgameLowestForwardCompatVersion;

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
           "  --letterbox                  Enable letterbox mode\n"
           "  --gfxfilter <filter>         Enable graphics filter. Available options:\n"
           "                                 StdScale2, StdScale3, StdScale4, Hq2x or Hq3x\n"
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
    force_gfxfilter[0] = '\0';

    for (int ee=1;ee<argc;ee++) {
        if (stricmp(argv[ee],"--help") == 0 || stricmp(argv[ee],"/?") == 0 || stricmp(argv[ee],"-?") == 0)
        {
            justDisplayHelp = true;
            return RETURN_CONTINUE;
        }
        if (stricmp(argv[ee],"-shelllaunch") == 0)
            change_to_game_dir = 1;
        else if (stricmp(argv[ee],"-updatereg") == 0)
            debug_flags |= DBG_REGONLY;
        else if (stricmp(argv[ee],"-windowed") == 0 || stricmp(argv[ee],"--windowed") == 0)
            force_window = 1;
        else if (stricmp(argv[ee],"-fullscreen") == 0 || stricmp(argv[ee],"--fullscreen") == 0)
            force_window = 2;
        else if (stricmp(argv[ee],"-hicolor") == 0 || stricmp(argv[ee],"--hicolor") == 0)
            force_16bit = 1;
        else if (stricmp(argv[ee],"-letterbox") == 0 || stricmp(argv[ee],"--letterbox") == 0)
            usetup.prefer_letterbox = 1;
        else if (stricmp(argv[ee],"-record") == 0)
            play.recording = 1;
        else if (stricmp(argv[ee],"-playback") == 0)
            play.playback = 1;
        else if ((stricmp(argv[ee],"-gfxfilter") == 0 || stricmp(argv[ee],"--gfxfilter") == 0) && (argc > ee + 1))
        {
            strncpy(force_gfxfilter, argv[ee + 1], 49);
            ee++;
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
        else if (stricmp(argv[ee],"--15bit")==0) debug_15bit_mode = 1;
        else if (stricmp(argv[ee],"--24bit")==0) debug_24bit_mode = 1;
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

void change_to_directory_of_file(String path)
{
    if (Path::IsFile(path))
    {
        int slash_at = path.FindCharReverse('/');
        if (slash_at > 0)
        {
            path.ClipMid(slash_at);
        }
    }
    if (Path::IsDirectory(path))
    {
        Directory::SetCurrentDirectory(path);
    }
}

void main_set_gamedir(int argc,char*argv[])
{
    if ((loadSaveGameOnStartup != NULL) && (argv[0] != NULL))
    {
        // When launched by double-clicking a save game file, the curdir will
        // be the save game folder unless we correct it
        change_to_directory_of_file(GetPathFromCmdArg(0));
    }

    getcwd(appDirectory, 512);

    //if (change_to_game_dir == 1)  {
    if (datafile_argv > 0) {
        // If launched by double-clicking .AGS file, change to that
        // folder; else change to this exe's folder
        change_to_directory_of_file(GetPathFromCmdArg(datafile_argv));
    }

#ifdef MAC_VERSION
    getcwd(dataDirectory, 512);
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
    // is always using ANSI chars.
    WCHAR short_path[MAX_PATH];
    char ansi_buffer[MAX_PATH];
    LPCWSTR arg_path = wArgv[arg_index];
    if (GetShortPathNameW(arg_path, short_path, MAX_PATH) == 0)
    {
        Out::FPrint("Unable to determine path: GetShortPathNameW failed.\nCommand line argument %i: %s", arg_index, global_argv[arg_index]);
        return "";
    }
    WideCharToMultiByte(CP_ACP, 0, short_path, -1, ansi_buffer, MAX_PATH, NULL, NULL);
    path = ansi_buffer;
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

    initialize_debug_system();

    Out::FPrint("Adventure Game Studio v%s Interpreter\n"
           "Copyright (c) 1999-2011 Chris Jones and " ACI_COPYRIGHT_YEARS " others\n"
#ifdef BUILD_STR
           "ACI version %s (Build: %s)\n",
           EngineVersion.ShortString.GetCStr(), EngineVersion.LongString.GetCStr(), EngineVersion.BuildInfo.GetCStr());
#else
           "ACI version %s\n", EngineVersion.ShortString.GetCStr(), EngineVersion.LongString.GetCStr());
#endif

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

    if (justDisplayHelp)
    {
        main_print_help();
        return 0;
    }

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
        platform->PostAllegroExit();
        return result;
    }
    else
    {
        return initialize_engine_with_exception_handling(argc, argv);
    }
}

#if !defined (DOS_VERSION)
END_OF_MAIN()
#endif
