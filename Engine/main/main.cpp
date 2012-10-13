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

#include "util/wgt2allg.h"
#include "ac/common.h"
#include "ac/gamesetup.h"
#include "ac/gamestate.h"
#include "debug/agseditordebugger.h"
#include "debug/debug_log.h"
#include "main/engine.h"
#include "main/mainheader.h"
#include "main/main.h"
#include "platform/base/agsplatformdriver.h"
#include "ac/route_finder.h"
#include "core/assetmanager.h"

#ifdef _DEBUG
#include "test/test_all.h"
#endif

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

#ifndef WINDOWS_VERSION
char **global_argv = 0;
#endif


extern GameSetup usetup;
extern GameState play;
extern int our_eip;
extern AGSPlatformDriver *platform;
extern int debug_flags;
extern int force_letterbox;
extern int debug_15bit_mode, debug_24bit_mode;
extern int convert_16bit_bgr;
extern int display_fps;
extern int editor_debugging_enabled;
extern int editor_debugging_initialized;
extern char editor_debugger_instance_token[100];


// Startup flags, set from parameters to engine
int datafile_argv=0, change_to_game_dir = 0, force_window = 0;
int override_start_room = 0, force_16bit = 0;
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

void main_init()
{
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
#else
    global_argv = argv;
#endif
    return RETURN_CONTINUE;
}

extern char return_to_roomedit[30];
extern char return_to_room[150];

int main_process_cmdline(int argc,char*argv[])
{
    for (int ee=1;ee<argc;ee++) {
        if (argv[ee][1]=='?') return 0;
        if (stricmp(argv[ee],"-shelllaunch") == 0)
            change_to_game_dir = 1;
        else if (stricmp(argv[ee],"-updatereg") == 0)
            debug_flags |= DBG_REGONLY;
        else if (stricmp(argv[ee],"-windowed") == 0)
            force_window = 1;
        else if (stricmp(argv[ee],"-fullscreen") == 0)
            force_window = 2;
        else if (stricmp(argv[ee],"-hicolor") == 0)
            force_16bit = 1;
        else if (stricmp(argv[ee],"-letterbox") == 0)
            force_letterbox = 1;
        else if (stricmp(argv[ee],"-record") == 0)
            play.recording = 1;
        else if (stricmp(argv[ee],"-playback") == 0)
            play.playback = 1;
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
        else if (stricmp(argv[ee],"-noexceptionhandler")==0) usetup.disable_exception_handling = 1;
        else if (stricmp(argv[ee],"-dbgscript")==0) debug_flags|=DBG_DBGSCRIPT;
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

void change_to_directory_of_file(LPCWSTR fileName)
{
    WCHAR wcbuffer[MAX_PATH];
    StrCpyW(wcbuffer, fileName);

#if defined (WINDOWS_VERSION)
    LPWSTR backSlashAt = StrRChrW(wcbuffer, NULL, L'\\');
    if (backSlashAt != NULL) {
        wcbuffer[wcslen(wcbuffer) - wcslen(backSlashAt)] = L'\0';
        SetCurrentDirectoryW(wcbuffer);
    }
#else
    if (strrchr(wcbuffer, '/') != NULL) {
        strrchr(wcbuffer, '/')[0] = 0;
        chdir(wcbuffer);
    }
#endif
}

void main_set_gamedir(int argc,char*argv[])
{
    if ((loadSaveGameOnStartup != NULL) && (argv[0] != NULL))
    {
        // When launched by double-clicking a save game file, the curdir will
        // be the save game folder unless we correct it
        change_to_directory_of_file(wArgv[0]);
    }

    getcwd(appDirectory, 512);

    //if (change_to_game_dir == 1)  {
    if (datafile_argv > 0) {
        // If launched by double-clicking .AGS file, change to that
        // folder; else change to this exe's folder
        change_to_directory_of_file(wArgv[datafile_argv]);
    }

#ifdef MAC_VERSION
    getcwd(dataDirectory, 512);
#endif
}

#if defined(WINDOWS_VERSION)
#include <new.h>
char tempmsg[100];
char*printfworkingspace;
int malloc_fail_handler(size_t amountwanted) {
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

    printf("Adventure Creator v%sInterpreter\n"
      "Copyright (c) 1999-2001 Chris Jones\n" "ACI version %s\n", AC_VERSION_TEXT, ACI_VERSION_TEXT);

    if ((argc>1) && (argv[1][1]=='?'))
        return 0;

    initialize_debug_system();
    write_log_debug("***** ENGINE STARTUP");

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

    main_init_crt_report();

    main_set_gamedir(argc, argv);    

    // Update shell associations and exit
    if (debug_flags & DBG_REGONLY)
        exit(0);

#ifndef USE_CUSTOM_EXCEPTION_HANDLER
    usetup.disable_exception_handling = 1;
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
