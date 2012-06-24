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
// Engine initialization
//

#include "main/mainheader.h"
#include "acmain/ac_main.h"

#if defined(MAC_VERSION) || (defined(LINUX_VERSION) && !defined(PSP_VERSION))
#include <pthread.h>
pthread_t soundthread;
#endif

int errcod;
int initasx,initasy;
int firstDepth, secondDepth;

void engine_read_config(int argc,char*argv[])
{
    write_log_debug("Reading config file");

    our_eip = -200;
    read_config_file(argv[0]);

    set_uformat(U_ASCII);
}

int engine_init_allegro()
{
    write_log_debug("Initializing allegro");

    our_eip = -199;
    // Initialize allegro
#ifdef WINDOWS_VERSION
    if (install_allegro(SYSTEM_AUTODETECT,&myerrno,atexit)) {
        platform->DisplayAlert("Unable to initialize graphics subsystem. Make sure you have DirectX 5 or above installed.");
#else
    if (install_allegro(SYSTEM_AUTODETECT, &myerrno, atexit)) {
        platform->DisplayAlert("Unknown error initializing graphics subsystem.");
#endif
        return EXIT_NORMAL;
    }

    return RETURN_CONTINUE;
}

void engine_setup_window()
{
    write_log_debug("Setting up window");

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

int engine_check_run_setup(int argc,char*argv[])
{
#if !defined(LINUX_VERSION) && !defined(MAC_VERSION)
    // check if Setup needs to be run instead
    if (argc>1) {
        if (stricmp(argv[1],"--setup")==0) { 
            write_log_debug("Running Setup");

            if (!platform->RunSetup())
                return EXIT_NORMAL;

#ifndef WINDOWS_VERSION
#define _spawnl spawnl
#define _P_OVERLAY P_OVERLAY
#endif
            // Just re-reading the config file seems to cause a caching
            // problem on Win9x, so let's restart the process.
            allegro_exit();
            char quotedpath[255];
            sprintf (quotedpath, "\"%s\"", argv[0]);
            _spawnl (_P_OVERLAY, argv[0], quotedpath, NULL);
            //read_config_file(argv[0]);
        }
    }
#endif

    return RETURN_CONTINUE;
}

void engine_force_window()
{
    // Force to run in a window, override the config file
    if (force_window == 1)
        usetup.windowed = 1;
    else if (force_window == 2)
        usetup.windowed = 0;
}

int engine_init_game_data_external(int argc,char*argv[])
{
    game_file_name = ci_find_file(usetup.data_files_dir, usetup.main_data_filename);

#if !defined(WINDOWS_VERSION) && !defined(PSP_VERSION) && !defined(ANDROID_VERSION) && !defined(IOS_VERSION)
    // Search the exe files for the game data
    if ((game_file_name == NULL) || (access(game_file_name, F_OK) != 0))
    {
        DIR* fd = NULL;
        struct dirent* entry = NULL;
        version_info_t version_info;

        if ((fd = opendir(".")))
        {
            while ((entry = readdir(fd)))
            {
                // Exclude the setup program
                if (stricmp(entry->d_name, "winsetup.exe") == 0)
                    continue;

                // Filename must be >= 4 chars long
                int length = strlen(entry->d_name);
                if (length < 4)
                    continue;

                if (stricmp(&(entry->d_name[length - 4]), ".exe") == 0)
                {
                    if (!getVersionInformation(entry->d_name, &version_info))
                        continue;
                    if (strcmp(version_info.internal_name, "acwin") == 0)
                    {
                        game_file_name = (char*)malloc(strlen(entry->d_name) + 1);
                        strcpy(game_file_name, entry->d_name);
                        break;
                    }
                }
            }
            closedir(fd);
        }
    }
#endif

    errcod=csetlib(game_file_name,"");
    if (errcod) {
        //sprintf(gamefilenamebuf,"%s\\ac2game.ags",usetup.data_files_dir);
        free(game_file_name);
        game_file_name = ci_find_file(usetup.data_files_dir, "ac2game.ags");

        errcod = csetlib(game_file_name,"");
    }

    return RETURN_CONTINUE;
}

int engine_init_game_data_internal(int argc,char*argv[])
{
    usetup.main_data_filename = get_filename(game_file_name);

    if (((strchr(game_file_name, '/') != NULL) ||
        (strchr(game_file_name, '\\') != NULL)) &&
        (stricmp(usetup.data_files_dir, ".") == 0)) {
            // there is a path in the game file name (and the user
            // has not specified another one)
            // save the path, so that it can load the VOX files, etc
            usetup.data_files_dir = (char*)malloc(strlen(game_file_name) + 1);
            strcpy(usetup.data_files_dir, game_file_name);

            if (strrchr(usetup.data_files_dir, '/') != NULL)
                strrchr(usetup.data_files_dir, '/')[0] = 0;
            else if (strrchr(usetup.data_files_dir, '\\') != NULL)
                strrchr(usetup.data_files_dir, '\\')[0] = 0;
            else {
                platform->DisplayAlert("Error processing game file name: slash but no slash");
                return EXIT_NORMAL;
            }
    }

    return RETURN_CONTINUE;
}

int engine_init_game_data(int argc,char*argv[])
{
    write_log_debug("Initializing game data");

    // initialize the data file
    initialise_game_file_name();
    if (game_file_name == NULL) return EXIT_NORMAL;

    errcod = csetlib(game_file_name,"");  // assume it's appended to exe

    our_eip = -194;
    //  char gamefilenamebuf[200];

    int init_res = RETURN_CONTINUE;

    if ((errcod!=0) && (change_to_game_dir == 0)) {
        // it's not, so look for the file
        init_res = engine_init_game_data_external(argc, argv);
    }
    else {
        // set the data filename to the EXE name
        int res = engine_init_game_data_internal(argc, argv);
    }

    if (init_res != RETURN_CONTINUE) {
        return init_res;
    }

    our_eip = -193;

    if (errcod!=0) {  // there's a problem
        if (errcod==-1) {  // file not found
            char emsg[STD_BUFFER_SIZE];
            sprintf (emsg,
                "You must create and save a game first in the AGS Editor before you can use "
                "this engine.\n\n"
                "If you have just downloaded AGS, you are probably running the wrong executable.\n"
                "Run AGSEditor.exe to launch the editor.\n\n"
                "(Unable to find '%s')\n", argv[datafile_argv]);
            platform->DisplayAlert(emsg);
        }
        else if (errcod==-4)
            platform->DisplayAlert("ERROR: Too many files in data file.");
        else platform->DisplayAlert("ERROR: The file is corrupt. Make sure you have the correct version of the\n"
            "editor, and that this really is an AGS game.\n");
        return EXIT_NORMAL;
    }

    return RETURN_CONTINUE;
}

void engine_init_fonts()
{
    write_log_debug("Initializing TTF renderer");

    init_font_renderer();
}

void engine_init_mouse()
{
    write_log_debug("Initializing mouse");

#ifdef _DEBUG
    // Quantify fails with the mouse for some reason
    minstalled();
#else
    if (minstalled()==0) {
        platform->DisplayAlert(platform->GetNoMouseErrorString());
        return EXIT_NORMAL;
    }
#endif // DEBUG
}

int engine_check_memory()
{
    write_log_debug("Checking memory");

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
    write_log_debug("Initializing rooms");

    roomstats=(RoomStatus*)calloc(sizeof(RoomStatus),MAX_ROOMS);
    for (int ee=0;ee<MAX_ROOMS;ee++) {
        roomstats[ee].beenhere=0;
        roomstats[ee].numobj=0;
        roomstats[ee].tsdatasize=0;
        roomstats[ee].tsdata=NULL;
    }
}

int engine_init_speech()
{
    play.want_speech=-2;

    FILE*ppp;

    if (usetup.no_speech_pack == 0) {
        /* Can't just use fopen here, since we need to change the filename
        so that pack functions, etc. will have the right case later */
        speech_file = ci_find_file(usetup.data_files_dir, "speech.vox");

        ppp = ci_fopen(speech_file, "rb");

        if (ppp == NULL)
        {
            // In case they're running in debug, check Compiled folder
            free(speech_file);
            speech_file = ci_find_file("Compiled", "speech.vox");
            ppp = ci_fopen(speech_file, "rb");
        }

        if (ppp!=NULL) {
            fclose(ppp);

            write_log_debug("Initializing speech vox");

            //if (csetlib(useloc,"")!=0) {
            if (csetlib(speech_file,"")!=0) {
                platform->DisplayAlert("Unable to initialize speech sample file - check for corruption and that\nit belongs to this game.\n");
                return EXIT_NORMAL;
            }
            FILE *speechsync = clibfopen("syncdata.dat", "rb");
            if (speechsync != NULL) {
                // this game has voice lip sync
                if (getw(speechsync) != 4)
                { 
                    // Don't display this warning.
                    // platform->DisplayAlert("Unknown speech lip sync format (might be from older or newer version); lip sync disabled");
                }
                else {
                    numLipLines = getw(speechsync);
                    splipsync = (SpeechLipSyncLine*)malloc (sizeof(SpeechLipSyncLine) * numLipLines);
                    for (int ee = 0; ee < numLipLines; ee++)
                    {
                        splipsync[ee].numPhenomes = getshort(speechsync);
                        fread(splipsync[ee].filename, 1, 14, speechsync);
                        splipsync[ee].endtimeoffs = (int*)malloc(splipsync[ee].numPhenomes * sizeof(int));
                        fread(splipsync[ee].endtimeoffs, sizeof(int), splipsync[ee].numPhenomes, speechsync);
                        splipsync[ee].frame = (short*)malloc(splipsync[ee].numPhenomes * sizeof(short));
                        fread(splipsync[ee].frame, sizeof(short), splipsync[ee].numPhenomes, speechsync);
                    }
                }
                fclose (speechsync);
            }
            csetlib(game_file_name,"");
            platform->WriteConsole("Speech sample file found and initialized.\n");
            play.want_speech=1;
        }
    }

    return RETURN_CONTINUE;
}

int engine_init_music()
{
    FILE*ppp;
    play.seperate_music_lib = 0;

    /* Can't just use fopen here, since we need to change the filename
    so that pack functions, etc. will have the right case later */
    music_file = ci_find_file(usetup.data_files_dir, "audio.vox");

    /* Don't need to use ci_fopen here, because we've used ci_find_file to get
    the case insensitive matched filename already */
    // Use ci_fopen anyway because it can handle NULL filenames.
    ppp = ci_fopen(music_file, "rb");

    if (ppp == NULL)
    {
        // In case they're running in debug, check Compiled folder
        free(music_file);
        music_file = ci_find_file("Compiled", "audio.vox");
        ppp = ci_fopen(music_file, "rb");
    }

    if (ppp!=NULL) {
        fclose(ppp);

        write_log_debug("Initializing audio vox");

        //if (csetlib(useloc,"")!=0) {
        if (csetlib(music_file,"")!=0) {
            platform->DisplayAlert("Unable to initialize music library - check for corruption and that\nit belongs to this game.\n");
            return EXIT_NORMAL;
        }
        csetlib(game_file_name,"");
        platform->WriteConsole("Audio vox found and initialized.\n");
        play.seperate_music_lib = 1;
    }

    return RETURN_CONTINUE;
}

void engine_init_keyboard()
{
#ifdef ALLEGRO_KEYBOARD_HANDLER
    write_log_debug("Initializing keyboard");

    install_keyboard();
#endif
}

void engine_init_timer()
{
    write_log_debug("Install timer");

    platform->WriteConsole("Checking sound inits.\n");
    if (opts.mod_player) reserve_voices(16,-1);
    // maybe this line will solve the sound volume?
    // [IKM] does this refer to install_timer or set_volume_per_voice?
    install_timer();
#if ALLEGRO_DATE > 19991010
    set_volume_per_voice(1);
#endif
}

void engine_init_sound()
{
#ifdef WINDOWS_VERSION
    // don't let it use the hardware mixer verion, crashes some systems
    //if ((usetup.digicard == DIGI_AUTODETECT) || (usetup.digicard == DIGI_DIRECTX(0)))
    //    usetup.digicard = DIGI_DIRECTAMX(0);

    if (usetup.digicard == DIGI_DIRECTX(0)) {
        // DirectX mixer seems to buffer an extra sample itself
        use_extra_sound_offset = 1;
    }

    // if the user clicked away to another app while we were
    // loading, DirectSound will fail to initialize. There doesn't
    // seem to be a solution to force us back to the foreground,
    // because we have no actual visible window at this time

#endif

    write_log_debug("Initialize sound drivers");

    // PSP: Disable sound by config file.
    if (!psp_audio_enabled)
    {
        usetup.digicard = DIGI_NONE;
        usetup.midicard = MIDI_NONE;
    }

    if (!psp_midi_enabled)
        usetup.midicard = MIDI_NONE;

    if (install_sound(usetup.digicard,usetup.midicard,NULL)!=0) {
        reserve_voices(-1,-1);
        opts.mod_player=0;
        opts.mp3_player=0;
        if (install_sound(usetup.digicard,usetup.midicard,NULL)!=0) {
            if ((usetup.digicard != DIGI_NONE) && (usetup.midicard != MIDI_NONE)) {
                // only flag an error if they wanted a sound card
                platform->DisplayAlert("\nUnable to initialize your audio hardware.\n"
                    "[Problem: %s]\n",allegro_error);
            }
            reserve_voices(0,0);
            install_sound(DIGI_NONE, MIDI_NONE, NULL);
            usetup.digicard = DIGI_NONE;
            usetup.midicard = MIDI_NONE;
        }
    }

    our_eip = -181;

    if (usetup.digicard == DIGI_NONE) {
        // disable speech and music if no digital sound
        // therefore the MIDI soundtrack will be used if present,
        // and the voice mode should not go to Voice Only
        play.want_speech = -2;
        play.seperate_music_lib = 0;
    }
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

void engine_init_exit_handler()
{
    write_log_debug("Install exit handler");

    atexit(atexit_handler);
}

void engine_init_rand()
{
    play.randseed = time(NULL);
    srand (play.randseed);
}

void engine_init_pathfinder()
{
    write_log_debug("Initialize path finder library");

    init_pathfinder();
}

void engine_pre_init_gfx()
{
    write_log_debug("Initialize gfx");

    platform->InitialiseAbufAtStartup();
}

int engine_load_game_data()
{
    write_log_debug("Load game data");

    our_eip=-17;
    int ee=load_game_file();
    if (ee != 0) {
        proper_exit=1;
        platform->FinishedUsingGraphicsMode();

        if (ee==-1)
            platform->DisplayAlert("Main game file not found. This may be from a different AGS version, or the file may have got corrupted.\n");
        else if (ee==-2)
            platform->DisplayAlert("Invalid file format. The file may be corrupt, or from a different\n"
            "version of AGS.\nThis engine can only run games made with AGS 2.5 or later.\n");
        else if (ee==-3)
            platform->DisplayAlert("Script link failed: %s\n",ccErrorString);
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

    write_log_debug(game.gamename);
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

    if (game.saveGameFolderName[0] != 0)
    {
        char newDirBuffer[MAX_PATH];
        sprintf(newDirBuffer, "$MYDOCS$/%s", game.saveGameFolderName);
        Game_SetSaveGameDirectory(newDirBuffer);
    }
    else if (use_compiled_folder_as_current_dir)
    {
        Game_SetSaveGameDirectory("Compiled");
    }
}

int engine_check_disk_space()
{
    write_log_debug("Checking for disk space");

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

// [IKM] I have a feeling this should be merged with engine_init_fonts
int engine_check_fonts()
{
    if (fontRenderers[0] == NULL) 
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
        write_log_debug("Initializing MOD/XM player");

        if (init_mod_player(NUM_MOD_DIGI_VOICES) < 0) {
            platform->DisplayAlert("Warning: install_mod: MOD player failed to initialize.");
            opts.mod_player=0;
        }
    }
#else
    opts.mod_player = 0;
    write_log_debug("Compiled without MOD/XM player");
#endif
}

void engine_init_screen_settings()
{
    write_log_debug("Initializing screen settings");

    // default shifts for how we store the sprite data

#if defined(PSP_VERSION)
    // PSP: Switch b<>r for 15/16 bit.
    _rgb_r_shift_32 = 16;
    _rgb_g_shift_32 = 8;
    _rgb_b_shift_32 = 0;
    _rgb_b_shift_16 = 11;
    _rgb_g_shift_16 = 5;
    _rgb_r_shift_16 = 0;
    _rgb_b_shift_15 = 10;
    _rgb_g_shift_15 = 5;
    _rgb_r_shift_15 = 0;
#else
    _rgb_r_shift_32 = 16;
    _rgb_g_shift_32 = 8;
    _rgb_b_shift_32 = 0;
    _rgb_r_shift_16 = 11;
    _rgb_g_shift_16 = 5;
    _rgb_b_shift_16 = 0;
    _rgb_r_shift_15 = 10;
    _rgb_g_shift_15 = 5;
    _rgb_b_shift_15 = 0;
#endif

    usetup.base_width = 320;
    usetup.base_height = 200;

    if (game.default_resolution >= 5)
    {
        if (game.default_resolution >= 6)
        {
            // 1024x768
            usetup.base_width = 512;
            usetup.base_height = 384;
        }
        else
        {
            // 800x600
            usetup.base_width = 400;
            usetup.base_height = 300;
        }
        // don't allow letterbox mode
        game.options[OPT_LETTERBOX] = 0;
        force_letterbox = 0;
        scrnwid = usetup.base_width * 2;
        scrnhit = usetup.base_height * 2;
        wtext_multiply = 2;
    }
    else if ((game.default_resolution == 4) ||
        (game.default_resolution == 3))
    {
        scrnwid = 640;
        scrnhit = 400;
        wtext_multiply = 2;
    }
    else if ((game.default_resolution == 2) ||
        (game.default_resolution == 1))
    {
        scrnwid = 320;
        scrnhit = 200;
        wtext_multiply = 1;
    }
    else
    {
        scrnwid = usetup.base_width;
        scrnhit = usetup.base_height;
        wtext_multiply = 1;
    }

    usetup.textheight = wgetfontheight(0) + 1;

    vesa_xres=scrnwid; vesa_yres=scrnhit;
    //scrnwto=scrnwid-1; scrnhto=scrnhit-1;
    current_screen_resolution_multiplier = scrnwid / BASEWIDTH;

    if ((game.default_resolution > 2) &&
        (game.options[OPT_NATIVECOORDINATES]))
    {
        usetup.base_width *= 2;
        usetup.base_height *= 2;
    }

    initasx=scrnwid,initasy=scrnhit;
    if (scrnwid==960) { initasx=1024; initasy=768; }

    // save this setting so we only do 640x480 full-screen if they want it
    usetup.want_letterbox = game.options[OPT_LETTERBOX];

    if (force_letterbox > 0)
        game.options[OPT_LETTERBOX] = 1;

    // PSP: Don't letterbox a 320x200 screen.
    if ((game.default_resolution != 2) && (game.default_resolution != 4))
        force_letterbox = usetup.want_letterbox = game.options[OPT_LETTERBOX] = 0;		

    // don't allow them to force a 256-col game to hi-color
    if (game.color_depth < 2)
        usetup.force_hicolor_mode = 0;

    firstDepth = 8, secondDepth = 8;
    if ((game.color_depth == 2) || (force_16bit) || (usetup.force_hicolor_mode)) {
        firstDepth = 16;
        secondDepth = 15;
    }
    else if (game.color_depth > 2) {
        firstDepth = 32;
        secondDepth = 24;
    }

    adjust_sizes_for_resolution(loaded_game_file_version);
}

int engine_init_gfx_filters()
{
    write_log_debug("Init gfx filters");

    if (initialize_graphics_filter(usetup.gfxFilterID, initasx, initasy, firstDepth))
    {
        return EXIT_NORMAL;
    }

    return RETURN_CONTINUE;
}

void engine_init_gfx_driver()
{
    write_log_debug("Init gfx driver");

    create_gfx_driver();
}

int engine_init_graphics_mode()
{
    write_log_debug("Switching to graphics mode");

    if (switch_to_graphics_mode(initasx, initasy, scrnwid, scrnhit, firstDepth, secondDepth))
    {
        bool errorAndExit = true;

        if (((usetup.gfxFilterID == NULL) || 
            (stricmp(usetup.gfxFilterID, "None") == 0)) &&
            (scrnwid == 320))
        {
            // If the game is 320x200 and no filter is being used, try using a 2x
            // filter automatically since many gfx drivers don't suport 320x200.
            write_log_debug("320x200 not supported, trying with 2x filter");
            delete filter;

            if (initialize_graphics_filter("StdScale2", initasx, initasy, firstDepth)) 
            {
                return EXIT_NORMAL;
            }

            create_gfx_driver();

            if (!switch_to_graphics_mode(initasx, initasy, scrnwid, scrnhit, firstDepth, secondDepth))
            {
                errorAndExit = false;
            }

        }

        if (errorAndExit)
        {
            proper_exit=1;
            platform->FinishedUsingGraphicsMode();

            // make sure the error message displays the true resolution
            if (game.options[OPT_LETTERBOX])
                initasy = (initasy * 12) / 10;

            if (filter != NULL)
                filter->GetRealResolution(&initasx, &initasy);

            platform->DisplayAlert("There was a problem initializing graphics mode %d x %d (%d-bit).\n"
                "(Problem: '%s')\n"
                "Try to correct the problem, or seek help from the AGS homepage.\n"
                "\nPossible causes:\n* your graphics card drivers do not support this resolution. "
                "Run the game setup program and try the other resolution.\n"
                "* the graphics driver you have selected does not work. Try switching between Direct3D and DirectDraw.\n"
                "* the graphics filter you have selected does not work. Try another filter.",
                initasx, initasy, firstDepth, allegro_error);
            return EXIT_NORMAL;
        }
    }

    return RETURN_CONTINUE;
}

void engine_post_init_gfx_driver()
{
    //screen = _filter->ScreenInitialized(screen, final_scrn_wid, final_scrn_hit);
    _old_screen = screen;

    if (gfxDriver->HasAcceleratedStretchAndFlip()) 
    {
        walkBehindMethod = DrawAsSeparateSprite;

        CreateBlankImage();
    }
}

void engine_prepare_screen()
{
    write_log_debug("Preparing graphics mode screen");

    if ((final_scrn_hit != scrnhit) || (final_scrn_wid != scrnwid)) {
        initasx = final_scrn_wid;
        initasy = final_scrn_hit;
        clear(_old_screen);
        screen = create_sub_bitmap(_old_screen, initasx / 2 - scrnwid / 2, initasy/2-scrnhit/2, scrnwid, scrnhit);
        _sub_screen=screen;

        scrnhit = screen->h;
        vesa_yres = screen->h;
        scrnwid = screen->w;
        vesa_xres = screen->w;
        gfxDriver->SetMemoryBackBuffer(screen);

        platform->WriteDebugString("Screen resolution: %d x %d; game resolution %d x %d", _old_screen->w, _old_screen->h, scrnwid, scrnhit);
    }


    // Most cards do 5-6-5 RGB, which is the format the files are saved in
    // Some do 5-6-5 BGR, or  6-5-5 RGB, in which case convert the gfx
    if ((final_col_dep == 16) && ((_rgb_b_shift_16 != 0) || (_rgb_r_shift_16 != 11))) {
        convert_16bit_bgr = 1;
        if (_rgb_r_shift_16 == 10) {
            // some very old graphics cards lie about being 16-bit when they
            // are in fact 15-bit ... get around this
            _places_r = 3;
            _places_g = 3;
        }
    }
    if (final_col_dep > 16) {
        // when we're using 32-bit colour, it converts hi-color images
        // the wrong way round - so fix that

#if defined(IOS_VERSION) || defined(ANDROID_VERSION) || defined(PSP_VERSION)
        _rgb_b_shift_16 = 0;
        _rgb_g_shift_16 = 5;
        _rgb_r_shift_16 = 11;

        _rgb_b_shift_15 = 0;
        _rgb_g_shift_15 = 5;
        _rgb_r_shift_15 = 10;

        _rgb_r_shift_32 = 0;
        _rgb_g_shift_32 = 8;
        _rgb_b_shift_32 = 16;
#else
        _rgb_r_shift_16 = 11;
        _rgb_g_shift_16 = 5;
        _rgb_b_shift_16 = 0;
#endif
    }
    else if (final_col_dep == 16) {
        // ensure that any 32-bit graphics displayed are converted
        // properly to the current depth
#if defined(PSP_VERSION)
        _rgb_r_shift_32 = 0;
        _rgb_g_shift_32 = 8;
        _rgb_b_shift_32 = 16;

        _rgb_b_shift_15 = 0;
        _rgb_g_shift_15 = 5;
        _rgb_r_shift_15 = 10;
#else
        _rgb_r_shift_32 = 16;
        _rgb_g_shift_32 = 8;
        _rgb_b_shift_32 = 0;
#endif
    }
    else if (final_col_dep < 16) {
        // ensure that any 32-bit graphics displayed are converted
        // properly to the current depth
#if defined(IOS_VERSION) || defined(ANDROID_VERSION) || defined(PSP_VERSION)
        _rgb_r_shift_32 = 0;
        _rgb_g_shift_32 = 8;
        _rgb_b_shift_32 = 16;

        _rgb_b_shift_15 = 0;
        _rgb_g_shift_15 = 5;
        _rgb_r_shift_15 = 10;
#else
        _rgb_r_shift_32 = 16;
        _rgb_g_shift_32 = 8;
        _rgb_b_shift_32 = 0;
#endif
    }
}

void engine_set_gfx_driver_callbacks()
{
    gfxDriver->SetCallbackForPolling(update_polled_stuff_if_runtime);
    gfxDriver->SetCallbackToDrawScreen(draw_screen_callback);
    gfxDriver->SetCallbackForNullSprite(GfxDriverNullSpriteCallback);
}

void engine_set_color_conversions()
{
    write_log_debug("Initializing colour conversion");

    set_color_conversion(COLORCONV_MOST | COLORCONV_EXPAND_256 | COLORCONV_REDUCE_16_TO_15);
}

void engine_show_preload()
{
    write_log_debug("Check for preload image");

    show_preload ();
}

int engine_init_sprites()
{
    write_log_debug("Initialize sprites");

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

void engine_setup_screen()
{
    write_log_debug("Set up screen");

    virtual_screen=create_bitmap_ex(final_col_dep,scrnwid,scrnhit);
    clear(virtual_screen);
    gfxDriver->SetMemoryBackBuffer(virtual_screen);
    //  ignore_mouseoff_bitmap = virtual_screen;
    abuf=screen;
    our_eip=-7;

    for (int ee = 0; ee < MAX_INIT_SPR + game.numcharacters; ee++)
        actsps[ee] = NULL;
}

void engine_init_game_settings()
{
    write_log_debug("Initialize game settings");

    init_game_settings();
}

void engine_init_game_shit()
{
    scsystem.width = final_scrn_wid;
    scsystem.height = final_scrn_hit;
    scsystem.coldepth = final_col_dep;
    scsystem.windowed = 0;
    scsystem.vsync = 0;
    scsystem.viewport_width = divide_down_coordinate(scrnwid);
    scsystem.viewport_height = divide_down_coordinate(scrnhit);
    strcpy(scsystem.aci_version, ACI_VERSION_TEXT);
    scsystem.os = platform->GetSystemOSID();

    if (usetup.windowed)
        scsystem.windowed = 1;

#if defined(WINDOWS_VERSION) || defined(LINUX_VERSION) || defined(MAC_VERSION)
    filter->SetMouseArea(0, 0, scrnwid-1, scrnhit-1);
#else
    filter->SetMouseArea(0,0,BASEWIDTH-1,BASEHEIGHT-1);
#endif
    //  mloadwcursor("mouse.spr");
    //mousecurs[0]=spriteset[2054];
    currentcursor=0;
    our_eip=-4;
    mousey=100;  // stop icon bar popping up
    init_invalid_regions(final_scrn_hit);
    wsetscreen(virtual_screen);
    our_eip = -41;

    gfxDriver->SetRenderOffset(get_screen_x_adjustment(virtual_screen), get_screen_y_adjustment(virtual_screen));
}

void engine_start_multithreaded_audio()
{
    // PSP: Initialize the sound cache.
    clear_sound_cache();

    // Create sound update thread. This is a workaround for sound stuttering.
    if (psp_audio_multithreaded)
    {
#if defined(PSP_VERSION)
        update_mp3_thread_running = true;
        SceUID thid = sceKernelCreateThread("update_mp3_thread", update_mp3_thread, 0x20, 0xFA0, THREAD_ATTR_USER, 0);
        if (thid > -1)
            thid = sceKernelStartThread(thid, 0, 0);
        else
        {
            update_mp3_thread_running = false;
            psp_audio_multithreaded = 0;
        }
#elif (defined(LINUX_VERSION) && !defined(PSP_VERSION)) || defined(MAC_VERSION)
        update_mp3_thread_running = true;
        if (pthread_create(&soundthread, NULL, update_mp3_thread, NULL) != 0)
        {
            update_mp3_thread_running = false;
            psp_audio_multithreaded = 0;
        }
#elif defined(WINDOWS_VERSION)
        update_mp3_thread_running = true;
        if (CreateThread(NULL, 0, update_mp3_thread, NULL, 0, NULL) == NULL)
        {
            update_mp3_thread_running = false;
            psp_audio_multithreaded = 0;
        }
#endif
    }
}

void engine_prepare_to_start_game()
{
    write_log_debug("Prepare to start game");

    engine_init_game_shit();
    engine_start_multithreaded_audio();

#if defined(ANDROID_VERSION)
    if (psp_load_latest_savegame)
        selectLatestSavegame();
#endif
}

int initialize_engine(int argc,char*argv[])
{
    int res;

    engine_read_config(argc, argv);
    res = engine_init_allegro();
    if (res != RETURN_CONTINUE) {
        return res;
    }    

    engine_setup_window();

    our_eip = -196;

    res = engine_check_run_setup(argc, argv);
    if (res != RETURN_CONTINUE) {
        return res;
    }

    engine_force_window();

    our_eip = -195;

    res = engine_init_game_data(argc, argv);
    if (res != RETURN_CONTINUE) {
        return res;
    }

    our_eip = -192;

    engine_init_fonts();

    our_eip = -188;

    engine_init_mouse();

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

    unlink("warnings.log");

    engine_init_rand();

    engine_init_pathfinder();

    engine_pre_init_gfx();

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

    // [IKM] I do not really understand why is this checked only now;
    // should not it be checked right after fonts initialization?
    res = engine_check_fonts();
    if (res != RETURN_CONTINUE) {
        return res;
    }

    our_eip = -179;

    engine_init_modxm_player();

    engine_init_screen_settings();

    res = engine_init_gfx_filters();
    if (res != RETURN_CONTINUE) {
        return res;
    }

    engine_init_gfx_driver();

    res = engine_init_graphics_mode();
    if (res != RETURN_CONTINUE) {
        return res;
    }

    engine_post_init_gfx_driver();

    engine_prepare_screen();

    platform->PostAllegroInit((usetup.windowed > 0) ? true : false);

    engine_set_gfx_driver_callbacks();

    engine_set_color_conversions();

    SetMultitasking(0);

    engine_show_preload();

    res = engine_init_sprites();
    if (res != RETURN_CONTINUE) {
        return res;
    }

    engine_setup_screen();

    engine_init_game_settings();

    engine_prepare_to_start_game();

    initialize_start_and_play_game(override_start_room, loadSaveGameOnStartup);

    update_mp3_thread_running = false;

    quit("|bye!");
    return 0;
}


#ifdef WINDOWS_VERSION
// in ac_minidump
extern int CustomExceptionHandler (LPEXCEPTION_POINTERS exinfo);
extern EXCEPTION_RECORD excinfo;
extern int miniDumpResultCode;
#endif

int initialize_engine_with_exception_handling(int argc,char*argv[])
{
    write_log_debug("Installing exception handler");

#ifdef USE_CUSTOM_EXCEPTION_HANDLER
    __try 
    {
#endif

        return initialize_engine(argc, argv);

#ifdef USE_CUSTOM_EXCEPTION_HANDLER
    }
    __except (CustomExceptionHandler ( GetExceptionInformation() )) 
    {
        strcpy (tempmsg, "");
        sprintf (printfworkingspace, "An exception 0x%X occurred in ACWIN.EXE at EIP = 0x%08X %s; program pointer is %+d, ACI version " ACI_VERSION_TEXT ", gtags (%d,%d)\n\n"
            "AGS cannot continue, this exception was fatal. Please note down the numbers above, remember what you were doing at the time and post the details on the AGS Technical Forum.\n\n%s\n\n"
            "Most versions of Windows allow you to press Ctrl+C now to copy this entire message to the clipboard for easy reporting.\n\n%s (code %d)",
            excinfo.ExceptionCode, excinfo.ExceptionAddress, tempmsg, our_eip, eip_guinum, eip_guiobj, get_cur_script(5),
            (miniDumpResultCode == 0) ? "An error file CrashInfo.dmp has been created. You may be asked to upload this file when reporting this problem on the AGS Forums." : 
            "Unable to create an error dump file.", miniDumpResultCode);
        MessageBoxA(allegro_wnd, printfworkingspace, "Illegal exception", MB_ICONSTOP | MB_OK);
        proper_exit = 1;
    }
    return EXIT_CRASH;
#endif
}
