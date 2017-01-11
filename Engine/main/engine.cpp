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
// Engine initialization
//

#include "main/mainheader.h"
#include "ac/common.h"
#include "ac/character.h"
#include "ac/characterextras.h"
#include "ac/characterinfo.h"
#include "ac/draw.h"
#include "ac/game.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/global_character.h"
#include "ac/global_game.h"
#include "ac/gui.h"
#include "ac/lipsync.h"
#include "ac/objectcache.h"
#include "ac/roomstatus.h"
#include "ac/speech.h"
#include "ac/translation.h"
#include "ac/viewframe.h"
#include "ac/dynobj/scriptobject.h"
#include "ac/dynobj/scriptsystem.h"
#include "debug/debug_log.h"
#include "debug/debugger.h"
#include "debug/out.h"
#include "font/agsfontrenderer.h"
#include "font/fonts.h"
#include "game/main_game_file.h"
#include "main/config.h"
#include "main/game_start.h"
#include "main/engine.h"
#include "main/engine_setup.h"
#include "main/graphics_mode.h"
#include "main/main.h"
#include "main/main_allegro.h"
#include "media/audio/sound.h"
#include "ac/spritecache.h"
#include "util/filestream.h"
#include "gfx/graphicsdriver.h"
#include "core/assetmanager.h"
#include "util/misc.h"
#include "platform/util/pe.h"
#include "util/directory.h"
#include "util/path.h"
#include "main/game_file.h"
#include "debug/out.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern char check_dynamic_sprites_at_exit;
extern int our_eip;
extern volatile char want_exit, abort_engine;
extern bool justRunSetup;
extern GameSetup usetup;
extern GameSetupStruct game;
extern int proper_exit;
extern char pexbuf[STD_BUFFER_SIZE];
extern char saveGameDirectory[260];
extern int spritewidth[MAX_SPRITES],spriteheight[MAX_SPRITES];
extern SpriteCache spriteset;
extern ObjectCache objcache[MAX_INIT_SPR];
extern ScriptObject scrObj[MAX_INIT_SPR];
extern ViewStruct*views;
extern int displayed_room;
extern int eip_guinum;
extern int eip_guiobj;
extern const char *replayTempFile;
extern SpeechLipSyncLine *splipsync;
extern int numLipLines, curLipLine, curLipLinePhoneme;
extern ScriptSystem scsystem;
extern IGraphicsDriver *gfxDriver;
extern Bitmap **actsps;
extern color palette[256];
extern CharacterExtras *charextra;
extern CharacterInfo*playerchar;
extern Bitmap **guibg;
extern IDriverDependantBitmap **guibgbmp;

char *music_file;
char *speech_file;

Common::AssetError errcod;

t_engine_pre_init_callback engine_pre_init_callback = 0;

extern "C" HWND allegro_wnd;

#define ALLEGRO_KEYBOARD_HANDLER
// KEYBOARD HANDLER
#if !defined (WINDOWS_VERSION)
int myerrno;
#else
int errno;
#define myerrno errno
#endif

bool engine_init_allegro()
{
    Debug::Printf(kDbgMsg_Init, "Initializing allegro");

    our_eip = -199;
    // Initialize allegro
    set_uformat(U_ASCII);
    if (install_allegro(SYSTEM_AUTODETECT, &myerrno, atexit))
    {
        const char *al_err = get_allegro_error();
        const char *user_hint = platform->GetAllegroFailUserHint();
        platform->DisplayAlert("Unable to initialize Allegro system driver.\n%s\n\n%s",
            al_err[0] ? al_err : "Allegro library provided no further information on the problem.",
            user_hint);
        return false;
    }
    return true;
}

void engine_setup_allegro()
{
    // Setup allegro using constructed config string
    const char *al_config_data = "[mouse]\n"
        "mouse_accel_factor = 0\n";
    override_config_data(al_config_data, ustrsize(al_config_data));
}

void winclosehook() {
  want_exit = 1;
  abort_engine = 1;
  check_dynamic_sprites_at_exit = 0;
}

void engine_setup_window()
{
    Debug::Printf(kDbgMsg_Init, "Setting up window");

    our_eip = -198;
#if (ALLEGRO_DATE > 19990103)
    set_window_title("Adventure Game Studio");
#if (ALLEGRO_DATE > 20021115)
    set_close_button_callback (winclosehook);
#else
    set_window_close_hook (winclosehook);
#endif

    our_eip = -197;
#endif

    platform->SetGameWindowIcon();
}

bool engine_check_run_setup(const String &exe_path, ConfigTree &cfg)
{
#if defined (WINDOWS_VERSION)
    // check if Setup needs to be run instead
    if (justRunSetup)
    {
            String cfg_file = find_user_cfg_file();
            if (cfg_file.IsEmpty())
                return false;

            Debug::Printf(kDbgMsg_Init, "Running Setup");

            // Add information about game resolution and let setup application
            // display some properties to the user
            INIwriteint(cfg, "misc", "defaultres", game.GetDefaultResolution());
            INIwriteint(cfg, "misc", "letterbox", game.options[OPT_LETTERBOX]);
            INIwriteint(cfg, "misc", "game_width", game.size.Width);
            INIwriteint(cfg, "misc", "game_height", game.size.Height);
            INIwriteint(cfg, "misc", "gamecolordepth", game.color_depth * 8);
            if (game.options[OPT_RENDERATSCREENRES] != kRenderAtScreenRes_UserDefined)
            {
                // force enabled/disabled
                INIwriteint(cfg, "graphics", "render_at_screenres", game.options[OPT_RENDERATSCREENRES] == kRenderAtScreenRes_Enabled);
                INIwriteint(cfg, "disabled", "render_at_screenres", 1);
            }

            ConfigTree cfg_out;
            SetupReturnValue res = platform->RunSetup(cfg, cfg_out);
            if (res != kSetup_Cancel)
            {
                if (!IniUtil::Merge(cfg_file, cfg_out))
                {
                    platform->DisplayAlert("Unable to write to the configuration file (error code 0x%08X).\n%s",
                        platform->GetLastSystemError(), platform->GetFileWriteTroubleshootingText());
                }
            }
            if (res != kSetup_RunGame)
                return false;

            // TODO: investigate if the full program restart may (should) be avoided

            // Just re-reading the config file seems to cause a caching
            // problem on Win9x, so let's restart the process.
            allegro_exit();
            char quotedpath[255];
            sprintf (quotedpath, "\"%s\"", exe_path);
            _spawnl (_P_OVERLAY, exe_path, quotedpath, NULL);
    }
#endif

    return true;
}

void engine_force_window()
{
    // Force to run in a window, override the config file
    // TODO: actually overwrite config tree instead
    if (force_window == 1)
    {
        usetup.Screen.DisplayMode.Windowed = true;
        usetup.Screen.DisplayMode.SizeDef = kScreenDef_ByGameScaling;
    }
    else if (force_window == 2)
        usetup.Screen.DisplayMode.Windowed = false;
}

void init_game_file_name_from_cmdline()
{
    game_file_name.Empty();
#if defined(PSP_VERSION) || defined(ANDROID_VERSION) || defined(IOS_VERSION) || defined(MAC_VERSION)
    game_file_name = psp_game_file_name;
#else
    game_file_name = GetPathFromCmdArg(datafile_argv);
#endif
}

String find_game_data_in_directory(const String &path)
{
    al_ffblk ff;
    String test_file;
    String first_nonstd_fn;
    String pattern = path;
    pattern.Append("/*");

    if (al_findfirst(pattern, &ff, FA_ALL & ~(FA_DIREC)) != 0)
        return "";
    // Select first found data file; files with standart names (*.ags) have
    // higher priority over files with custom names.
    do
    {
        test_file = ff.name;
        // Add a bit of sanity and do not parse contents of the 10k-files-large
        // digital sound libraries.
        // NOTE: we could certainly benefit from any kind of flag in file lib
        // that would tell us this is the main lib without extra parsing.
        if (test_file.CompareRightNoCase(".vox") == 0)
            continue;

        // *.ags is a standart cross-platform file pattern for AGS games,
        // ac2game.dat is a legacy file name for very old games,
        // *.exe is a MS Win executable; it is included to this case because
        // users often run AGS ports with Windows versions of games.
        bool is_std_name = test_file.CompareRightNoCase(".ags") == 0 ||
            test_file.CompareNoCase("ac2game.dat") == 0 ||
            test_file.CompareRightNoCase(".exe") == 0;
        if (is_std_name || first_nonstd_fn.IsEmpty())
        {
            test_file.Format("%s/%s", path.GetCStr(), ff.name);
            if (IsMainGameLibrary(test_file))
            {
                if (is_std_name)
                {
                    al_findclose(&ff);
                    return test_file;
                }
                else
                    first_nonstd_fn = test_file;
            }
        }
    }
    while(al_findnext(&ff) == 0);
    al_findclose(&ff);
    return first_nonstd_fn;
}

extern char appDirectory[512];
void initialise_game_file_name()
{
    // 1. From command line argument
    if (datafile_argv > 0)
    {
        // set game_file_name from cmd arg (do any convertions if needed)
        init_game_file_name_from_cmdline();
        if (!Path::IsFile(game_file_name))
        {
            // if it is not a file, assume it is a directory and seek for data file
            game_file_name = find_game_data_in_directory(game_file_name);
        }
    }
    // 2. From setup
    // 2.1. Use the provided data dir and filename
    else if (!usetup.main_data_filename.IsEmpty())
    {
        if (!usetup.data_files_dir.IsEmpty() && is_relative_filename(usetup.main_data_filename))
        {
            game_file_name = usetup.data_files_dir;
            if (game_file_name.GetLast() != '/' && game_file_name.GetLast() != '\\')
                game_file_name.AppendChar('/');
            game_file_name.Append(usetup.main_data_filename);
        }
        else
        {
            game_file_name = usetup.main_data_filename;
        }
    }
    // 2.2. Search in the provided data dir
    else if (!usetup.data_files_dir.IsEmpty())
    {
        game_file_name = find_game_data_in_directory(usetup.data_files_dir);
    }
    // 3. Look in known locations
    else
    {
        // 3.1. Look for attachment in the running executable
        //
        // set game_file_name from cmd arg (do any conversions if needed)
        // this will use argument zero, the executable's name
        init_game_file_name_from_cmdline();
        if (game_file_name.IsEmpty() || !Common::AssetManager::IsDataFile(game_file_name))
        {
            // 3.2 Look in current directory
            String cur_dir = Directory::GetCurrentDirectory();
            game_file_name = find_game_data_in_directory(cur_dir);
            if (game_file_name.IsEmpty())
            {
                // 3.3 Look in executable's directory (if it's different from current dir)
                if (Path::ComparePaths(appDirectory, cur_dir))
                {
                    game_file_name = find_game_data_in_directory(appDirectory);
                }
            }
        }
    }

    // Finally, store game file's absolute path, or report error
    if (game_file_name.IsEmpty())
    {
        Debug::Printf(kDbgMsg_Error, "Game data file could not be found\n");
    }
    else
    {
        game_file_name = Path::MakeAbsolutePath(game_file_name);
        Debug::Printf(kDbgMsg_Init, "Located game data file: %s\n", game_file_name.GetCStr());
    }
}

bool engine_init_game_data()
{
    Debug::Printf("Looking for game data file");

    // initialize the data file
    initialise_game_file_name();
    errcod = Common::AssetManager::SetDataFile(game_file_name);

    our_eip = -194;
    our_eip = -193;

    if (errcod!=Common::kAssetNoError)
    {  // there's a problem
        char emsg[STD_BUFFER_SIZE];
        if (errcod==Common::kAssetErrNoLibFile)
        {  // file not found
            if (game_file_name.IsEmpty())
            {
                sprintf(emsg, "ERROR: Unable to find game data files\n\n");
            }
            else
            {
                sprintf(emsg, "ERROR: Unable to find or open '%s'.\n\n", game_file_name.GetCStr());
            }
        }
        else if (errcod==Common::kAssetErrLibParse)
        {
            sprintf(emsg, "ERROR: The game file is of unsupported format or file is corrupt. "
                "Make sure you have the correct version of the "
                "editor, and that this really is an AGS game.\n\n%s\n\n", game_file_name.GetCStr());
        }

        platform->DisplayAlert(emsg);

        main_print_help();

        return false;
    }

    // Save data file name and data folder
    usetup.main_data_filename = get_filename(game_file_name);
    // There is a path in the game file name (and the user/ has not specified
    // another one) save the path, so that it can load the VOX files, etc
    if (usetup.data_files_dir.IsEmpty())
    {
        int ichar = game_file_name.FindCharReverse('/');
        if (ichar >= 0)
        {
            usetup.data_files_dir = game_file_name.Left(ichar);
        }
    }

    return true;
}

void engine_init_fonts()
{
    Debug::Printf(kDbgMsg_Init, "Initializing TTF renderer");

    init_font_renderer();
}

int engine_init_mouse()
{
    int res = minstalled();
    if (res < 0)
        Debug::Printf(kDbgMsg_Init, "Initializing mouse: failed");
    else
        Debug::Printf(kDbgMsg_Init, "Initializing mouse: number of buttons reported is %d", res);
	return RETURN_CONTINUE;
}

int engine_check_memory()
{
    Debug::Printf(kDbgMsg_Init, "Checking memory");

    char*memcheck=(char*)malloc(4000000);
    if (memcheck==NULL) {
        platform->DisplayAlert("There is not enough memory available to run this game. You need 4 Mb free\n"
            "extended memory to run the game.\n"
            "If you are running from Windows, check the 'DPMI memory' setting on the DOS box\n"
            "properties.\n");
        return EXIT_NORMAL;
    }
    free(memcheck);
    unlink (replayTempFile);
    return RETURN_CONTINUE;
}

void engine_init_rooms()
{
    // Obsolete now since room statuses are allocated only when needed
}

int engine_init_speech()
{
    play.want_speech=-2;

    if (!usetup.no_speech_pack) {
        /* Can't just use fopen here, since we need to change the filename
        so that pack functions, etc. will have the right case later */
        speech_file = ci_find_file(usetup.data_files_dir, "speech.vox");

        Stream *speech_s = ci_fopen(speech_file);

        if (speech_s == NULL)
        {
            // In case they're running in debug, check Compiled folder
            free(speech_file);
            speech_file = ci_find_file("Compiled", "speech.vox");
            speech_s = ci_fopen(speech_file);
        }

        if (speech_s) {
            delete speech_s;

            Debug::Printf("Initializing speech vox");

            //if (Common::AssetManager::SetDataFile(useloc,"")!=0) {
            if (Common::AssetManager::SetDataFile(speech_file)!=Common::kAssetNoError) {
                platform->DisplayAlert("Unable to initialize speech sample file - check for corruption and that\nit belongs to this game.\n");
                return EXIT_NORMAL;
            }
            Stream *speechsync = Common::AssetManager::OpenAsset("syncdata.dat");
            if (speechsync != NULL) {
                // this game has voice lip sync
                if (speechsync->ReadInt32() != 4)
                { 
                    // Don't display this warning.
                    // platform->DisplayAlert("Unknown speech lip sync format (might be from older or newer version); lip sync disabled");
                }
                else {
                    numLipLines = speechsync->ReadInt32();
                    splipsync = (SpeechLipSyncLine*)malloc (sizeof(SpeechLipSyncLine) * numLipLines);
                    for (int ee = 0; ee < numLipLines; ee++)
                    {
                        splipsync[ee].numPhonemes = speechsync->ReadInt16();
                        speechsync->Read(splipsync[ee].filename, 14);
                        splipsync[ee].endtimeoffs = (int*)malloc(splipsync[ee].numPhonemes * sizeof(int));
                        speechsync->ReadArrayOfInt32(splipsync[ee].endtimeoffs, splipsync[ee].numPhonemes);
                        splipsync[ee].frame = (short*)malloc(splipsync[ee].numPhonemes * sizeof(short));
                        speechsync->ReadArrayOfInt16(splipsync[ee].frame, splipsync[ee].numPhonemes);
                    }
                }
                delete speechsync;
            }
            Common::AssetManager::SetDataFile(game_file_name);
            Debug::Printf(kDbgMsg_Init, "Speech sample file found and initialized.");
            play.want_speech=1;
        }
    }

    return RETURN_CONTINUE;
}

int engine_init_music()
{
    play.separate_music_lib = 0;

    /* Can't just use fopen here, since we need to change the filename
    so that pack functions, etc. will have the right case later */
    music_file = ci_find_file(usetup.data_files_dir, "audio.vox");

    /* Don't need to use ci_fopen here, because we've used ci_find_file to get
    the case insensitive matched filename already */
    // Use ci_fopen anyway because it can handle NULL filenames.
    Stream *music_s = ci_fopen(music_file);

    if (music_s == NULL)
    {
        // In case they're running in debug, check Compiled folder
        free(music_file);
        music_file = ci_find_file("Compiled", "audio.vox");
        music_s = ci_fopen(music_file);
    }

    if (music_s) {
        delete music_s;

        Debug::Printf("Initializing audio vox");

        //if (Common::AssetManager::SetDataFile(useloc,"")!=0) {
        if (Common::AssetManager::SetDataFile(music_file)!=Common::kAssetNoError) {
            platform->DisplayAlert("Unable to initialize music library - check for corruption and that\nit belongs to this game.\n");
            return EXIT_NORMAL;
        }
        Common::AssetManager::SetDataFile(game_file_name);
        Debug::Printf(kDbgMsg_Init, "Audio vox found and initialized.");
        play.separate_music_lib = 1;
    }

    return RETURN_CONTINUE;
}

void engine_init_keyboard()
{
#ifdef ALLEGRO_KEYBOARD_HANDLER
    Debug::Printf(kDbgMsg_Init, "Initializing keyboard");

    install_keyboard();
#endif
}

void engine_init_timer()
{
    Debug::Printf(kDbgMsg_Init, "Install timer");
    install_timer();
}

typedef char AlIDStr[5];

void AlIDToChars(int al_id, AlIDStr &id_str)
{
    id_str[0] = (al_id >> 24) & 0xFF;
    id_str[1] = (al_id >> 16) & 0xFF;
    id_str[2] = (al_id >> 8) & 0xFF;
    id_str[3] = (al_id) & 0xFF;
    id_str[4] = 0;
}

void AlDigiToChars(int digi_id, AlIDStr &id_str)
{
    if (digi_id == DIGI_NONE)
        strcpy(id_str, "None");
    else if (digi_id == DIGI_AUTODETECT)
        strcpy(id_str, "Auto");
    else
        AlIDToChars(digi_id, id_str);
}

void AlMidiToChars(int midi_id, AlIDStr &id_str)
{
    if (midi_id == MIDI_NONE)
        strcpy(id_str, "None");
    else if (midi_id == MIDI_AUTODETECT)
        strcpy(id_str, "Auto");
    else
        AlIDToChars(midi_id, id_str);
}

bool try_install_sound(int digi_id, int midi_id)
{
    if (install_sound(digi_id, midi_id, NULL) == 0)
        return true;
    // Allegro does not let you try digital and MIDI drivers separately,
    // and does not indicate which driver failed by return value.
    // Therefore we try to guess.
    if (midi_id != MIDI_NONE)
    {
        Debug::Printf(kDbgMsg_Error, "Failed to init one of the drivers; Error: '%s'.\nWill try to start without MIDI", get_allegro_error());
        if (install_sound(digi_id, MIDI_NONE, NULL) == 0)
            return true;
    }
    if (digi_id != DIGI_NONE)
    {
        Debug::Printf(kDbgMsg_Error, "Failed to init one of the drivers; Error: '%s'.\nWill try to start without DIGI", get_allegro_error());
        if (install_sound(DIGI_NONE, midi_id, NULL) == 0)
            return true;
    }
    Debug::Printf(kDbgMsg_Error, "Failed to init sound drivers. Error: %s", get_allegro_error());
    return false;
}

void engine_init_sound()
{
    if (opts.mod_player)
        reserve_voices(16, -1);
#if ALLEGRO_DATE > 19991010
    // maybe this line will solve the sound volume?
    set_volume_per_voice(1);
#endif

    Debug::Printf("Initialize sound drivers");

    // PSP: Disable sound by config file.
    if (!psp_audio_enabled)
    {
        usetup.digicard = DIGI_NONE;
        usetup.midicard = MIDI_NONE;
    }

    if (!psp_midi_enabled)
        usetup.midicard = MIDI_NONE;

    AlIDStr digi_id;
    AlIDStr midi_id;
    AlDigiToChars(usetup.digicard, digi_id);
    AlMidiToChars(usetup.midicard, midi_id);
    Debug::Printf(kDbgMsg_Init, "Sound settings: digital driver ID: '%s' (0x%x), MIDI driver ID: '%s' (0x%x)",
        digi_id, usetup.digicard, midi_id, usetup.midicard);

    bool sound_res = try_install_sound(usetup.digicard, usetup.midicard);
    if (!sound_res && opts.mod_player)
    {
        Debug::Printf("Resetting to default sound parameters and trying again.");
        reserve_voices(-1, -1); // this resets voice number to defaults
        opts.mod_player = 0;
        opts.mp3_player = 0; // CHECKME: why disabling MP3 player too?
        sound_res = try_install_sound(usetup.digicard, usetup.midicard);
    }
    if (!sound_res)
    {
        // If everything failed, disable sound completely
        reserve_voices(0,0);
        install_sound(DIGI_NONE, MIDI_NONE, NULL);
    }
    if (usetup.digicard != DIGI_NONE && digi_card == DIGI_NONE ||
        usetup.midicard != MIDI_NONE && midi_card == MIDI_NONE)
    {
        // only flag an error if they wanted a sound card
        platform->DisplayAlert("\nUnable to initialize your audio hardware.\n"
            "[Problem: %s]\n", get_allegro_error());
    }

    usetup.digicard = digi_card;
    usetup.midicard = midi_card;

    AlDigiToChars(usetup.digicard, digi_id);
    AlMidiToChars(usetup.midicard, midi_id);
    Debug::Printf(kDbgMsg_Init, "Installed digital driver ID: '%s' (0x%x), MIDI driver ID: '%s' (0x%x)",
        digi_id, usetup.digicard, midi_id, usetup.midicard);

    our_eip = -181;

    if (usetup.digicard == DIGI_NONE)
    {
        // disable speech and music if no digital sound
        // therefore the MIDI soundtrack will be used if present,
        // and the voice mode should not go to Voice Only
        play.want_speech = -2;
        play.separate_music_lib = 0;
    }

#ifdef WINDOWS_VERSION
    if (usetup.digicard == DIGI_DIRECTX(0))
    {
        // DirectX mixer seems to buffer an extra sample itself
        use_extra_sound_offset = 1;
    }
#endif
}

void engine_init_debug()
{
    //set_volume(255,-1);
    if ((debug_flags & (~DBG_DEBUGMODE)) >0) {
        platform->DisplayAlert("Engine debugging enabled.\n"
            "\nNOTE: You have selected to enable one or more engine debugging options.\n"
            "These options cause many parts of the game to behave abnormally, and you\n"
            "may not see the game as you are used to it. The point is to test whether\n"
            "the engine passes a point where it is crashing on you normally.\n"
            "[Debug flags enabled: 0x%02X]\n"
            "Press a key to continue.\n",debug_flags);
    }
}

void atexit_handler() {
    if (proper_exit==0) {
        sprintf(pexbuf,"\nError: the program has exited without requesting it.\n"
            "Program pointer: %+03d  (write this number down), ACI version %s\n"
            "If you see a list of numbers above, please write them down and contact\n"
            "developers. Otherwise, note down any other information displayed.\n",
            our_eip, EngineVersion.LongString.GetCStr());
        platform->DisplayAlert(pexbuf);
    }

    if (!(music_file == NULL))
        free(music_file);

    if (!(speech_file == NULL))
        free(speech_file);
}

void engine_init_exit_handler()
{
    Debug::Printf(kDbgMsg_Init, "Install exit handler");

    atexit(atexit_handler);
}

void engine_init_rand()
{
    play.randseed = time(NULL);
    srand (play.randseed);
}

void engine_init_pathfinder()
{
    Debug::Printf(kDbgMsg_Init, "Initialize path finder library");

    init_pathfinder();
}

void engine_pre_init_gfx()
{
    //Debug::Printf("Initialize gfx");

    //platform->InitialiseAbufAtStartup();
}

int engine_load_game_data()
{
    Debug::Printf("Load game data");

    our_eip=-17;
    String err_str;
    if (!load_game_file(err_str))
    {
        proper_exit=1;
        platform->FinishedUsingGraphicsMode();
        display_game_file_error(err_str);
        return EXIT_NORMAL;
    }

    return RETURN_CONTINUE;
}

int engine_check_register_game()
{
    if (justRegisterGame) 
    {
        platform->RegisterGameWithGameExplorer();
        proper_exit = 1;
        return EXIT_NORMAL;
    }

    if (justUnRegisterGame) 
    {
        platform->UnRegisterGameWithGameExplorer();
        proper_exit = 1;
        return EXIT_NORMAL;
    }

    return RETURN_CONTINUE;
}

void engine_init_title()
{
    //platform->DisplayAlert("loaded game");
    our_eip=-91;
#if (ALLEGRO_DATE > 19990103)
    set_window_title(game.gamename);
#endif

    Debug::Printf(kDbgMsg_Init, "Game title: '%s'", game.gamename);
}

void engine_init_directories()
{
    if (file_exists("Compiled", FA_ARCH | FA_DIREC, NULL))
    {
        // running in debugger
        use_compiled_folder_as_current_dir = 1;
        // don't redirect to the game exe folder (_Debug)
        usetup.data_files_dir = ".";
    }

    // if end-user specified custom save path, use it
    bool res = false;
    if (!usetup.user_data_dir.IsEmpty())
    {
        res = SetCustomSaveParent(String::FromFormat("%s/UserSaves", usetup.user_data_dir.GetCStr()));
        if (!res)
        {
            Debug::Printf(kDbgMsg_Warn, "WARNING: custom user save path failed, using default system paths");
            usetup.user_data_dir.Empty();
            res = false;
        }
    }
    // if there is no custom path, or if custom path failed, use default system path
    if (!res)
    {
        char newDirBuffer[MAX_PATH];
        sprintf(newDirBuffer, "%s/%s", UserSavedgamesRootToken.GetCStr(), game.saveGameFolderName);
        SetSaveGameDirectoryPath(newDirBuffer);
    }
}

#if defined(ANDROID_VERSION)
extern char android_base_directory[256];
#endif // ANDROID_VERSION

int check_write_access() {

  if (platform->GetDiskFreeSpaceMB() < 2)
    return 0;

  our_eip = -1895;

  // The Save Game Dir is the only place that we should write to
  char tempPath[MAX_PATH];
  sprintf(tempPath, "%s""tmptest.tmp", saveGameDirectory);
  Stream *temp_s = Common::File::CreateFile(tempPath);
  if (!temp_s)
#if defined(ANDROID_VERSION)
  {
	  put_backslash(android_base_directory);
	  sprintf(tempPath, "%s""tmptest.tmp", android_base_directory);
	  temp_s = Common::File::CreateFile(tempPath);
	  if (temp_s == NULL) return 0;
	  else SetCustomSaveParent(android_base_directory);
  }
#else
    return 0;
#endif // ANDROID_VERSION

  our_eip = -1896;

  temp_s->Write("just to test the drive free space", 30);
  delete temp_s;

  our_eip = -1897;

  if (unlink(tempPath))
    return 0;

  return 1;
}

int engine_check_disk_space()
{
    Debug::Printf(kDbgMsg_Init, "Checking for disk space");

    //init_language_text("en");
    if (check_write_access()==0) {
#if defined(IOS_VERSION)
        platform->DisplayAlert("Unable to write to the current directory. Make sure write permissions are"
            " set for the game directory.\n");
#else
        platform->DisplayAlert("Unable to write to the current directory. Do not run this game off a\n"
            "network or CD-ROM drive. Also check drive free space (you need 1 Mb free).\n");
#endif
        proper_exit = 1;
        return EXIT_NORMAL; 
    }

    return RETURN_CONTINUE;
}

int engine_check_font_was_loaded()
{
    if (!font_first_renderer_loaded())
    {
        platform->DisplayAlert("No fonts found. If you're trying to run the game from the Debug directory, this is not supported. Use the Build EXE command to create an executable in the Compiled folder.");
        proper_exit = 1;
        return EXIT_NORMAL;
    }

    return RETURN_CONTINUE;
}

void engine_init_modxm_player()
{
#ifndef PSP_NO_MOD_PLAYBACK
    if (game.options[OPT_NOMODMUSIC])
        opts.mod_player = 0;

    if (opts.mod_player) {
        Debug::Printf(kDbgMsg_Init, "Initializing MOD/XM player");

        if (init_mod_player(NUM_MOD_DIGI_VOICES) < 0) {
            platform->DisplayAlert("Warning: install_mod: MOD player failed to initialize.");
            opts.mod_player=0;
        }
    }
#else
    opts.mod_player = 0;
    Debug::Printf(kDbgMsg_Init, "Compiled without MOD/XM player");
#endif
}

void show_preload () {
    // ** Do the preload graphic if available
    color temppal[256];
	Bitmap *splashsc = BitmapHelper::CreateRawBitmapOwner( load_pcx("preload.pcx",temppal) );
    if (splashsc != NULL) {
        if (splashsc->GetColorDepth() == 8)
            set_palette_range(temppal, 0, 255, 0);
		Bitmap *screen_bmp = BitmapHelper::GetScreenBitmap();
        Bitmap *tsc = BitmapHelper::CreateBitmapCopy(splashsc, screen_bmp->GetColorDepth());

		screen_bmp->Fill(0);
        screen_bmp->StretchBlt(tsc, RectWH(0, 0, play.viewport.GetWidth(),play.viewport.GetHeight()), Common::kBitmap_Transparency);

        gfxDriver->ClearDrawList();

        if (!gfxDriver->UsesMemoryBackBuffer())
        {
            IDriverDependantBitmap *ddb = gfxDriver->CreateDDBFromBitmap(screen_bmp, false, true);
            gfxDriver->DrawSprite(0, 0, ddb);
            render_to_screen(screen_bmp, 0, 0);
            gfxDriver->DestroyDDB(ddb);
        }
        else
			render_to_screen(screen_bmp, 0, 0);

        delete splashsc;
        delete tsc;
        platform->Delay(500);
    }
}

void engine_show_preload()
{
    Debug::Printf("Check for preload image");

    show_preload ();
}

int engine_init_sprites()
{
    Debug::Printf(kDbgMsg_Init, "Initialize sprites");

    if (spriteset.initFile ("acsprset.spr")) 
    {
        platform->FinishedUsingGraphicsMode();
        allegro_exit();
        proper_exit=1;
        platform->DisplayAlert("Could not load sprite set file ACSPRSET.SPR\n"
            "This means that the file is missing or there is not enough free\n"
            "system memory to load the file.\n");
        return EXIT_NORMAL;
    }

    return RETURN_CONTINUE;
}

void engine_init_game_settings()
{
    our_eip=-7;
    Debug::Printf("Initialize game settings");

    int ee;

    for (ee = 0; ee < MAX_INIT_SPR + game.numcharacters; ee++)
        actsps[ee] = NULL;

    for (ee=0;ee<256;ee++) {
        if (game.paluses[ee]!=PAL_BACKGROUND)
            palette[ee]=game.defpal[ee];
    }

    if (game.options[OPT_NOSCALEFNT]) wtext_multiply=1;

    for (ee = 0; ee < game.numcursors; ee++) 
    {
        // The cursor graphics are assigned to mousecurs[] and so cannot
        // be removed from memory
        if (game.mcurs[ee].pic >= 0)
            spriteset.precache (game.mcurs[ee].pic);

        // just in case they typed an invalid view number in the editor
        if (game.mcurs[ee].view >= game.numviews)
            game.mcurs[ee].view = -1;

        if (game.mcurs[ee].view >= 0)
            precache_view (game.mcurs[ee].view);
    }
    // may as well preload the character gfx
    if (playerchar->view >= 0)
        precache_view (playerchar->view);

    for (ee = 0; ee < MAX_INIT_SPR; ee++)
        objcache[ee].image = NULL;

    /*  dummygui.guiId = -1;
    dummyguicontrol.guin = -1;
    dummyguicontrol.objn = -1;*/

    our_eip=-6;
    //  game.chars[0].talkview=4;
    //init_language_text(game.langcodes[0]);

    for (ee = 0; ee < MAX_INIT_SPR; ee++) {
        scrObj[ee].id = ee;
        // 64 bit: Using the id instead
        // scrObj[ee].obj = NULL;
    }

    for (ee=0;ee<game.numcharacters;ee++) {
        memset(&game.chars[ee].inv[0],0,MAX_INV*sizeof(short));
        game.chars[ee].activeinv=-1;
        game.chars[ee].following=-1;
        game.chars[ee].followinfo=97 | (10 << 8);
        game.chars[ee].idletime=20;  // can be overridden later with SetIdle or summink
        game.chars[ee].idleleft=game.chars[ee].idletime;
        game.chars[ee].transparency = 0;
        game.chars[ee].baseline = -1;
        game.chars[ee].walkwaitcounter = 0;
        game.chars[ee].z = 0;
        charextra[ee].xwas = INVALID_X;
        charextra[ee].zoom = 100;
        if (game.chars[ee].view >= 0) {
            // set initial loop to 0
            game.chars[ee].loop = 0;
            // or to 1 if they don't have up/down frames
            if (views[game.chars[ee].view].loops[0].numFrames < 1)
                game.chars[ee].loop = 1;
        }
        charextra[ee].process_idle_this_time = 0;
        charextra[ee].invorder_count = 0;
        charextra[ee].slow_move_counter = 0;
        charextra[ee].animwait = 0;
    }
    // multiply up gui positions
    guibg = (Bitmap **)malloc(sizeof(Bitmap *) * game.numgui);
    guibgbmp = (IDriverDependantBitmap**)malloc(sizeof(IDriverDependantBitmap*) * game.numgui);
    for (ee=0;ee<game.numgui;ee++) {
        guibg[ee] = NULL;
        guibgbmp[ee] = NULL;
    }

    our_eip=-5;
    for (ee=0;ee<game.numinvitems;ee++) {
        if (game.invinfo[ee].flags & IFLG_STARTWITH) playerchar->inv[ee]=1;
        else playerchar->inv[ee]=0;
    }
    play.score=0;
    play.sierra_inv_color=7;
    // copy the value set by the editor
    if (game.options[OPT_GLOBALTALKANIMSPD] >= 0)
    {
        play.talkanim_speed = game.options[OPT_GLOBALTALKANIMSPD];
        game.options[OPT_GLOBALTALKANIMSPD] = 1;
    }
    else
    {
        play.talkanim_speed = -game.options[OPT_GLOBALTALKANIMSPD] - 1;
        game.options[OPT_GLOBALTALKANIMSPD] = 0;
    }
    play.inv_item_wid = 40;
    play.inv_item_hit = 22;
    play.messagetime=-1;
    play.disabled_user_interface=0;
    play.gscript_timer=-1;
    play.debug_mode=game.options[OPT_DEBUGMODE];
    play.inv_top=0;
    play.inv_numdisp=0;
    play.obsolete_inv_numorder=0;
    play.text_speed=15;
    play.text_min_display_time_ms = 1000;
    play.ignore_user_input_after_text_timeout_ms = 500;
    play.ignore_user_input_until_time = 0;
    play.lipsync_speed = 15;
    play.close_mouth_speech_time = 10;
    play.disable_antialiasing = 0;
    play.rtint_enabled = false;
    play.rtint_level = 0;
    play.rtint_light = 0;
    play.text_speed_modifier = 0;
    play.text_align = SCALIGN_LEFT;
    // Make the default alignment to the right with right-to-left text
    if (game.options[OPT_RIGHTLEFTWRITE])
        play.text_align = SCALIGN_RIGHT;

    play.speech_bubble_width = get_fixed_pixel_size(100);
    play.bg_frame=0;
    play.bg_frame_locked=0;
    play.bg_anim_delay=0;
    play.anim_background_speed = 0;
    play.silent_midi = 0;
    play.current_music_repeating = 0;
    play.skip_until_char_stops = -1;
    play.get_loc_name_last_time = -1;
    play.get_loc_name_save_cursor = -1;
    play.restore_cursor_mode_to = -1;
    play.restore_cursor_image_to = -1;
    play.ground_level_areas_disabled = 0;
    play.next_screen_transition = -1;
    play.temporarily_turned_off_character = -1;
    play.inv_backwards_compatibility = 0;
    play.gamma_adjustment = 100;
    play.num_do_once_tokens = 0;
    play.do_once_tokens = NULL;
    play.music_queue_size = 0;
    play.shakesc_length = 0;
    play.wait_counter=0;
    play.key_skip_wait = 0;
    play.cur_music_number=-1;
    play.music_repeat=1;
    play.music_master_volume=100 + LegacyMusicMasterVolumeAdjustment;
    play.digital_master_volume = 100;
    play.screen_flipped=0;
    play.offsets_locked=0;
    play.cant_skip_speech = user_to_internal_skip_speech((SkipSpeechStyle)game.options[OPT_NOSKIPTEXT]);
    play.sound_volume = 255;
    play.speech_volume = 255;
    play.normal_font = 0;
    play.speech_font = 1;
    play.speech_text_shadow = 16;
    play.screen_tint = -1;
    play.bad_parsed_word[0] = 0;
    play.swap_portrait_side = 0;
    play.swap_portrait_lastchar = -1;
    play.swap_portrait_lastlastchar = -1;
    play.in_conversation = 0;
    play.skip_display = 3;
    play.no_multiloop_repeat = 0;
    play.in_cutscene = 0;
    play.fast_forward = 0;
    play.totalscore = game.totalscore;
    play.roomscript_finished = 0;
    play.no_textbg_when_voice = 0;
    play.max_dialogoption_width = get_fixed_pixel_size(180);
    play.no_hicolor_fadein = 0;
    play.bgspeech_game_speed = 0;
    play.bgspeech_stay_on_display = 0;
    play.unfactor_speech_from_textlength = 0;
    play.mp3_loop_before_end = 70;
    play.speech_music_drop = 60;
    play.room_changes = 0;
    play.check_interaction_only = 0;
    play.replay_hotkey = 318;  // Alt+R
    play.dialog_options_x = 0;
    play.dialog_options_y = 0;
    play.min_dialogoption_width = 0;
    play.disable_dialog_parser = 0;
    play.ambient_sounds_persist = 0;
    play.screen_is_faded_out = 0;
    play.player_on_region = 0;
    play.top_bar_backcolor = 8;
    play.top_bar_textcolor = 16;
    play.top_bar_bordercolor = 8;
    play.top_bar_borderwidth = 1;
    play.top_bar_ypos = 25;
    play.top_bar_font = -1;
    play.screenshot_width = 160;
    play.screenshot_height = 100;
    play.speech_text_align = SCALIGN_CENTRE;
    play.auto_use_walkto_points = 1;
    play.inventory_greys_out = 0;
    play.skip_speech_specific_key = 0;
    play.abort_key = 324;  // Alt+X
    play.fade_to_red = 0;
    play.fade_to_green = 0;
    play.fade_to_blue = 0;
    play.show_single_dialog_option = 0;
    play.keep_screen_during_instant_transition = 0;
    play.read_dialog_option_colour = -1;
    play.speech_portrait_placement = 0;
    play.speech_portrait_x = 0;
    play.speech_portrait_y = 0;
    play.speech_display_post_time_ms = 0;
    play.dialog_options_highlight_color = DIALOG_OPTIONS_HIGHLIGHT_COLOR_DEFAULT;
    play.speech_in_post_state = false;
    play.narrator_speech = game.playercharacter;
    play.crossfading_out_channel = 0;
    play.speech_textwindow_gui = game.options[OPT_TWCUSTOM];
    if (play.speech_textwindow_gui == 0)
        play.speech_textwindow_gui = -1;
    strcpy(play.game_name, game.gamename);
    play.lastParserEntry[0] = 0;
    play.follow_change_room_timer = 150;
    for (ee = 0; ee < MAX_BSCENE; ee++) 
        play.raw_modified[ee] = 0;
    play.game_speed_modifier = 0;
    if (debug_flags & DBG_DEBUGMODE)
        play.debug_mode = 1;
    gui_disabled_style = convert_gui_disabled_style(game.options[OPT_DISABLEOFF]);

    memset(&play.walkable_areas_on[0],1,MAX_WALK_AREAS+1);
    memset(&play.script_timers[0],0,MAX_TIMERS * sizeof(int));
    memset(&play.default_audio_type_volumes[0], -1, MAX_AUDIO_TYPES * sizeof(int));

    // reset graphical script vars (they're still used by some games)
    for (ee = 0; ee < MAXGLOBALVARS; ee++) 
        play.globalvars[ee] = 0;

    for (ee = 0; ee < MAXGLOBALSTRINGS; ee++)
        play.globalstrings[ee][0] = 0;

    for (ee = 0; ee < MAX_SOUND_CHANNELS; ee++)
        last_sound_played[ee] = -1;

    if (usetup.translation)
        init_translation (usetup.translation, "", true);

    update_invorder();
    displayed_room = -10;

    currentcursor=0;
    our_eip=-4;
    mousey=100;  // stop icon bar popping up
}

void engine_setup_scsystem_auxiliary()
{
    // ScriptSystem::aci_version is only 10 chars long
    strncpy(scsystem.aci_version, EngineVersion.LongString, 10);
    if (usetup.override_script_os >= 0)
    {
        scsystem.os = usetup.override_script_os;
    }
    else
    {
        scsystem.os = platform->GetSystemOSID();
    }
}

void engine_update_mp3_thread()
{
  update_mp3_thread();
  platform->Delay(50);
}

void engine_start_multithreaded_audio()
{
  // PSP: Initialize the sound cache.
  clear_sound_cache();

  // Create sound update thread. This is a workaround for sound stuttering.
  if (psp_audio_multithreaded)
  {
    if (!audioThread.CreateAndStart(engine_update_mp3_thread, true))
    {
      Debug::Printf(kDbgMsg_Init, "Failed to start audio thread, audio will be processed on the main thread");
      psp_audio_multithreaded = 0;
    }
    else
    {
      Debug::Printf(kDbgMsg_Init, "Audio thread started");
    }
  }
  else
  {
    Debug::Printf(kDbgMsg_Init, "Audio is processed on the main thread");
  }
}

void engine_prepare_to_start_game()
{
    Debug::Printf("Prepare to start game");

    engine_setup_scsystem_auxiliary();
    engine_start_multithreaded_audio();

#if defined(ANDROID_VERSION)
    if (psp_load_latest_savegame)
        selectLatestSavegame();
#endif
}

// TODO: move to test unit
Bitmap *test_allegro_bitmap;
IDriverDependantBitmap *test_allegro_ddb;
void allegro_bitmap_test_init()
{
	test_allegro_bitmap = NULL;
	// Switched the test off for now
	//test_allegro_bitmap = AllegroBitmap::CreateBitmap(320,200,32);
}

bool engine_read_config(const String &exe_path, ConfigTree &cfg)
{
    Debug::Printf(kDbgMsg_Init, "Reading configuration");

    config_defaults();
    // Don't read in the standard config file if disabled.
    if (psp_ignore_acsetup_cfg_file)
        return true;

    // Read default configuration file
    our_eip = -200;
    String def_cfg_file = find_default_cfg_file(exe_path);
    IniUtil::Read(def_cfg_file, cfg);

    read_game_data_location(cfg);

    // Deduce the game data file location
    if (!engine_init_game_data())
        return false;

    // Pre-load game name and savegame folder names from data file
    // TODO: research if that is possible to avoid this step and just
    // read the full head game data at this point. This might require
    // further changes of the order of initialization.
    String err_str;
    if (!preload_game_data(err_str))
    {
        display_game_file_error(err_str);
        return false;
    }

    // Read user global configuration file
    String user_global_cfg_file = find_user_global_cfg_file();
    if (Path::ComparePaths(user_global_cfg_file, def_cfg_file) != 0)
        IniUtil::Read(user_global_cfg_file, cfg);

    // Read user configuration file
    String user_cfg_file = find_user_cfg_file();
    if (Path::ComparePaths(user_cfg_file, def_cfg_file) != 0 &&
        Path::ComparePaths(user_cfg_file, user_global_cfg_file) != 0)
        IniUtil::Read(user_cfg_file, cfg);

    // TODO: override config tree with all the command-line args.
    // NOTE: at the moment AGS provide little means to determine whether an
    // option was overriden by command line, and since command line args
    // are applied first, we need to check if the option differs from
    // default before applying value from config file.
    if (disable_log_file)
        INIwriteint(cfg, "misc", "log", 0);
    else if (enable_log_file)
        INIwriteint(cfg, "misc", "log", 1);

    // Parse and set up game config
    read_config(cfg);

    // Fixup configuration after reading
    post_config();
    return true;
}

bool engine_do_config(int argc, char*argv[])
{
    ConfigTree cfg;
    if (!engine_read_config(argv[0], cfg))
        return false;

    apply_debug_config(cfg);

    return engine_check_run_setup(argv[0], cfg);
}

int initialize_engine(int argc,char*argv[])
{
    if (engine_pre_init_callback) {
        engine_pre_init_callback();
    }
    
    int res;
    if (!engine_init_allegro())
        return EXIT_NORMAL;

    if (!engine_do_config(argc, argv))
        return EXIT_NORMAL;

    engine_setup_allegro();

    engine_setup_window();

    our_eip = -196;

    engine_force_window();

    our_eip = -195;

    our_eip = -192;

    engine_init_fonts();

    our_eip = -188;

    res = engine_init_mouse();
	if (res != RETURN_CONTINUE) {
        return res;
    }

    our_eip = -187;

    res = engine_check_memory();
    if (res != RETURN_CONTINUE) {
        return res;
    }

    engine_init_rooms();

    our_eip = -186;
    
    res = engine_init_speech();
    if (res != RETURN_CONTINUE) {
        return res;
    }

    our_eip = -185;
    
    res = engine_init_music();
    if (res != RETURN_CONTINUE) {
        return res;
    }

    our_eip = -184;

    engine_init_keyboard();

    our_eip = -183;

    engine_init_timer();

    our_eip = -182;

    engine_init_sound();

    engine_init_debug();

    our_eip = -10;

    engine_init_exit_handler();

    // [IKM] I seriously don't get it why do we need to delete warnings.log
    // in the middle of procedure; some warnings may have already being
    // written there at this point, no?
    unlink("warnings.log");

    engine_init_rand();

    engine_init_pathfinder();

    //engine_pre_init_gfx();

    LOCK_VARIABLE(timerloop);
    LOCK_FUNCTION(dj_timer_handler);
    set_game_speed(40);

    our_eip=-20;
    //thisroom.allocall();
    our_eip=-19;
    //setup_sierra_interface();   // take this out later

    res = engine_load_game_data();
    if (res != RETURN_CONTINUE) {
        return res;
    }
    
    res = engine_check_register_game();
    if (res != RETURN_CONTINUE) {
        return res;
    }

    engine_init_title();

    our_eip = -189;

    engine_init_directories();

    our_eip = -178;

    res = engine_check_disk_space();
    if (res != RETURN_CONTINUE) {
        return res;
    }

    // Make sure that at least one font was loaded in the process of loading
    // the game data.
    // TODO: Fold this check into engine_load_game_data()
    res = engine_check_font_was_loaded();
    if (res != RETURN_CONTINUE) {
        return res;
    }

    our_eip = -179;

    engine_init_modxm_player();

    engine_init_resolution_settings(game.size);

    // Attempt to initialize graphics mode
    if (!engine_try_set_gfxmode_any(usetup.Screen))
        return EXIT_NORMAL;

    SetMultitasking(0);

    // [ER] 2014-03-13
    // Hide the system cursor via allegro
    show_os_cursor(MOUSE_CURSOR_NONE);

    engine_show_preload();

    res = engine_init_sprites();
    if (res != RETURN_CONTINUE) {
        return res;
    }

    engine_init_game_settings();

    engine_prepare_to_start_game();

	allegro_bitmap_test_init();

    initialize_start_and_play_game(override_start_room, loadSaveGameOnStartup);

    quit("|bye!");
    return 0;
}

bool engine_try_set_gfxmode_any(const ScreenSetup &setup)
{
    engine_shutdown_gfxmode();

    const Size init_desktop = get_desktop_size();

    ColorDepthOption color_depths;
    engine_get_color_depths(color_depths);
    if (!graphics_mode_init_any(game.size, setup, color_depths))
        return false;

    engine_post_gfxmode_setup(init_desktop);
    return true;
}

void engine_shutdown_gfxmode()
{
    if (!gfxDriver)
        return;

    engine_pre_gfxmode_shutdown();
    graphics_mode_shutdown();
}


#ifdef WINDOWS_VERSION
// in ac_minidump
extern int CustomExceptionHandler (LPEXCEPTION_POINTERS exinfo);
extern EXCEPTION_RECORD excinfo;
extern int miniDumpResultCode;
#endif

// defined in main/main
extern char tempmsg[100];
extern char*printfworkingspace;

int initialize_engine_with_exception_handling(int argc,char*argv[])
{
#ifdef USE_CUSTOM_EXCEPTION_HANDLER
    __try 
    {
        Debug::Printf(kDbgMsg_Init, "Installing exception handler");
#endif

        return initialize_engine(argc, argv);

#ifdef USE_CUSTOM_EXCEPTION_HANDLER
    }
    __except (CustomExceptionHandler ( GetExceptionInformation() )) 
    {
        strcpy (tempmsg, "");
        sprintf (printfworkingspace, "An exception 0x%X occurred in ACWIN.EXE at EIP = 0x%08X %s; program pointer is %+d, ACI version %s, gtags (%d,%d)\n\n"
            "AGS cannot continue, this exception was fatal. Please note down the numbers above, remember what you were doing at the time and post the details on the AGS Technical Forum.\n\n%s\n\n"
            "Most versions of Windows allow you to press Ctrl+C now to copy this entire message to the clipboard for easy reporting.\n\n%s (code %d)",
            excinfo.ExceptionCode, excinfo.ExceptionAddress, tempmsg, our_eip, EngineVersion.LongString.GetCStr(), eip_guinum, eip_guiobj, get_cur_script(5),
            (miniDumpResultCode == 0) ? "An error file CrashInfo.dmp has been created. You may be asked to upload this file when reporting this problem on the AGS Forums." : 
            "Unable to create an error dump file.", miniDumpResultCode);
        MessageBoxA(allegro_wnd, printfworkingspace, "Illegal exception", MB_ICONSTOP | MB_OK);
        proper_exit = 1;
    }
    return EXIT_CRASH;
#endif
}

const char *get_engine_version() {
    return EngineVersion.LongString.GetCStr();
}

void engine_set_pre_init_callback(t_engine_pre_init_callback callback) {
    engine_pre_init_callback = callback;
}
