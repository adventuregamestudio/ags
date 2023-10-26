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

#include "core/platform.h"

#include <errno.h>
#include <stdio.h>
#include <stdexcept>
#if AGS_PLATFORM_OS_WINDOWS
#include <process.h>  // _spawnl
#endif
#include <allegro.h> // allegro_install and _exit

#include "ac/asset_helper.h"
#include "ac/common.h"
#include "ac/character.h"
#include "ac/characterextras.h"
#include "ac/characterinfo.h"
#include "ac/draw.h"
#include "ac/game.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_character.h"
#include "ac/global_game.h"
#include "ac/gui.h"
#include "ac/lipsync.h"
#include "ac/path_helper.h"
#include "ac/route_finder.h"
#include "ac/sys_events.h"
#include "ac/roomstatus.h"
#include "ac/speech.h"
#include "ac/spritecache.h"
#include "ac/translation.h"
#include "ac/viewframe.h"
#include "ac/dynobj/scriptobject.h"
#include "ac/dynobj/scriptsystem.h"
#include "core/assetmanager.h"
#include "debug/debug_log.h"
#include "debug/debugger.h"
#include "debug/out.h"
#include "device/mousew32.h"
#include "font/agsfontrenderer.h"
#include "font/fonts.h"
#include "gfx/graphicsdriver.h"
#include "gfx/gfxdriverfactory.h"
#include "gfx/ddb.h"
#include "media/audio/sound.h"
#include "main/config.h"
#include "main/game_file.h"
#include "main/game_start.h"
#include "main/engine.h"
#include "main/engine_setup.h"
#include "main/graphics_mode.h"
#include "main/main.h"
#include "media/audio/audio_core.h"
#include "platform/base/sys_main.h"
#include "platform/base/agsplatformdriver.h"
#include "script/script_runtime.h"
#include "util/directory.h"
#include "util/error.h"
#include "util/path.h"
#include "util/string_utils.h"
#include "ac/joystick.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern char check_dynamic_sprites_at_exit;
extern int our_eip;
extern volatile bool want_exit, abort_engine;
extern bool justRunSetup;
extern GameSetupStruct game;
extern int proper_exit;
extern char pexbuf[STD_BUFFER_SIZE];
extern SpriteCache spriteset;
extern ScriptObject scrObj[MAX_ROOM_OBJECTS];
extern std::vector<ViewStruct> views;
extern int displayed_room;
extern int eip_guinum;
extern int eip_guiobj;
extern int numLipLines, curLipLine, curLipLinePhoneme;
extern ScriptSystem scsystem;
extern IGraphicsDriver *gfxDriver;
extern RGB palette[256];
extern CharacterInfo*playerchar;

ResourcePaths ResPaths;

t_engine_pre_init_callback engine_pre_init_callback = nullptr;

bool engine_init_backend()
{
    our_eip = -199;
    platform->PreBackendInit();
    // Initialize SDL
    Debug::Printf(kDbgMsg_Info, "Initializing backend libs");
    if (sys_main_init())
    {
        const char *err = SDL_GetError();
        const char *user_hint = platform->GetBackendFailUserHint();
        platform->DisplayAlert("Unable to initialize SDL library.\n%s\n\n%s",
            (err && err[0]) ? err : "SDL provided no further information on the problem.",
            user_hint);
        return false;
    }
    
    // Initialize stripped allegro library
    if (install_allegro(SYSTEM_NONE, &errno, atexit))
    {
        platform->DisplayAlert("Internal error: unable to initialize stripped Allegro 4 library.");
        return false;
    }

    platform->PostBackendInit();
    return true;
}

void winclosehook()
{
    want_exit = true;
    abort_engine = true;
    check_dynamic_sprites_at_exit = 0;
    AbortGame();
}

void engine_setup_window()
{
    Debug::Printf(kDbgMsg_Info, "Setting up window");

    our_eip = -198;
    sys_window_set_title(game.gamename);
    sys_window_set_icon();
    sys_evt_set_quit_callback(winclosehook);
    our_eip = -197;
}

// Fills map with game settings, to e.g. let setup application(s)
// display correct properties to the user
static void fill_game_properties(StringOrderMap &map)
{
    map["title"] = game.gamename;
    map["guid"] = game.guid;
    map["legacy_uniqueid"] = StrUtil::IntToString(game.uniqueid);
    map["resolution_width"] = StrUtil::IntToString(game.GetGameRes().Width);
    map["resolution_height"] = StrUtil::IntToString(game.GetGameRes().Height);
    map["resolution_bpp"] = StrUtil::IntToString(game.GetColorDepth());
    map["render_at_screenres"] = StrUtil::IntToString(
        game.options[OPT_RENDERATSCREENRES] == kRenderAtScreenRes_UserDefined ? -1 :
        (game.options[OPT_RENDERATSCREENRES] == kRenderAtScreenRes_Enabled ? 1 : 0));
}

// Starts up setup application, if capable.
// Returns TRUE if should continue running the game, otherwise FALSE.
bool engine_run_setup(const ConfigTree &cfg, int &app_res)
{
    app_res = EXIT_NORMAL;
#if AGS_PLATFORM_OS_WINDOWS
    {
            Debug::Printf(kDbgMsg_Info, "Running Setup");

            ConfigTree cfg_with_meta = cfg;
            fill_game_properties(cfg_with_meta["gameproperties"]);
            ConfigTree cfg_out;
            SetupReturnValue res = platform->RunSetup(cfg_with_meta, cfg_out);
            if (res != kSetup_Cancel)
            {
                String cfg_file = PreparePathForWriting(GetGameUserConfigDir(), DefaultConfigFileName);
                if (cfg_file.IsEmpty())
                {
                    platform->DisplayAlert("Unable to write into directory '%s'.\n%s",
                        GetGameUserConfigDir().FullDir.GetCStr(), platform->GetDiskWriteAccessTroubleshootingText());
                }
                else if (!IniUtil::Merge(cfg_file, cfg_out))
                {
                    platform->DisplayAlert("Unable to write to the configuration file (error code 0x%08X).\n%s",
                        platform->GetLastSystemError(), platform->GetDiskWriteAccessTroubleshootingText());
                }
            }
            if (res != kSetup_RunGame)
                return false;

            // TODO: investigate if the full program restart may (should) be avoided

            // Just re-reading the config file seems to cause a caching
            // problem on Win9x, so let's restart the process.
            sys_main_shutdown();
            allegro_exit();
            String args = String::FromFormat("\"%s\"", appPath.GetCStr());
            _spawnl(_P_OVERLAY, appPath.GetCStr(), args.GetCStr(), NULL);
    }
#else
    (void)cfg;
#endif
    return true;
}

// Scans given directory for the AGS game config. If such config exists
// and it contains data file name, then returns one.
// Otherwise returns empty string.
static String find_game_data_in_config(const String &path)
{
    // First look for config
    ConfigTree cfg;
    String def_cfg_file = Path::ConcatPaths(path, DefaultConfigFileName);
    if (IniUtil::Read(def_cfg_file, cfg))
    {
        String data_file = CfgReadString(cfg, "misc", "datafile");
        Debug::Printf("Found game config: %s", def_cfg_file.GetCStr());
        Debug::Printf(" Cfg: data file: %s", data_file.GetCStr());
        // Only accept if it's a relative path
        if (!data_file.IsEmpty() && Path::IsRelativePath(data_file))
            return Path::ConcatPaths(path, data_file);
    }
    return ""; // not found in config
}

// Scans for game data in several common locations.
// When it does so, it first looks for game config file, which contains
// explicit directions to game data in its settings.
// If such config is not found, it scans same location for *any* game data instead.
String search_for_game_data_file(String &was_searching_in)
{
    Debug::Printf("Looking for the game data.\n Cwd: %s\n Path arg: %s",
        Directory::GetCurrentDirectory().GetCStr(),
        cmdGameDataPath.GetCStr());
    // 1. From command line argument, which may be a directory or actual file
    if (!cmdGameDataPath.IsEmpty())
    {
        if (File::IsFile(cmdGameDataPath))
            return cmdGameDataPath; // this path is a file
        if (!File::IsDirectory(cmdGameDataPath))
            return ""; // path is neither file nor directory
        was_searching_in = cmdGameDataPath;
        Debug::Printf("Searching in (cmd arg): %s", was_searching_in.GetCStr());
        // first scan for config
        String data_path = find_game_data_in_config(cmdGameDataPath);
        if (!data_path.IsEmpty())
            return data_path;
        // if not found in config, lookup for data in same dir
        return FindGameData(cmdGameDataPath);
    }

    // 2. Look in other known locations
    // 2.1. Look for attachment in the running executable
    if (!appPath.IsEmpty() && Common::AssetManager::IsDataFile(appPath))
    {
        Debug::Printf("Found game data embedded in executable");
        was_searching_in = Path::GetDirectoryPath(appPath);
        return appPath;
    }

    // 2.2 Look in current working directory
    String cur_dir = Directory::GetCurrentDirectory();
    was_searching_in = cur_dir;
    Debug::Printf("Searching in (cwd): %s", was_searching_in.GetCStr());
    // first scan for config
    String data_path = find_game_data_in_config(cur_dir);
    if (!data_path.IsEmpty())
        return data_path;
    // if not found in config, lookup for data in same dir
    data_path = FindGameData(cur_dir);
    if (!data_path.IsEmpty())
        return data_path;

    // 2.3 Look in executable's directory (if it's different from current dir)
    if (Path::ComparePaths(appDirectory, cur_dir) == 0)
        return ""; // no luck
    was_searching_in = appDirectory;
    Debug::Printf("Searching in (exe dir): %s", was_searching_in.GetCStr());
    // first scan for config
    data_path = find_game_data_in_config(appDirectory);
    if (!data_path.IsEmpty())
        return data_path;
    // if not found in config, lookup for data in same dir
    return FindGameData(appDirectory);
}

void engine_init_fonts()
{
    Debug::Printf(kDbgMsg_Info, "Initializing TTF renderer");

    init_font_renderer();
}

void engine_init_mouse()
{
    int res = Mouse::GetButtonCount();
    if (res < 0)
        Debug::Printf(kDbgMsg_Info, "Initializing mouse: failed");
    else
        Debug::Printf(kDbgMsg_Info, "Initializing mouse: number of buttons reported is %d", res);
    Mouse::SetSpeed(usetup.mouse_speed);
}

void engine_locate_speech_pak()
{
    init_voicepak("");
}

void engine_locate_audio_pak()
{
    String music_file = game.GetAudioVOXName();
    String music_filepath = find_assetlib(music_file);
    if (!music_filepath.IsEmpty())
    {
        if (AssetMgr->AddLibrary(music_filepath) == kAssetNoError)
        {
            Debug::Printf(kDbgMsg_Info, "%s found and initialized.", music_file.GetCStr());
            ResPaths.AudioPak.Name = music_file;
            ResPaths.AudioPak.Path = music_filepath;
        }
        else
        {
            platform->DisplayAlert("Unable to initialize digital audio pack '%s', file could be corrupt or of unsupported format.",
                music_file.GetCStr());
        }
    }
    else if (!ResPaths.AudioDir2.IsEmpty() &&
        Path::ComparePaths(ResPaths.DataDir, ResPaths.AudioDir2) != 0)
    {
        Debug::Printf(kDbgMsg_Info, "Audio pack was not found, but explicit audio directory is defined.");
    }
}

// Assign asset locations to the AssetManager
void engine_assign_assetpaths()
{
    AssetMgr->AddLibrary(ResPaths.GamePak.Path, ",audio"); // main pack may have audio bundled too
    // The asset filters are currently a workaround for limiting search to certain locations;
    // this is both an optimization and to prevent unexpected behavior.
    // - empty filter is for regular files
    // audio - audio clips
    // voice - voice-over clips
    // NOTE: we add extra optional directories first because they should have higher priority
    // TODO: maybe change AssetManager library order to stack-like later (last added = top priority)?
    if (!ResPaths.DataDir2.IsEmpty() && Path::ComparePaths(ResPaths.DataDir2, ResPaths.DataDir) != 0)
        AssetMgr->AddLibrary(ResPaths.DataDir2, ",audio,voice"); // dir may have anything
    if (!ResPaths.AudioDir2.IsEmpty() && Path::ComparePaths(ResPaths.AudioDir2, ResPaths.DataDir) != 0)
        AssetMgr->AddLibrary(ResPaths.AudioDir2, "audio");
    if (!ResPaths.VoiceDir2.IsEmpty() && Path::ComparePaths(ResPaths.VoiceDir2, ResPaths.DataDir) != 0)
        AssetMgr->AddLibrary(ResPaths.VoiceDir2, "voice");

    AssetMgr->AddLibrary(ResPaths.DataDir, ",audio,voice"); // dir may have anything
    if (!ResPaths.AudioPak.Path.IsEmpty())
        AssetMgr->AddLibrary(ResPaths.AudioPak.Path, "audio");
    if (!ResPaths.SpeechPak.Path.IsEmpty())
        AssetMgr->AddLibrary(ResPaths.SpeechPak.Path, "voice");
}

void engine_init_keyboard()
{
    /* do nothing */
}

void engine_init_audio()
{
    if (usetup.audio_enabled)
    {
        Debug::Printf("Initializing audio");
        bool res = sys_audio_init(usetup.audio_driver);
        if (res)
        {
            try {
                audio_core_init(); // audio core system
            }
            catch (std::runtime_error ex) {
                Debug::Printf(kDbgMsg_Error, "Failed to initialize audio system: %s", ex.what());
                res = false;
            }
        }
        usetup.audio_enabled = res;
    }
    
    if (usetup.audio_enabled)
    {
        soundcache_set_rules(usetup.SoundLoadAtOnceSize * 1024, usetup.SoundCacheSize * 1024);
    }
    else
    {
        // all audio is disabled
        Debug::Printf(kDbgMsg_Info, "Audio is disabled");
    }
}

void engine_init_debug()
{
    if (usetup.show_fps)
        display_fps = kFPS_Forced;
    if ((debug_flags & (~DBG_DEBUGMODE)) >0) {
        platform->DisplayAlert("Engine debugging enabled.\n"
            "\nNOTE: You have selected to enable one or more engine debugging options.\n"
            "These options cause many parts of the game to behave abnormally, and you\n"
            "may not see the game as you are used to it. The point is to test whether\n"
            "the engine passes a point where it is crashing on you normally.\n"
            "[Debug flags enabled: 0x%02X]",debug_flags);
    }
}

void atexit_handler() {
    if (proper_exit==0) {
        platform->DisplayAlert("Error: the program has exited without requesting it.\n"
            "Program pointer: %+03d  (write this number down), engine version %s\n"
            "If you see a list of numbers above, please write them down and contact\n"
            "developers. Otherwise, note down any other information displayed.",
            our_eip, EngineVersion.LongString.GetCStr());
    }
}

void engine_init_exit_handler()
{
    Debug::Printf(kDbgMsg_Info, "Install exit handler");

    atexit(atexit_handler);
}

void engine_init_pathfinder()
{
    init_pathfinder(loaded_game_file_version);
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
    HError err = load_game_file();
    if (!err)
    {
        proper_exit=1;
        display_game_file_error(err);
        return EXIT_ERROR;
    }
    return 0;
}

// Replace special tokens inside a user path option
static void resolve_configured_path(String &option)
{
    option.Replace("$GAMENAME$", game.gamename);
}

// Setup paths and directories that may be affected by user configuration
void engine_init_user_directories()
{
    resolve_configured_path(usetup.user_data_dir);
    resolve_configured_path(usetup.shared_data_dir);
    if (!usetup.user_conf_dir.IsEmpty())
        Debug::Printf(kDbgMsg_Info, "User config directory: %s", usetup.user_conf_dir.GetCStr());
    if (!usetup.user_data_dir.IsEmpty())
        Debug::Printf(kDbgMsg_Info, "User data directory: %s", usetup.user_data_dir.GetCStr());
    if (!usetup.shared_data_dir.IsEmpty())
        Debug::Printf(kDbgMsg_Info, "Shared data directory: %s", usetup.shared_data_dir.GetCStr());

    // Initialize default save directory early, for we'll need it to set restart point
    SetDefaultSaveDirectory();
}

// TODO: remake/remove this nonsense
int check_write_access() {

  if (platform->GetDiskFreeSpaceMB() < 2)
    return 0;

  our_eip = -1895;

  // The Save Game Dir is the only place that we should write to
  String svg_dir = get_save_game_directory();
  String tempPath = String::FromFormat("%s""tmptest.tmp", svg_dir.GetCStr());
  Stream *temp_s = Common::File::CreateFile(tempPath);
  if (!temp_s)
    return 0;

  our_eip = -1896;

  temp_s->Write("just to test the drive free space", 30);
  delete temp_s;

  our_eip = -1897;

  if (!File::DeleteFile(tempPath))
    return 0;

  return 1;
}

int engine_check_disk_space()
{
    Debug::Printf(kDbgMsg_Info, "Checking for disk space");

    if (check_write_access()==0) {
        platform->DisplayAlert("Unable to write in the savegame directory.\n%s", platform->GetDiskWriteAccessTroubleshootingText());
        proper_exit = 1;
        return EXIT_ERROR; 
    }

    return 0;
}

int engine_check_font_was_loaded()
{
    if (!font_first_renderer_loaded())
    {
        platform->DisplayAlert("No game fonts found. At least one font is required to run the game.");
        proper_exit = 1;
        return EXIT_ERROR;
    }

    return 0;
}

// Do the preload graphic if available
void show_preload()
{
    RGB temppal[256];

    std::unique_ptr<Stream> stream (AssetMgr->OpenAsset("preload.pcx"));
    if(stream == nullptr) return;

    Bitmap *splashsc = BitmapHelper::LoadBitmap("pcx",stream.get(),temppal);
    if (splashsc != nullptr)
    {
        Debug::Printf("Displaying preload image");
        if (splashsc->GetColorDepth() == 8)
            set_palette_range(temppal, 0, 255, 0);
        if (gfxDriver->UsesMemoryBackBuffer())
            gfxDriver->GetMemoryBackBuffer()->Clear();

        const Rect &view = play.GetMainViewport();
        Bitmap *tsc = BitmapHelper::CreateBitmapCopy(splashsc, game.GetColorDepth());
        if (!gfxDriver->HasAcceleratedTransform() && view.GetSize() != tsc->GetSize())
        {
            Bitmap *stretched = new Bitmap(view.GetWidth(), view.GetHeight(), tsc->GetColorDepth());
            stretched->StretchBlt(tsc, RectWH(0, 0, view.GetWidth(), view.GetHeight()));
            delete tsc;
            tsc = stretched;
        }
        IDriverDependantBitmap *ddb = gfxDriver->CreateDDBFromBitmap(tsc, true /*opaque*/);
        ddb->SetStretch(view.GetWidth(), view.GetHeight());
        gfxDriver->ClearDrawLists();
        gfxDriver->BeginSpriteBatch(view);
        gfxDriver->DrawSprite(0, 0, ddb);
        gfxDriver->EndSpriteBatch();
        render_to_screen();
        gfxDriver->DestroyDDB(ddb);
        delete splashsc;
        delete tsc;
        platform->Delay(500);
    }
}

int engine_init_sprites()
{
    Debug::Printf(kDbgMsg_Info, "Initialize sprites");
    HError err = spriteset.InitFile(SpriteFile::DefaultSpriteFileName, SpriteFile::DefaultSpriteIndexName);
    if (!err) 
    {
        sys_main_shutdown();
        allegro_exit();
        proper_exit=1;
        platform->DisplayAlert("Could not load sprite set file %s\n%s",
            SpriteFile::DefaultSpriteFileName.GetCStr(),
            err->FullMessage().GetCStr());
        return EXIT_ERROR;
    }
    if (usetup.SpriteCacheSize > 0)
        spriteset.SetMaxCacheSize(usetup.SpriteCacheSize * 1024);
    Debug::Printf("Sprite cache set: %zu KB", spriteset.GetMaxCacheSize() / 1024);
    return 0;
}

// TODO: this should not be a part of "engine_" function group,
// move this elsewhere (InitGameState?).
void engine_init_game_settings()
{
    our_eip=-7;
    Debug::Printf("Initialize game settings");

    // Initialize randomizer
    play.randseed = time(nullptr);
    srand(play.randseed);

    if (usetup.audio_enabled)
    {
        play.separate_music_lib = !ResPaths.AudioPak.Name.IsEmpty();
        play.voice_avail = ResPaths.VoiceAvail;
    }
    else
    {
        play.voice_avail = false;
        play.separate_music_lib = false;
    }

    // Setup a text encoding mode depending on the game data hint
    if (game.options[OPT_GAMETEXTENCODING] == 65001) // utf-8 codepage number
        set_uformat(U_UTF8);
    else
        set_uformat(U_ASCII);

    int ee;

    for (ee=0;ee<256;ee++) {
        if (game.paluses[ee]!=PAL_BACKGROUND)
            palette[ee]=game.defpal[ee];
    }

    for (ee = 0; ee < game.numcursors; ee++) 
    {
        // The cursor graphics are assigned to mousecurs[] and so cannot
        // be removed from memory
        if (game.mcurs[ee].pic >= 0)
            spriteset.Precache(game.mcurs[ee].pic);

        // just in case they typed an invalid view number in the editor
        if (game.mcurs[ee].view >= game.numviews)
            game.mcurs[ee].view = -1;

        if (game.mcurs[ee].view >= 0)
            precache_view (game.mcurs[ee].view);
    }
    // may as well preload the character gfx
    if (playerchar->view >= 0)
        precache_view (playerchar->view, Character_GetDiagonalWalking(playerchar) ? 8 : 4);

    our_eip=-6;

    for (ee = 0; ee < MAX_ROOM_OBJECTS; ee++) {
        scrObj[ee].id = ee;
    }

    for (ee=0;ee<game.numcharacters;ee++) {
        memset(&game.chars[ee].inv[0],0,MAX_INV*sizeof(short));
        game.chars[ee].activeinv=-1;
        game.chars[ee].following=-1;
        game.chars[ee].followinfo=97 | (10 << 8);
        game.chars[ee].idleleft=game.chars[ee].idletime;
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

    our_eip=-5;
    for (ee=0;ee<game.numinvitems;ee++) {
        if (game.invinfo[ee].flags & IFLG_STARTWITH) playerchar->inv[ee]=1;
        else playerchar->inv[ee]=0;
    }

    //
    // TODO: following big initialization sequence could be in GameState ctor
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
    play.text_speed=15;
    play.text_min_display_time_ms = 1000;
    play.ignore_user_input_after_text_timeout_ms = 500;
    play.ClearIgnoreInput();
    play.lipsync_speed = 15;
    play.close_mouth_speech_time = 10;
    play.disable_antialiasing = 0;
    play.rtint_enabled = false;
    play.rtint_level = 0;
    play.rtint_light = 0;
    play.text_speed_modifier = 0;
    play.text_align = kHAlignLeft;
    // Make the default alignment to the right with right-to-left text
    if (game.options[OPT_RIGHTLEFTWRITE])
        play.text_align = kHAlignRight;

    play.speech_bubble_width = 100;
    play.bg_frame=0;
    play.bg_frame_locked=0;
    play.bg_anim_delay=0;
    play.anim_background_speed = 0;
    play.mouse_cursor_hidden = 0;
    play.skip_until_char_stops = -1;
    play.get_loc_name_last_time = -1;
    play.get_loc_name_save_cursor = -1;
    play.restore_cursor_mode_to = -1;
    play.restore_cursor_image_to = -1;
    play.ground_level_areas_disabled = 0;
    play.next_screen_transition = -1;
    play.temporarily_turned_off_character = -1;
    play.gamma_adjustment = 100;
    play.shakesc_length = 0;
    play.wait_counter=0;
    play.SetWaitSkipResult(SKIP_NONE);
    play.key_skip_wait = SKIP_NONE;
    play.audio_master_volume = 100;
    play.screen_flipped=0;
    play.speech_mode = kSpeech_VoiceText;
    play.cant_skip_speech = user_to_internal_skip_speech((SkipSpeechStyle)game.options[OPT_NOSKIPTEXT]);
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
    play.roomscript_finished = 0;
    play.no_textbg_when_voice = 0;
    play.max_dialogoption_width = 180;
    play.no_hicolor_fadein = 0;
    play.bgspeech_game_speed = 0;
    play.bgspeech_stay_on_display = 0;
    play.unfactor_speech_from_textlength = 0;
    play.speech_music_drop = 60;
    play.room_changes = 0;
    play.check_interaction_only = 0;
    play.replay_hotkey_unused = -1;  // StartRecording: not supported.
    play.dialog_options_x = 0;
    play.dialog_options_y = 0;
    play.min_dialogoption_width = 0;
    play.disable_dialog_parser = 0;
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
    play.speech_text_align = kHAlignCenter;
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
    play.stop_dialog_at_end = DIALOG_NONE;
    play.speech_portrait_placement = 0;
    play.speech_portrait_x = 0;
    play.speech_portrait_y = 0;
    play.speech_display_post_time_ms = 0;
    play.dialog_options_highlight_color = DIALOG_OPTIONS_HIGHLIGHT_COLOR_DEFAULT;
    play.speech_has_voice = false;
    play.speech_voice_blocking = false;
    play.speech_in_post_state = false;
    play.complete_overlay_on = 0;
    play.text_overlay_on = 0;
    play.narrator_speech = game.playercharacter;
    play.crossfading_out_channel = 0;
    play.speech_textwindow_gui = game.options[OPT_TWCUSTOM];
    if (play.speech_textwindow_gui == 0)
        play.speech_textwindow_gui = -1;
    snprintf(play.game_name, sizeof(play.game_name), "%s", game.gamename);
    play.lastParserEntry[0] = 0;
    play.follow_change_room_timer = 150;
    for (ee = 0; ee < MAX_ROOM_BGFRAMES; ee++) 
        play.raw_modified[ee] = 0;
    play.game_speed_modifier = 0;
    if (debug_flags & DBG_DEBUGMODE)
        play.debug_mode = 1;
    play.shake_screen_yoff = 0;

    GUI::Options.DisabledStyle = static_cast<GuiDisableStyle>(game.options[OPT_DISABLEOFF]);
    GUI::Options.ClipControls = game.options[OPT_CLIPGUICONTROLS] != 0;
    // Force GUI metrics recalculation, accomodating for loaded fonts
    GUI::MarkForFontUpdate(-1);

    memset(&play.walkable_areas_on[0],1,MAX_WALK_AREAS+1);
    memset(&play.script_timers[0],0,MAX_TIMERS * sizeof(int));
    memset(&play.default_audio_type_volumes[0], -1, MAX_AUDIO_TYPES * sizeof(int));

    if (!usetup.translation.IsEmpty())
        Game_ChangeTranslation(usetup.translation.GetCStr());

    update_invorder();
    displayed_room = -10;

    currentcursor=0;
    our_eip=-4;
    mousey=100;  // stop icon bar popping up

    // We use same variable to read config and be used at runtime for now,
    // so update it here with regards to game design option
    usetup.RenderAtScreenRes = 
        (game.options[OPT_RENDERATSCREENRES] == kRenderAtScreenRes_UserDefined && usetup.RenderAtScreenRes) ||
         game.options[OPT_RENDERATSCREENRES] == kRenderAtScreenRes_Enabled;
}

void engine_setup_scsystem_auxiliary()
{
    if (usetup.override_script_os > eOS_Unknown)
    {
        scsystem.os = usetup.override_script_os;
    }
    else
    {
        scsystem.os = platform->GetSystemOSID();
    }
}

void engine_prepare_to_start_game()
{
    Debug::Printf("Prepare to start game");

    engine_setup_scsystem_auxiliary();

    if (usetup.load_latest_save)
    {
        int slot = GetLastSaveSlot();
        if (slot >= 0)
            loadSaveGameOnStartup = get_save_game_path(slot);
    }
}

// Define location of the game data either using direct settings or searching
// for the available resource packs in common locations.
// Returns two paths:
// - startup_dir: this is where engine found game config and/or data;
// - data_path: full path of the main data pack;
// data_path's directory (may or not be eq to startup_dir) should be considered data directory,
// and this is where engine look for all game data.
HError define_gamedata_location_checkall(String &data_path, String &startup_dir)
{
    // First try if they provided a startup option
    if (!cmdGameDataPath.IsEmpty())
    {
        // If not a valid path - bail out
        if (!File::IsFileOrDir(cmdGameDataPath))
            return new Error(String::FromFormat("Provided game location is not a valid path.\n Cwd: %s\n Path: %s",
                Directory::GetCurrentDirectory().GetCStr(),
                cmdGameDataPath.GetCStr()));
        // If it's a file, then keep it and proceed
        if (File::IsFile(cmdGameDataPath))
        {
            Debug::Printf("Using provided game data path: %s", cmdGameDataPath.GetCStr());
            startup_dir = Path::GetDirectoryPath(cmdGameDataPath);
            data_path = cmdGameDataPath;
            return HError::None();
        }
    }

#if AGS_SEARCH_FOR_GAME_ON_LAUNCH
    // No direct filepath provided, search in common locations.
    data_path = search_for_game_data_file(startup_dir);
    if (data_path.IsEmpty())
    {
        return new Error("Engine was not able to find any compatible game data.",
            startup_dir.IsEmpty() ? String() : String::FromFormat("Searched in: %s", startup_dir.GetCStr()));
    }
    data_path = Path::MakeAbsolutePath(data_path);
    Debug::Printf(kDbgMsg_Info, "Located game data pak: %s", data_path.GetCStr());
    return HError::None();
#else
    data_path = platform->GetGameDataFile();
    if (!data_path.IsEmpty())
    {
        return HError::None();
    }
    return new Error("The game location was not defined by startup settings.");
#endif
}

// Define location of the game data
bool define_gamedata_location()
{
    String data_path, startup_dir;
    HError err = define_gamedata_location_checkall(data_path, startup_dir);
    if (!err)
    {
        platform->DisplayAlert("ERROR: Unable to determine game data.\n%s", err->FullMessage().GetCStr());
        main_print_help();
        return false;
    }

    // On success: set all the necessary path and filename settings
    usetup.startup_dir = startup_dir;
    usetup.main_data_file = data_path;
    usetup.main_data_dir = Path::GetDirectoryPath(data_path);
    return true;
}

// Find and preload main game data
bool engine_init_gamedata()
{
    Debug::Printf(kDbgMsg_Info, "Initializing game data");
    // First, find data location
    if (!define_gamedata_location())
        return false;

    // Try init game lib
    AssetError asset_err = AssetMgr->AddLibrary(usetup.main_data_file);
    if (asset_err != kAssetNoError)
    {
        platform->DisplayAlert("ERROR: The game data is missing, is of unsupported format or corrupt.\nFile: '%s'", usetup.main_data_file.GetCStr());
        return false;
    }

    // Pre-load game name and savegame folder names from data file
    // TODO: research if that is possible to avoid this step and just
    // read the full head game data at this point. This might require
    // further changes of the order of initialization.
    HError err = preload_game_data();
    if (!err)
    {
        display_game_file_error(err);
        return false;
    }

    // Setup ResPaths, so that we know out main locations further
    ResPaths.GamePak.Path = usetup.main_data_file;
    ResPaths.GamePak.Name = Path::GetFilename(usetup.main_data_file);
    ResPaths.DataDir = usetup.install_dir.IsEmpty() ? usetup.main_data_dir : Path::MakeAbsolutePath(usetup.install_dir);
    ResPaths.DataDir2 = Path::MakeAbsolutePath(usetup.opt_data_dir);
    ResPaths.AudioDir2 = Path::MakeAbsolutePath(usetup.opt_audio_dir);
    ResPaths.VoiceDir2 = Path::MakeAbsolutePath(usetup.opt_voice_dir);

    Debug::Printf(kDbgMsg_Info, "Startup directory: %s", usetup.startup_dir.GetCStr());
    Debug::Printf(kDbgMsg_Info, "Data directory: %s", ResPaths.DataDir.GetCStr());
    if (!ResPaths.DataDir2.IsEmpty())
        Debug::Printf(kDbgMsg_Info, "Opt data directory: %s", ResPaths.DataDir2.GetCStr());
    if (!ResPaths.AudioDir2.IsEmpty())
        Debug::Printf(kDbgMsg_Info, "Opt audio directory: %s", ResPaths.AudioDir2.GetCStr());
    if (!ResPaths.VoiceDir2.IsEmpty())
        Debug::Printf(kDbgMsg_Info, "Opt voice-over directory: %s", ResPaths.VoiceDir2.GetCStr());
    return true;
}

void engine_read_config(ConfigTree &cfg)
{
    if (!usetup.conf_path.IsEmpty())
    {
        IniUtil::Read(usetup.conf_path, cfg);
        return;
    }

    // Read default configuration file
    String def_cfg_file = find_default_cfg_file();
    IniUtil::Read(def_cfg_file, cfg);

    // Disabled on Windows because people were afraid that this config could be mistakenly
    // created by some installer and screw up their games. Until any kind of solution is found.
    String user_global_cfg_file;
    // Read user global configuration file
    user_global_cfg_file = find_user_global_cfg_file();
    if (Path::ComparePaths(user_global_cfg_file, def_cfg_file) != 0)
        IniUtil::Read(user_global_cfg_file, cfg);

    // Handle directive to search for the user config inside the custom directory;
    // this option may come either from command line or default/global config.
    if (usetup.user_conf_dir.IsEmpty())
        usetup.user_conf_dir = CfgReadString(cfg, "misc", "user_conf_dir");
    if (usetup.user_conf_dir.IsEmpty()) // also try deprecated option
        usetup.user_conf_dir = CfgReadBoolInt(cfg, "misc", "localuserconf") ? "." : "";
    // Test if the file is writeable, if it is then both engine and setup
    // applications may actually use it fully as a user config, otherwise
    // fallback to default behavior.
    if (!usetup.user_conf_dir.IsEmpty())
    {
        resolve_configured_path(usetup.user_conf_dir);
        if (Path::IsRelativePath(usetup.user_conf_dir))
            usetup.user_conf_dir = Path::ConcatPaths(usetup.startup_dir, usetup.user_conf_dir);
        if (!Directory::CreateDirectory(usetup.user_conf_dir) ||
            !File::TestWriteFile(Path::ConcatPaths(usetup.user_conf_dir, DefaultConfigFileName)))
        {
            Debug::Printf(kDbgMsg_Warn, "Write test failed at user config dir '%s', using default path.",
                usetup.user_conf_dir.GetCStr());
            usetup.user_conf_dir = "";
        }
    }

    // Read user configuration file
    String user_cfg_file = find_user_cfg_file();
    if (Path::ComparePaths(user_cfg_file, def_cfg_file) != 0 &&
        Path::ComparePaths(user_cfg_file, user_global_cfg_file) != 0)
        IniUtil::Read(user_cfg_file, cfg);

    // Apply overriding options from platform settings
    // TODO: normally, those should be instead stored in the same config file in a uniform way
    override_config_ext(cfg);
}

// Gathers settings from all available sources into single ConfigTree
void engine_prepare_config(ConfigTree &cfg, const ConfigTree &startup_opts)
{
    Debug::Printf(kDbgMsg_Info, "Setting up game configuration");
    // Read configuration files
    engine_read_config(cfg);
    // Merge startup options in
    for (const auto &sectn : startup_opts)
        for (const auto &opt : sectn.second)
            cfg[sectn.first][opt.first] = opt.second;
}

// Applies configuration to the running game
void engine_set_config(const ConfigTree cfg)
{
    config_defaults();
    apply_config(cfg);
    post_config();
}

//
// --tell command support: printing engine/game info by request
//
extern std::set<String> tellInfoKeys;
static bool print_info_needs_game(const std::set<String> &keys)
{
    return keys.count("all") > 0 || keys.count("config") > 0 || keys.count("configpath") > 0 ||
        keys.count("data") > 0 || keys.count("filepath") > 0 || keys.count("gameproperties") > 0;
}

static void engine_print_info(const std::set<String> &keys, ConfigTree *user_cfg)
{
    const bool all = keys.count("all") > 0;
    ConfigTree data;
    if (all || keys.count("engine") > 0)
    {
        data["engine"]["name"] = get_engine_name();
        data["engine"]["version"] = get_engine_version();
    }
    if (all || keys.count("graphicdriver") > 0)
    {
        StringV drv;
        AGS::Engine::GetGfxDriverFactoryNames(drv);
        for (size_t i = 0; i < drv.size(); ++i)
        {
            data["graphicdriver"][String::FromFormat("%zu", i)] = drv[i];
        }
    }
    if (all || keys.count("configpath") > 0)
    {
        String def_cfg_file = find_default_cfg_file();
        String gl_cfg_file = find_user_global_cfg_file();
        String user_cfg_file = find_user_cfg_file();
        data["configpath"]["default"] = def_cfg_file;
        data["configpath"]["global"] = gl_cfg_file;
        data["configpath"]["user"] = user_cfg_file;
    }
    if ((all || keys.count("config") > 0) && user_cfg)
    {
        for (const auto &sectn : *user_cfg)
        {
            String cfg_sectn = String::FromFormat("config@%s", sectn.first.GetCStr());
            for (const auto &opt : sectn.second)
                data[cfg_sectn][opt.first] = opt.second;
        }
    }
    if (all || keys.count("data") > 0)
    {
        data["data"]["gamename"] = game.gamename;
        data["data"]["version"] = StrUtil::IntToString(loaded_game_file_version);
        data["data"]["compiledwith"] = game.compiled_with;
        data["data"]["basepack"] = ResPaths.GamePak.Path;
    }
    if (all || keys.count("gameproperties") > 0)
    {
        fill_game_properties(data["gameproperties"]);
    }
    if (all || keys.count("filepath") > 0)
    {
        data["filepath"]["exe"] = appPath;
        data["filepath"]["cwd"] = Directory::GetCurrentDirectory();
        data["filepath"]["datadir"] = Path::MakePathNoSlash(ResPaths.DataDir);
        if (!ResPaths.DataDir2.IsEmpty())
        {
            data["filepath"]["datadir2"] = Path::MakePathNoSlash(ResPaths.DataDir2);
            data["filepath"]["audiodir2"] = Path::MakePathNoSlash(ResPaths.AudioDir2);
            data["filepath"]["voicedir2"] = Path::MakePathNoSlash(ResPaths.VoiceDir2);
        }
        data["filepath"]["savegamedir"] = Path::MakePathNoSlash(GetGameUserDataDir().FullDir);
        data["filepath"]["appdatadir"] = Path::MakePathNoSlash(GetGameAppDataDir().FullDir);
    }
    String full;
    IniUtil::WriteToString(full, data);
    platform->WriteStdOut("%s", full.GetCStr());
}

void engine_init_editor_debugging(const ConfigTree &cfg)
{
    Debug::Printf(kDbgMsg_Info, "Try connect to the external debugger");
    if (!init_editor_debugging(cfg))
        return;

    auto waitUntil = AGS_Clock::now() + std::chrono::milliseconds(500);
    while (waitUntil > AGS_Clock::now())
    {
        // pick up any breakpoints in game_start
        check_for_messages_from_debugger();
    }

    ccSetDebugHook(scriptDebugHook);
}

// TODO: this function is still a big mess, engine/system-related initialization
// is mixed with game-related data adjustments. Divide it in parts, move game
// data init into either InitGameState() or other game method as appropriate.
int initialize_engine(const ConfigTree &startup_opts)
{
    if (engine_pre_init_callback) {
        engine_pre_init_callback();
    }

    //-----------------------------------------------------
    // Install backend
    if (!engine_init_backend())
        return EXIT_ERROR;

    //-----------------------------------------------------
    // Connect to the external debugger, if required;
    // use only startup options here, the full config will be available
    // only after game files location is found
    if (editor_debugging_enabled &&
        !(justTellInfo || justRunSetup))
    {
        engine_init_editor_debugging(startup_opts);
    }

    //-----------------------------------------------------
    // Locate game data and assemble game config
    if (justTellInfo && !print_info_needs_game(tellInfoKeys))
    {
        engine_print_info(tellInfoKeys, nullptr);
        return EXIT_NORMAL;
    }

    if (!engine_init_gamedata())
        return EXIT_ERROR;
    ConfigTree cfg;
    engine_prepare_config(cfg, startup_opts);
    // Test if need to run built-in setup program (where available)
    if (!justTellInfo && justRunSetup)
    {
        int res;
        if (!engine_run_setup(cfg, res))
            return res;
    }
    // Set up game options from user config
    engine_set_config(cfg);
    if (justTellInfo)
    {
        engine_print_info(tellInfoKeys, &cfg);
        return EXIT_NORMAL;
    }

    our_eip = -190;

    //-----------------------------------------------------
    // Init auxiliary data files and other directories, initialize asset manager
    engine_init_user_directories();

    our_eip = -191;

    engine_locate_speech_pak();

    our_eip = -192;

    engine_locate_audio_pak();

    our_eip = -193;

    engine_assign_assetpaths();

    //-----------------------------------------------------
    // Begin setting up systems

    our_eip = -194;

    engine_init_fonts();

    our_eip = -195;

    init_joystick();

    engine_init_keyboard();

    our_eip = -196;

    engine_init_mouse();

    our_eip = -198;

    engine_init_audio();

    our_eip = -199;

    engine_init_debug();

    our_eip = -10;

    engine_init_exit_handler();

    engine_init_pathfinder();

    set_game_speed(40);

    our_eip=-20;
    our_eip=-19;

    int res = engine_load_game_data();
    if (res != 0)
        return res;

    our_eip = -189;

    res = engine_check_disk_space();
    if (res != 0)
        return res;

    // Make sure that at least one font was loaded in the process of loading
    // the game data.
    // TODO: Fold this check into engine_load_game_data()
    res = engine_check_font_was_loaded();
    if (res != 0)
        return res;

    our_eip = -179;

    engine_adjust_for_rotation_settings();

    // Attempt to initialize graphics mode
    if (!engine_try_set_gfxmode_any(usetup.Screen))
        return EXIT_ERROR;

    // Configure game window after renderer was initialized
    engine_setup_window();

    SetMultitasking(usetup.multitasking);

    sys_window_show_cursor(false); // hide the system cursor

    show_preload();
    res = engine_init_sprites();
    if (res != 0)
        return res;

    // TODO: move *init_game_settings to game init code unit
    engine_init_game_settings();
    engine_prepare_to_start_game();

    initialize_start_and_play_game(override_start_room, loadSaveGameOnStartup);

    return EXIT_NORMAL;
}

bool engine_try_set_gfxmode_any(const DisplayModeSetup &setup)
{
    engine_shutdown_gfxmode();

    sys_renderer_set_output(usetup.software_render_driver);

    const Size init_desktop = get_desktop_size();
    bool res = graphics_mode_init_any(GraphicResolution(game.GetGameRes(), game.color_depth * 8),
        setup, ColorDepthOption(game.GetColorDepth()));

    if (res)
        engine_post_gfxmode_setup(init_desktop);
    // Make sure that we don't receive window events queued during init
    sys_flush_events();
    return res;
}

bool engine_try_switch_windowed_gfxmode()
{
    if (!gfxDriver || !gfxDriver->IsModeSet() || !platform->FullscreenSupported())
        return false;

    // Keep previous mode in case we need to revert back
    DisplayMode old_dm = gfxDriver->GetDisplayMode();
    FrameScaleDef old_frame = graphics_mode_get_render_frame();

    // Release engine resources that depend on display mode
    engine_pre_gfxmode_release();

    Size init_desktop = get_desktop_size();
    bool windowed = !old_dm.IsWindowed();
    ActiveDisplaySetting setting = graphics_mode_get_last_setting(windowed);
    DisplayMode last_opposite_mode = setting.Dm;
    FrameScaleDef frame = setting.Frame;

    last_opposite_mode.Vsync = usetup.Screen.Params.VSync = old_dm.Vsync; // normalize vsync in case it has been toggled at runtime

    // Apply vsync in case it has been toggled at runtime
    last_opposite_mode.Vsync = usetup.Screen.Params.VSync;

    // If there are saved parameters for given mode (fullscreen/windowed)
    // then use them, if there are not, get default setup for the new mode.
    bool res;
    if (last_opposite_mode.IsValid())
    {
        res = graphics_mode_set_dm(last_opposite_mode);
    }
    else
    {
        WindowSetup ws = windowed ? usetup.Screen.WinSetup : usetup.Screen.FsSetup;
        frame = windowed ? usetup.Screen.WinGameFrame : usetup.Screen.FsGameFrame;
        res = graphics_mode_set_dm_any(game.GetGameRes(), ws, old_dm.ColorDepth,
            frame, usetup.Screen.Params);
    }

    // Apply corresponding frame render method
    if (res)
        res = graphics_mode_set_render_frame(frame);

    if (!res)
    {
        // If failed, try switching back to previous gfx mode
        res = graphics_mode_set_dm(old_dm) &&
              graphics_mode_set_render_frame(old_frame);
        if (!res)
            quitprintf("Failed to restore graphics mode.");
    }

    // If succeeded (with any case), update engine objects that rely on
    // active display mode.
    if (!gfxDriver->GetDisplayMode().IsRealFullscreen())
        init_desktop = get_desktop_size();
    engine_post_gfxmode_setup(init_desktop);
    // Make sure that we don't receive window events queued during init
    sys_flush_events();
    return res;
}

void engine_on_window_changed(const Size &sz)
{
    graphics_mode_on_window_changed(sz);
    on_coordinates_scaling_changed();
    invalidate_screen();
}

void engine_shutdown_gfxmode()
{
    if (!gfxDriver)
        return;

    engine_pre_gfxsystem_shutdown();
    graphics_mode_shutdown();
}

const char *get_engine_name()
{
    return "Adventure Game Studio run-time engine";
}

const char *get_engine_version()
{
    return EngineVersion.LongString.GetCStr();
}

String get_engine_version_and_build()
{
    const char *bit = (AGS_PLATFORM_64BIT) ? "64-bit" : "32-bit";
    const char *end = (AGS_PLATFORM_ENDIAN_LITTLE) ? "LE" : "BE";
#ifdef BUILD_STR
    return String::FromFormat("%s (Build: %s), %s %s",
        EngineVersion.LongString.GetCStr(), EngineVersion.BuildInfo.GetCStr(),
        bit, end);
#else
    return String::FromFormat("%s, %s %s",
        EngineVersion.LongString.GetCStr(),
        bit, end);
#endif
}

void engine_set_pre_init_callback(t_engine_pre_init_callback callback)
{
    engine_pre_init_callback = callback;
}
