//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "ac/game.h"
#include <stdio.h>
#include "ac/common.h"
#include "ac/view.h"
#include "ac/audiochannel.h"
#include "ac/button.h"
#include "ac/character.h"
#include "ac/dialog.h"
#include "ac/draw.h"
#include "ac/dynamicsprite.h"
#include "ac/event.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_audio.h"
#include "ac/global_display.h"
#include "ac/global_game.h"
#include "ac/global_gui.h"
#include "ac/global_object.h"
#include "ac/global_translation.h"
#include "ac/gui.h"
#include "ac/hotspot.h"
#include "ac/keycode.h"
#include "ac/lipsync.h"
#include "ac/mouse.h"
#include "ac/overlay.h"
#include "ac/path_helper.h"
#include "ac/sys_events.h"
#include "ac/room.h"
#include "ac/roomstatus.h"
#include "ac/sprite.h"
#include "ac/spritecache.h"
#include "ac/string.h"
#include "ac/translation.h"
#include "ac/dynobj/all_dynamicclasses.h"
#include "ac/dynobj/all_scriptclasses.h"
#include "ac/dynobj/cc_dynamicarray.h"
#include "ac/dynobj/scriptcamera.h"
#include "ac/dynobj/scriptgame.h"
#include "ac/dynobj/dynobj_manager.h"
#include "debug/debug_log.h"
#include "debug/out.h"
#include "device/mousew32.h"
#include "font/fonts.h"
#include "game/savegame.h"
#include "gfx/bitmap.h"
#include "gfx/graphicsdriver.h"
#include "gui/guibutton.h"
#include "gui/guiinv.h"
#include "gui/guilabel.h"
#include "gui/guilistbox.h"
#include "gui/guislider.h"
#include "gui/guitextbox.h"
#include "gui/guidialog.h"
#include "main/engine.h"
#include "main/game_run.h"
#include "media/audio/audio_system.h"
#include "media/video/video.h"
#include "platform/base/agsplatformdriver.h"
#include "platform/base/sys_main.h"
#include "plugin/plugin_engine.h"
#include "script/script.h"
#include "script/script_runtime.h"
#include "util/directory.h"
#include "util/file.h"
#include "util/path.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern ScriptSystem scsystem;
extern ScriptAudioChannel scrAudioChannel[MAX_GAME_CHANNELS];
extern std::vector<SpeechLipSyncLine> splipsync;
extern int numLipLines, curLipLine, curLipLinePhoneme;

extern int obj_lowest_yp, char_lowest_yp;

extern RGB palette[256];
extern IGraphicsDriver *gfxDriver;

//=============================================================================
std::unique_ptr<AssetManager> AssetMgr;
GamePlayState play;
GameSetup usetup;
GameSetupStruct game;
RoomStatus troom;    // used for non-saveable rooms, eg. intro
RoomObject*objs=nullptr;
RoomStatus*croom=nullptr;
RoomStruct thisroom;

volatile int switching_away_from_game = 0;
volatile bool switched_away = false;
volatile bool game_update_suspend = false;
volatile bool want_exit = false, abort_engine = false;
GameDataVersion loaded_game_file_version = kGameVersion_Undefined;
Version game_compiled_version;
int frames_per_second=40;
int displayed_room=-10;
EnterNewRoomState in_new_room = kEnterRoom_None, new_room_was = kEnterRoom_None;
int new_room_pos=0;
int new_room_x = SCR_NO_VALUE, new_room_y = SCR_NO_VALUE;
int new_room_loop = SCR_NO_VALUE;
bool new_room_placeonwalkable = false;
int proper_exit=0;

AGS::Common::SpriteCache::Callbacks spritecallbacks = {
    get_new_size_for_sprite,
    initialize_sprite,
    post_init_sprite,
    nullptr
};
SpriteCache spriteset(game.SpriteInfos, spritecallbacks);

std::vector<GUIMain> guis;
std::vector<GUIButton> guibuts;
std::vector<GUIInvWindow> guiinv;
std::vector<GUILabel> guilabels;
std::vector<GUIListBox> guilist;
std::vector<GUISlider> guislider;
std::vector<GUITextBox> guitext;

CCGUIObject ccDynamicGUIObject;
CCCharacter ccDynamicCharacter;
CCHotspot   ccDynamicHotspot;
CCRegion    ccDynamicRegion;
CCInventory ccDynamicInv;
CCGUI       ccDynamicGUI;
CCObject    ccDynamicObject;
CCDialog    ccDynamicDialog;
CCAudioClip ccDynamicAudioClip;
CCAudioChannel ccDynamicAudio;
ScriptObject scrObj[MAX_ROOM_OBJECTS];
std::vector<ScriptGUI> scrGui;
ScriptHotspot scrHotspot[MAX_ROOM_HOTSPOTS];
ScriptRegion scrRegion[MAX_ROOM_REGIONS];
ScriptInvItem scrInv[MAX_INV];
std::vector<ScriptDialog> scrDialog;

std::vector<ViewStruct> views;
std::vector<CharacterExtras> charextra;
// MoveLists for characters and room objects; NOTE: 1-based array!
// object sprites begin with index 1, characters are after MAX_ROOM_OBJECTS + 1
std::vector<MoveList> mls;

//=============================================================================

String saveGameDirectory = "./";

String saveGameSuffix;

int game_paused=0;

unsigned int load_new_game = 0;
int load_new_game_restore = -1;
SaveCmpSelection load_new_game_restore_cmp = kSaveCmp_All;

// TODO: refactor these global vars into function arguments
int getloctype_index = 0, getloctype_throughgui = 0;

//=============================================================================
// Audio
//=============================================================================

void Game_StopAudio(int audioType)
{
    if (((audioType < 0) || ((size_t)audioType >= game.audioClipTypes.size())) && (audioType != SCR_NO_VALUE))
        quitprintf("!Game.StopAudio: invalid audio type %d", audioType);
    for (int aa = 0; aa < game.numGameChannels; aa++)
    {
        if (audioType == SCR_NO_VALUE)
        {
            stop_or_fade_out_channel(aa);
        }
        else
        {
            ScriptAudioClip *clip = AudioChannel_GetPlayingClip(&scrAudioChannel[aa]);
            if ((clip != nullptr) && (clip->type == audioType))
                stop_or_fade_out_channel(aa);
        }
    }

    remove_clips_of_type_from_queue(audioType);
}

int Game_IsAudioPlaying(int audioType)
{
    if (((audioType < 0) || ((size_t)audioType >= game.audioClipTypes.size())) && (audioType != SCR_NO_VALUE))
        quitprintf("!Game.IsAudioPlaying: invalid audio type %d", audioType);

    if (play.fast_forward)
        return 0;

    for (int aa = 0; aa < game.numGameChannels; aa++)
    {
        ScriptAudioClip *clip = AudioChannel_GetPlayingClip(&scrAudioChannel[aa]);
        if (clip != nullptr)
        {
            if ((clip->type == audioType) || (audioType == SCR_NO_VALUE))
            {
                return 1;
            }
        }
    }
    return 0;
}

void Game_SetAudioTypeSpeechVolumeDrop(int audioType, int volumeDrop)
{
    if ((audioType < 0) || ((size_t)audioType >= game.audioClipTypes.size()))
        quitprintf("!Game.SetAudioTypeVolume: invalid audio type: %d", audioType);

    Debug::Printf("Game.SetAudioTypeSpeechVolumeDrop: type: %d, drop: %d", audioType, volumeDrop);
    game.audioClipTypes[audioType].volume_reduction_while_speech_playing = volumeDrop;
    update_volume_drop_if_voiceover();
}

void Game_SetAudioTypeVolume(int audioType, int volume, int changeType)
{
    if ((volume < 0) || (volume > 100))
        quitprintf("!Game.SetAudioTypeVolume: volume %d is not between 0..100", volume);
    if ((audioType < 0) || ((size_t)audioType >= game.audioClipTypes.size()))
        quitprintf("!Game.SetAudioTypeVolume: invalid audio type: %d", audioType);

    const char *change_str[3]{"existing", "future", "all"};
    Debug::Printf("Game.SetAudioTypeVolume: type: %d, volume: %d, change: %s", audioType, volume,
        change_str[changeType - VOL_CHANGEEXISTING]);
    if ((changeType == VOL_CHANGEEXISTING) ||
        (changeType == VOL_BOTH))
    {
        for (int aa = 0; aa < game.numGameChannels; aa++)
        {
            ScriptAudioClip *clip = AudioChannel_GetPlayingClip(&scrAudioChannel[aa]);
            if ((clip != nullptr) && (clip->type == audioType))
            {
                auto* ch = AudioChans::GetChannel(aa);
                if (ch)
                    ch->set_volume100(volume);
            }
        }
    }

    if ((changeType == VOL_SETFUTUREDEFAULT) ||
        (changeType == VOL_BOTH))
    {
        play.default_audio_type_volumes[audioType] = volume;

        // update queued clip volumes
        update_queued_clips_volume(audioType, volume);
    }

}

int Game_GetMODPattern() {
    if (current_music_type != MUS_MOD)
        return -1;
    auto* music_ch = AudioChans::GetChannelIfPlaying(SCHAN_MUSIC);
    return music_ch ? music_ch->get_pos() : -1;
}

//=============================================================================
// ---
//=============================================================================

int Game_GetDialogCount()
{
  return game.numdialog;
}

void set_debug_mode(bool on)
{
    play.debug_mode = on ? 1 : 0;
}

void set_game_speed(int new_fps) {
    frames_per_second = new_fps;
    if (!isTimerFpsMaxed()) // if in maxed mode, don't update timer for now
        setTimerFps(new_fps);
}

float get_game_speed() {
    return frames_per_second;
}

extern int cbuttfont;
extern int acdialog_font;

int oldmouse;
void setup_for_dialog(int use_font) {
    cbuttfont = use_font;
    acdialog_font = use_font;
    oldmouse=cur_cursor;
    set_mouse_cursor(CURS_ARROW);
}
void restore_after_dialog() {
    set_mouse_cursor(oldmouse);
    invalidate_screen();
}



String get_save_game_directory()
{
    return saveGameDirectory;
}

String get_save_game_suffix()
{
    return saveGameSuffix;
}

void set_save_game_suffix(const String &suffix)
{
    saveGameSuffix = suffix;
}

String get_save_game_filename(int slotNum)
{
    return String::FromFormat("agssave.%03d%s", slotNum, saveGameSuffix.GetCStr());
}

String get_save_game_path(int slotNum)
{
    return Path::ConcatPaths(saveGameDirectory, get_save_game_filename(slotNum));
}

bool get_save_slotnum(const String &filename, int &slot)
{
    if (filename.CompareLeftNoCase("agssave.") == 0)
        return sscanf(filename.GetCStr(), "agssave.%03d", &slot) == 1;
    return false;
}

// Convert a path possibly containing path tags into acceptable save path
// NOTE that the game script may issue an order to change the save directory to
// a dir of a new name. While we let this work, we also try to keep these
// inside same parent location, would that be a common system directory,
// or a custom one set by a player in config.
static bool MakeSaveGameDir(const String &new_dir, FSLocation &fsdir)
{
    fsdir = FSLocation();
    // don't allow absolute paths
    if (!Path::IsRelativePath(new_dir))
        return false;

    String fixed_newdir = new_dir;
    if (new_dir.CompareLeft(UserSavedgamesRootToken, UserSavedgamesRootToken.GetLength()) == 0)
    {
        fixed_newdir.ClipLeft(UserSavedgamesRootToken.GetLength());
    }
    else if (game.options[OPT_SAFEFILEPATHS] > 0)
    { // For games made in the safe-path-aware versions of AGS, report a warning
        debug_script_warn("Attempt to explicitly set savegame location relative to the game installation directory ('%s') denied;\nPath will be remapped to the user documents directory: '%s'",
            new_dir.GetCStr(), fsdir.FullDir.GetCStr());
    }

    // Resolve the new dir relative to the user data parent dir
    fsdir = GetGameUserDataDir().Concat(fixed_newdir);
    return true;
}

static bool SetSaveGameDirectory(const FSLocation &fsdir)
{
    if (!Directory::CreateAllDirectories(fsdir.BaseDir, fsdir.SubDir))
    {
        debug_script_warn("SetSaveGameDirectory: failed to create all subdirectories: %s", fsdir.FullDir.GetCStr());
        return false;
    }

    String newSaveGameDir = fsdir.FullDir;
    if (!File::TestCreateFile(Path::ConcatPaths(newSaveGameDir, "agstmp.tmp")))
        return false;

    // copy the Restart Game file, if applicable
    String old_restart_path = Path::ConcatPaths(saveGameDirectory, get_save_game_filename(RESTART_POINT_SAVE_GAME_NUMBER));
    if (File::IsFile(old_restart_path))
    {
        String new_restart_path = Path::ConcatPaths(newSaveGameDir, get_save_game_filename(RESTART_POINT_SAVE_GAME_NUMBER));
        File::CopyFile(old_restart_path, new_restart_path, true);
    }
    saveGameDirectory = newSaveGameDir;
    return true;
}

void SetDefaultSaveDirectory()
{
    // Request a default save location, and assign it as a save dir
    FSLocation fsdir = GetGameUserDataDir();
    SetSaveGameDirectory(fsdir);
}

int Game_SetSaveGameDirectory(const char *newFolder)
{
    // First resolve the script path (it may contain tokens)
    FSLocation fsdir;
    if (!MakeSaveGameDir(newFolder, fsdir))
        return 0;
    // If resolved successfully, try to assign the new dir
    return SetSaveGameDirectory(fsdir) ? 1 : 0;
}

const char* Game_GetSaveSlotDescription(int slnum) {
    String description;
    if (read_savedgame_description(get_save_game_path(slnum), description))
    {
        return CreateNewScriptString(description);
    }
    return nullptr;
}

ScriptDateTime* Game_GetSaveSlotTime(int slnum)
{
    time_t ft = File::GetFileTime(get_save_game_path(slnum));
    ScriptDateTime *sdt = ScriptDateTime::FromStdTime(ft);
    ccRegisterManagedObject(sdt, sdt);
    return sdt;
}

void restore_game_dialog()
{
    restore_game_dialog2(1, LEGACY_TOP_BUILTINDIALOGSAVESLOT);
}

void restore_game_dialog2(int min_slot, int max_slot)
{
    // Optionally override the max slot
    max_slot = usetup.Override.MaxSaveSlot > 0 ? usetup.Override.MaxSaveSlot : max_slot;

    can_run_delayed_command();
    if (thisroom.Options.SaveLoadDisabled == 1) {
        DisplayMessage(983);
        return;
    }
    if (inside_script) {
        get_executingscript()->QueueAction(PostScriptAction(ePSARestoreGameDialog, (min_slot & 0xFFFF) | (max_slot & 0xFFFF) << 16, play.normal_font, "RestoreGameDialog"));
        return;
    }
    do_restore_game_dialog(min_slot, max_slot, play.normal_font);
}

bool do_restore_game_dialog(int min_slot, int max_slot, int use_font)
{
    setup_for_dialog(use_font);
    int toload = loadgamedialog(min_slot, max_slot);
    restore_after_dialog();
    if (toload >= 0)
        try_restore_save(toload);
    return toload >= 0;
}

void save_game_dialog()
{
    save_game_dialog2(1, LEGACY_TOP_BUILTINDIALOGSAVESLOT);
}

void save_game_dialog2(int min_slot, int max_slot)
{
    // Optionally override the max slot
    max_slot = usetup.Override.MaxSaveSlot > 0 ? usetup.Override.MaxSaveSlot : max_slot;

    can_run_delayed_command();
    if (thisroom.Options.SaveLoadDisabled == 1) {
        DisplayMessage(983);
        return;
    }
    if (inside_script) {
        get_executingscript()->QueueAction(PostScriptAction(ePSASaveGameDialog, (min_slot & 0xFFFF) | (max_slot & 0xFFFF) << 16, play.normal_font, "SaveGameDialog"));
        return;
    }
    do_save_game_dialog(min_slot, max_slot, play.normal_font);
}

bool do_save_game_dialog(int min_slot, int max_slot, int use_font)
{
    setup_for_dialog(use_font);
    int tosave = savegamedialog(min_slot, max_slot);
    restore_after_dialog();
    if (tosave >= 0)
        save_game(tosave, get_gui_dialog_buffer());
    return tosave >= 0;
}

void free_do_once_tokens()
{
    play.do_once_tokens.clear();
}

void shutdown_game_state()
{
    // Try every possible game state that is represented by a global object
    shutdown_dialog_state();
    ShutGameWaitState();
}

// Free all the memory associated with the game
void unload_game()
{
    dispose_game_drawdata();
    // NOTE: fonts should be freed prior to stopping plugins,
    // as plugins may provide font renderer interface.
    free_all_fonts();
    close_translation();

    // NOTE: script objects must be freed prior to stopping plugins,
    // in case there are managed objects provided by plugins.
    ccRemoveAllSymbols();
    ccUnregisterAllObjects();
    pl_stop_plugins();

    FreeAllScriptInstances();
    FreeGlobalScripts();

    charextra.clear();
    mls.clear();
    views.clear();
    splipsync.clear();
    numLipLines = 0;
    curLipLine = -1;

    dialog.clear();
    scrDialog.clear();

    guis.clear();
    scrGui.clear();

    get_overlays().clear();

    resetRoomStatuses();

    dispose_room_pathfinder();

    // Free game state and game struct
    play = GamePlayState();
    game = GameSetupStruct();

    // Reset all resource caches
    // IMPORTANT: this is hard reset, including locked items
    spriteset.Reset();
    soundcache_clear();
}






const char* Game_GetGlobalStrings(int index) {
    if ((index < 0) || (index >= MAXGLOBALSTRINGS))
        quit("!Game.GlobalStrings: invalid index");

    return CreateNewScriptString(play.globalstrings[index]);
}


// ** GetGameParameter replacement functions

int Game_GetInventoryItemCount() {
    // because of the dummy item 0, this is always one higher than it should be
    return game.numinvitems - 1;
}

int Game_GetFontCount() {
    return game.numfonts;
}

int Game_GetMouseCursorCount() {
    return game.numcursors;
}

int Game_GetCharacterCount() {
    return game.numcharacters;
}

int Game_GetGUICount() {
    return game.numgui;
}

int Game_GetViewCount() {
    return game.numviews;
}

int Game_GetUseNativeCoordinates()
{
    return game.IsDataInNativeCoordinates() ? 1 : 0;
}

int Game_GetSpriteWidth(int spriteNum) {
    if (spriteNum < 0)
        return 0;

    if (!spriteset.DoesSpriteExist(spriteNum))
        return 0;

    return game_to_data_coord(game.SpriteInfos[spriteNum].Width);
}

int Game_GetSpriteHeight(int spriteNum) {
    if (spriteNum < 0)
        return 0;

    if (!spriteset.DoesSpriteExist(spriteNum))
        return 0;

    return game_to_data_coord(game.SpriteInfos[spriteNum].Height);
}

ScriptFileSortStyle ValidateFileSort(const char *apiname, int file_sort)
{
    if (file_sort < kScFileSort_None || file_sort > kScFileSort_Time)
    {
        debug_script_warn("%s: invalid file sort style (%d)", apiname, file_sort);
        return kScFileSort_None;
    }
    return static_cast<ScriptFileSortStyle>(file_sort);
}

ScriptSaveGameSortStyle ValidateSaveGameSort(const char *apiname, int save_sort)
{
    if (save_sort < kScSaveGameSort_None || save_sort > kScSaveGameSort_Description)
    {
        debug_script_warn("%s: invalid save game sort style (%d)", apiname, save_sort);
        return kScSaveGameSort_None;
    }
    return static_cast<ScriptSaveGameSortStyle>(save_sort);
}

ScriptSortDirection ValidateSortDirection(const char *apiname, int sort_dir)
{
    if (sort_dir < kScSortNone || sort_dir > kScSortDescending)
    {
        debug_script_warn("%s: invalid sorting direction (%d)", apiname, sort_dir);
        return kScSortNone;
    }
    return static_cast<ScriptSortDirection>(sort_dir);
}

bool ValidateSaveSlotRange(const char *api_name, int &min_slot, int &max_slot)
{
    int do_max_slot = std::min(max_slot, TOP_SAVESLOT);
    int do_min_slot = std::min(do_max_slot, std::max(0, min_slot));
    if (do_max_slot - do_min_slot + 1 <= 0)
    {
        debug_script_warn("%s: empty or invalid slots range requested (requested: %d..%d, valid range %d..%d)",
            api_name, min_slot, max_slot, 0, TOP_SAVESLOT);
        return false;
    }
    min_slot = do_min_slot;
    max_slot = do_max_slot;
    return true;
}

void AssertView(const char *apiname, int view)
{
    // NOTE: we assume (here and below) that the view is already in an internal 0-based range.
    // but when printing an error we will use (view + 1) for compliance with the script API.
    if ((view < 0) || (view >= game.numviews))
        quitprintf("!%s: invalid view %d (range is 1..%d)", apiname, view + 1, game.numviews);
}

void AssertViewHasLoops(const char *apiname, int view)
{
    AssertView(apiname, view);
    if (views[view].numLoops == 0)
        quitprintf("!%s: view %d does not have any loops.", apiname, view + 1);
}

void AssertLoop(const char *apiname, int view, int loop)
{
    AssertViewHasLoops(apiname, view);
    if ((loop < 0) || (loop >= views[view].numLoops))
        quitprintf("!%s: invalid loop number %d for view %d (range is 0..%d).",
            apiname, loop, view + 1, views[view].numLoops - 1);
}

void AssertFrame(const char *apiname, int view, int loop, int frame)
{
    AssertLoop(apiname, view, loop);
    if (views[view].loops[loop].numFrames == 0)
        quitprintf("!%s: view %d loop %d does not have any frames", apiname, view + 1, loop);
    if ((frame < 0) || (frame >= views[view].loops[loop].numFrames))
        quitprintf("!%s: invalid frame number %d for view %d loop %d (range is 0..%d)",
            apiname, frame, view + 1, loop, views[view].loops[loop].numFrames - 1);
}

int Game_GetLoopCountForView(int view) {
    view--; // convert to 0-based
    AssertView("Game.GetLoopCountForView", view);
    return views[view].numLoops;
}

int Game_GetRunNextSettingForLoop(int view, int loop) {
    view--; // convert to 0-based
    AssertLoop("Game.GetRunNextSettingForLoop", view, loop);
    return (views[view].loops[loop].RunNextLoop()) ? 1 : 0;
}

int Game_GetFrameCountForLoop(int view, int loop) {
    view--; // convert to 0-based
    AssertLoop("Game.GetFrameCountForLoop", view, loop);
    return views[view].loops[loop].numFrames;
}

ScriptViewFrame* Game_GetViewFrame(int view, int loop, int frame) {
    view--; // convert to 0-based
    AssertFrame("Game.GetViewFrame", view, loop, frame);
    ScriptViewFrame *sdt = new ScriptViewFrame(view, loop, frame);
    ccRegisterManagedObject(sdt, sdt);
    return sdt;
}

int Game_DoOnceOnly(const char *token)
{
    if (play.do_once_tokens.count(String::Wrapper(token)) > 0)
        return 0;
    play.do_once_tokens.insert(token);
    return 1;
}

void Game_ResetDoOnceOnly()
{
    free_do_once_tokens();
}

int Game_GetTextReadingSpeed()
{
    return play.text_speed;
}

void Game_SetTextReadingSpeed(int newTextSpeed)
{
    if (newTextSpeed < 1)
        quitprintf("!Game.TextReadingSpeed: %d is an invalid speed", newTextSpeed);

    if (usetup.Access.TextReadSpeed <= 0)
        play.text_speed = newTextSpeed;
}

int Game_GetMinimumTextDisplayTimeMs()
{
    return play.text_min_display_time_ms;
}

void Game_SetMinimumTextDisplayTimeMs(int newTextMinTime)
{
    if (usetup.Access.TextReadSpeed <= 0)
        play.text_min_display_time_ms = newTextMinTime;
}

int Game_GetIgnoreUserInputAfterTextTimeoutMs()
{
    return play.ignore_user_input_after_text_timeout_ms;
}

void Game_SetIgnoreUserInputAfterTextTimeoutMs(int newValueMs)
{
    play.ignore_user_input_after_text_timeout_ms = newValueMs;
}

const char *Game_GetFileName() {
    return CreateNewScriptString(ResPaths.GamePak.Name);
}

const char *Game_GetName() {
    return CreateNewScriptString(play.game_name);
}

void Game_SetName(const char *newName) {
    play.game_name = newName;
    sys_window_set_title(play.game_name.GetCStr());
    GUIE::MarkSpecialLabelsForUpdate(kLabelMacro_Gamename);
}

int Game_GetSkippingCutscene()
{
    if (play.fast_forward)
    {
        return 1;
    }
    return 0;
}

int Game_GetInSkippableCutscene()
{
    if (play.in_cutscene)
    {
        return 1;
    }
    return 0;
}

int Game_GetColorFromRGB(int red, int grn, int blu) {
    if ((red < 0) || (red > 255) || (grn < 0) || (grn > 255) ||
        (blu < 0) || (blu > 255))
        quit("!GetColorFromRGB: colour values must be 0-255");

    if (game.color_depth == 1)
    {
        return makecol8(red, grn, blu);
    }

    int agscolor = ((blu >> 3) & 0x1f);
    agscolor += ((grn >> 2) & 0x3f) << 5;
    agscolor += ((red >> 3) & 0x1f) << 11;
    return agscolor;
}

const char* Game_InputBox(const char *msg) {
    char buffer[STD_BUFFER_SIZE];
    ShowInputBoxImpl(msg, buffer, STD_BUFFER_SIZE);
    return CreateNewScriptString(buffer);
}

const char* Game_GetLocationName(int x, int y) {
    return CreateNewScriptString(GetLocationName(x, y));
}

const char* Game_GetGlobalMessages(int index) {
    if ((index < 500) || (index >= MAXGLOBALMES + 500)) {
        return nullptr;
    }
    char buffer[STD_BUFFER_SIZE];
    replace_tokens(get_translation(get_global_message(index)), buffer, STD_BUFFER_SIZE);
    return CreateNewScriptString(buffer);
}

int Game_GetSpeechFont() {
    return play.speech_font;
}
int Game_GetNormalFont() {
    return play.normal_font;
}

const char* Game_GetTranslationFilename() {
    char buffer[STD_BUFFER_SIZE];
    GetTranslationName(buffer); // fills up to MAX_MAXSTRLEN
    return CreateNewScriptString(buffer);
}

int Game_ChangeTranslation(const char *newFilename)
{
    if ((newFilename == nullptr) || (newFilename[0] == 0))
    { // switch back to default translation
        close_translation();
        usetup.Translation = "";
        GUIE::MarkForTranslationUpdate();
        return 1;
    }

    String oldTransFileName = get_translation_name();
    if (!init_translation(newFilename, oldTransFileName))
        return 0; // failed, kept previous translation

    usetup.Translation = newFilename;
    GUIE::MarkForTranslationUpdate();
    return 1;
}

const char* Game_GetSpeechVoxFilename()
{
    return CreateNewScriptString(get_voicepak_name());
}

bool Game_ChangeSpeechVox(const char *newFilename)
{
    play.voice_avail = init_voicepak(newFilename);
    if (!play.voice_avail)
    {
        // if failed (and was not default)- fallback to default
        if (strlen(newFilename) > 0)
            play.voice_avail = init_voicepak();
        return false;
    }
    return true;
}

int Game_GetAudioClipCount()
{
    return game.audioClips.size();
}

ScriptAudioClip *Game_GetAudioClip(int index)
{
    if (index < 0 || (size_t)index >= game.audioClips.size())
        return nullptr;
    return &game.audioClips[index];
}

ScriptCamera* Game_GetCamera()
{
    return play.GetScriptCamera(0);
}

int Game_GetCameraCount()
{
    return play.GetRoomCameraCount();
}

ScriptCamera* Game_GetAnyCamera(int index)
{
    return play.GetScriptCamera(index);
}

void Game_SimulateKeyPress(int key)
{
    ags_simulate_keypress(static_cast<eAGSKeyCode>(key), (game.options[OPT_KEYHANDLEAPI] == 0));
}

int Game_BlockingWaitSkipped()
{
    return play.GetWaitSkipResult();
}

int Game_InBlockingWait()
{
    return IsInWaitMode();
}

void Game_PrecacheSprite(int sprnum)
{
    const auto tp_start = FastClock::now();
    spriteset.PrecacheSprite(sprnum);
    const auto tp_filedone = FastClock::now();
    texturecache_precache(sprnum);
    const auto tp_texturedone = FastClock::now();

    const auto dur1 = ToMilliseconds(tp_filedone - tp_start);
    const auto dur2 = ToMilliseconds(tp_texturedone - tp_filedone);
    const auto dur_t = ToMilliseconds(tp_texturedone - tp_start);
    Debug::Printf("Precache sprite %d; file->mem = %lld ms, bm->tx = %lld ms, total = %lld ms", sprnum, dur1, dur2, dur_t);
}

void Game_PrecacheView(int view, int first_loop, int last_loop)
{
    precache_view(view - 1 /* to 0-based view index */, first_loop, last_loop, true);
}

void *Game_GetSaveSlots(int min_slot, int max_slot, int save_sort, int sort_dir)
{
    if (!ValidateSaveSlotRange("Game.GetSaveSlots", min_slot, max_slot))
        return CCDynamicArray::Create(0, sizeof(int32_t), false).Obj;
    save_sort = ValidateSaveGameSort("Game.GetSaveSlots", save_sort);
    sort_dir = ValidateSortDirection("Game.GetSaveSlots", sort_dir);

    std::vector<SaveListItem> saves;
    FillSaveList(saves, min_slot, max_slot, false /* no desc */, (ScriptSaveGameSortStyle)save_sort, (ScriptSortDirection)sort_dir);

    DynObjectRef arr = CCDynamicArray::Create(saves.size(), sizeof(int32_t), false);
    int32_t *arr_ptr = static_cast<int32_t*>(arr.Obj);
    for (const auto &save : saves)
        *(arr_ptr++) = save.Slot;
    return arr.Obj;
}

extern void prescan_saves(int *dest_arr, size_t dest_count, int min_slot, int max_slot, int file_sort, int sort_dir);
extern ExecutingScript *curscript;

void Game_ScanSaveSlots(void *dest_arr, int min_slot, int max_slot, int save_sort, int sort_dir, int user_param)
{
    const auto &hdr = CCDynamicArray::GetHeader(dest_arr);
    if (hdr.GetElemCount() == 0u)
    {
        debug_script_warn("Game.ScanSaveSlots: empty array provided, skip execution");
        return;
    }

    if (!ValidateSaveSlotRange("Game.ScanSaveSlots", min_slot, max_slot))
        return;
    save_sort = ValidateSaveGameSort("Game.ScanSaveSlots", save_sort);
    sort_dir = ValidateSortDirection("Game.ScanSaveSlots", sort_dir);

    can_run_delayed_command();
    if (inside_script)
    {
        int handle = ccGetObjectHandleFromAddress(dest_arr);
        ccAddObjectReference(handle); // add internal handle to prevent disposal
        curscript->QueueAction(PostScriptAction(ePSAScanSaves, handle, min_slot, max_slot, save_sort, sort_dir, user_param, "ScanSaveSlots"));
        return;
    }

    prescan_saves(static_cast<int*>(dest_arr), hdr.GetElemCount(), min_slot, max_slot, save_sort, sort_dir);
}

//=============================================================================

std::unique_ptr<Common::Bitmap> create_game_screenshot(int width, int height, int layers)
{
    // NOTE: be aware that by the historical logic AGS makes a screenshot
    // of a "main viewport", that may be smaller in legacy "letterbox" mode.
    const Rect &viewport = play.GetMainViewport();
    if (width <= 0)
        width = viewport.GetWidth();
    else
        width = data_to_game_coord(width);

    if (height <= 0)
        height = viewport.GetHeight();
    else
        height = data_to_game_coord(height);

    // NOTE: if there will be a difference between script constants and internal
    // constants of Render Layers, or any necessity to adjust these, then convert flags here.
    std::unique_ptr<Bitmap> shot;
    if (layers != 0)
        shot.reset(CopyScreenIntoBitmap(width, height, &viewport, false, ~layers));
    else
        shot.reset(new Bitmap(width, height));
    return shot;
}

static std::unique_ptr<Common::Bitmap> create_savegame_screenshot()
{
    return create_game_screenshot(play.screenshot_width, play.screenshot_height, game.options[OPT_SAVESCREENSHOTLAYER]);
}

void save_game(int slotn, const String &descript, std::unique_ptr<Bitmap> &&image)
{
    pl_run_plugin_hooks(kPluginEvt_PreSaveGame, 0);

    String nametouse = get_save_game_path(slotn);
    if (!image && (game.options[OPT_SAVESCREENSHOT] != 0))
        image = create_savegame_screenshot();

    HSaveError err = SaveGame(nametouse, descript, image.get(),
             (SaveCmpSelection)(kSaveCmp_All & ~(game.options[OPT_SAVECOMPONENTSIGNORE] & kSaveCmp_ScriptIgnoreMask)),
             usetup.CompressSaves);
    if (!err)
    {
        // FIXME: left this original Display call for the time being,
        // but this is wrong. Figure out how to use event callback for this instead:
        // either pass error code to kScriptEvent_GameSaved, or have a separate event for errors
        Display("ERROR: Unable to open savegame file for writing!");
        Debug::Printf(kDbgMsg_Error, "Save game failed: %s", err->FullMessage().GetCStr());
        return;
    }

    // call "After Save" event callback
    run_on_event(kScriptEvent_GameSaved, slotn);
}

int gameHasBeenRestored = 0;
int oldeip;

bool read_savedgame_description(const String &filename, String &description)
{
    if (!DoesSavegameExist(filename))
    {
        Debug::Printf("Save does not exist: %s", filename.GetCStr());
        return false;
    }

    SavegameDescription desc;
    HSaveError err = ReadSaveDescription(filename, desc, kSvgDesc_UserText);
    if (!err)
    {
        Debug::Printf(kDbgMsg_Error, "Unable to read save's description.\n%s", err->FullMessage().GetCStr());
        return false;
    }
    description = desc.UserText;
    return true;
}

std::unique_ptr<Bitmap> read_savedgame_screenshot(const String &filename)
{
    if (!DoesSavegameExist(filename))
    {
        Debug::Printf("Save does not exist: %s", filename.GetCStr());
        return {};
    }

    SavegameDescription desc;
    HSaveError err = ReadSaveDescription(filename, desc, kSvgDesc_UserImage);
    if (!err)
    {
        Debug::Printf(kDbgMsg_Error, "Unable to read save's screenshot.\n%s", err->FullMessage().GetCStr());
        return {};
    }
    if (desc.UserImage)
    {
        desc.UserImage.reset(PrepareSpriteForUse(desc.UserImage.release(), false /* no alpha */, false /* no keep mask */));
        return std::move(desc.UserImage);
    }
    return {};
}


// Test if the game file contains expected GUID / legacy id
bool test_game_guid(const String &filepath, const String &guid, int legacy_id)
{
    std::unique_ptr<AssetManager> amgr(new AssetManager());
    if (amgr->AddLibrary(filepath) != kAssetNoError)
        return false;
    MainGameSource src;
    if (!OpenMainGameFileFromDefaultAsset(src, amgr.get()))
        return false;
    GameSetupStruct g;
    PreReadGameData(g, std::move(src.InputStream), src.DataVersion);
    if (!guid.IsEmpty())
        return guid.CompareNoCase(g.guid) == 0;
    return legacy_id == g.uniqueid;
}

HSaveError load_game(const String &path, int slotNumber, bool startup, bool &data_overwritten)
{
    data_overwritten = false;
    gameHasBeenRestored++;

    oldeip = get_our_eip();
    set_our_eip(2050);

    SavegameDescription desc;
    desc.Slot = slotNumber;
    HSaveError err = ReadSaveDescription(path, desc, (SavegameDescElem)(kSvgDesc_EnvInfo | kSvgDesc_UserText));
    // saved in incompatible enviroment
    if (!err)
        return err;
    // CHECKME: is this color depth test still essential? if yes, is there possible workaround?
    else if (desc.ColorDepth != game.GetColorDepth())
        return new SavegameError(kSvgErr_DifferentColorDepth, String::FromFormat("Running: %d-bit, saved in: %d-bit.", game.GetColorDepth(), desc.ColorDepth));

    // Match game GUIDs and test whether save was made for the different game
    if (!desc.GameGuid.IsEmpty() || desc.LegacyID != 0)
    {
        if (desc.GameGuid.Compare(game.guid) != 0 && desc.LegacyID != game.uniqueid)
        {
            // Try to find wanted game's data using game id
            String gamefile = FindGameData(ResPaths.DataDir,
                [&desc](const String &filepath) { return test_game_guid(filepath, desc.GameGuid, desc.LegacyID); });
            if (!gamefile.IsEmpty())
            {
                RunAGSGame(gamefile.GetCStr(), 0, 0);
                load_new_game_restore = slotNumber;
                return HSaveError::None();
            }
            return new SavegameError(kSvgErr_GameGuidMismatch);
        }
    }
    // If it's an older save, then do the stupid old-style filename test
    // TODO: remove filename test after deprecating old saves (pre - kSvgVersion_351)
    else if (desc.MainDataFilename.Compare(ResPaths.GamePak.Name))
    {
        String gamefile = Path::ConcatPaths(ResPaths.DataDir, desc.MainDataFilename);
        if (IsMainGameLibrary(gamefile))
        {
            RunAGSGame(desc.MainDataFilename.GetCStr(), 0, 0);
            load_new_game_restore = slotNumber;
            return HSaveError::None();
        }
        // if it does not exist, continue loading savedgame in current game, and pray for the best
        Common::Debug::Printf(kDbgMsg_Warn, "WARNING: the saved game '%s' references game file '%s' (title: '%s'), but it cannot be found in the current directory. Trying to restore in the running game instead.",
            path.GetCStr(), desc.MainDataFilename.GetCStr(), desc.GameTitle.GetCStr());
    }

    // Do the actual game state restore
    SaveRestoreFeedback feedback;
    err = RestoreSavegame(path,
        RestoreGameStateOptions(
            (SaveCmpSelection)(kSaveCmp_All & ~(game.options[OPT_SAVECOMPONENTSIGNORE] & kSaveCmp_ScriptIgnoreMask)),
            startup), feedback);
    data_overwritten = true;

    // Handle restoration error
    if (!err)
    {
        // We must detect a case when the save contains less data and requires
        // a clean game reset before trying to restore again
        if (feedback.RetryWithClearGame)
        {
            // Schedule same game file re-run and save restore
            RunAGSGame(ResPaths.GamePak.Path, 0, 0);
            load_new_game_restore = slotNumber;
            load_new_game_restore_cmp = feedback.RetryWithoutComponents;
            return HSaveError::None();
        }
        // Else bail out with error
        return err;
    }

    set_our_eip(oldeip);
    // ensure input state is reset
    ags_clear_input_state();
    // call "After Restore" event callback
    run_on_event(kScriptEvent_GameRestored, slotNumber);
    return HSaveError::None();
}

bool try_restore_save(int slot, bool startup)
{
    return try_restore_save(get_save_game_path(slot), slot, startup);
}

bool try_restore_save(const Common::String &path, int slot, bool startup)
{
    bool data_overwritten;
    Debug::Printf(kDbgMsg_Info, "Restoring saved game '%s'", path.GetCStr());
    HSaveError err = load_game(path, slot, startup, data_overwritten);
    if (!err)
    {
        String error = String::FromFormat("Unable to restore the saved game.\n%s",
            err->FullMessage().GetCStr());
        Debug::Printf(kDbgMsg_Error, "%s", error.GetCStr());
        // currently AGS cannot properly revert to stable state if some of the
        // game data was released or overwritten by the data from save file,
        // this is why we tell engine to shutdown if that happened.
        if (data_overwritten)
            quitprintf("%s", error.GetCStr());
        else
            Display("%s", error.GetCStr());
        return false;
    }
    return true;
}

void prescan_save_slots(int dest_arr_handle, int min_slot, int max_slot, int save_sort, int sort_dir, int user_param)
{
    void *dest_arr;
    IScriptObject *mgr;
    ccGetObjectAddressAndManagerFromHandle(dest_arr_handle, dest_arr, mgr);
    if (!dest_arr || strcmp(mgr->GetType(), CCDynamicArray::TypeName) != 0)
    {
        debug_script_warn("Game.PrescanSaveSlots: internal error: destination array not available, has been disposed?");
        return;
    }

    prescan_saves(static_cast<int*>(dest_arr), CCDynamicArray::GetHeader(dest_arr).GetElemCount(), min_slot, max_slot, save_sort, sort_dir);
    ccReleaseObjectReference(dest_arr_handle); // release internal handle

    run_on_event(kScriptEvent_SavesScanComplete, user_param);
}

void prescan_saves(int *dest_arr, size_t dest_count, int min_slot, int max_slot, int save_sort, int sort_dir)
{
    // Gather existing list of saves in the requested range
    // ...and sort this list according to the parameters
    std::vector<SaveListItem> saves;
    FillSaveList(saves, min_slot, max_slot, false /* no desc */, (ScriptSaveGameSortStyle)save_sort, (ScriptSortDirection)sort_dir);

    // Prescan saves from the sorted list, and fill the destination array
    int *pdst = dest_arr;
    for (const auto &save : saves)
    {
        if (pdst == dest_arr + dest_count)
            break;

        HSaveError err;
        SavegameSource src;
        SavegameDescription desc;
        desc.Slot = save.Slot;
        err = OpenSavegame(get_save_game_path(save.Slot), src, desc, (SavegameDescElem)(kSvgDesc_FileFormat | kSvgDesc_EnvInfo | kSvgDesc_UserText));
        if (!err)
        {
            debug_script_log("Prescan save slot %d: failed to open save: %s", save.Slot, err->FullMessage().GetCStr());
            continue;
        }
        // CHECKME: is this color depth test still essential? if yes, is there possible workaround?
        else if (desc.ColorDepth != game.GetColorDepth())
        {
            debug_script_log("Prescan save slot %d: mismatching game color depth (game: %d-bit, save: %d-bit)", save.Slot, game.GetColorDepth(), desc.ColorDepth);
            continue;
        }

        // Do the save prescan
        err = PrescanSaveState(src.InputStream.get(), src.Version, desc,
            RestoreGameStateOptions(
                (SaveCmpSelection)(kSaveCmp_All & ~(game.options[OPT_SAVECOMPONENTSIGNORE] & kSaveCmp_ScriptIgnoreMask)),
                false));

        if (!err)
        {
            debug_script_log("Prescan save slot %d: incompatible: %s", save.Slot, err->FullMessage().GetCStr());
            continue;
        }

        // Put valid slot into dest arr
        *(pdst++) = save.Slot;
    }

    // Fill remaining destination array with "no slot" index
    for (; pdst != dest_arr + dest_count; ++pdst)
        *pdst = -1;
}

bool is_in_cutscene()
{
    return play.in_cutscene > 0;
}

CutsceneSkipStyle get_cutscene_skipstyle()
{
    return static_cast<CutsceneSkipStyle>(play.in_cutscene);
}

void start_skipping_cutscene () {
    play.fast_forward = 1;
    // if a drop-down icon bar is up, remove it as it will pause the game
    if (ifacepopped>=0)
        remove_popup_interface(ifacepopped);

    // if a text message is currently displayed, remove it
    if (play.text_overlay_on > 0)
    {
        remove_screen_overlay(play.text_overlay_on);
        play.SetWaitSkipResult(SKIP_AUTOTIMER);
    }
}

bool check_skip_cutscene_keypress (int kgn) {
    if (IsAGSServiceKey(static_cast<eAGSKeyCode>(kgn)))
        return false;
    CutsceneSkipStyle skip = get_cutscene_skipstyle();
    if (skip == eSkipSceneAnyKey || skip == eSkipSceneKeyMouse ||
        (kgn == eAGSKeyCodeEscape && (skip == eSkipSceneEscOnly || skip == eSkipSceneEscOrRMB)))
    {
        start_skipping_cutscene();
        return true;
    }
    return false;
}

bool check_skip_cutscene_mclick(int mbut)
{
    CutsceneSkipStyle skip = get_cutscene_skipstyle();
    if (skip == eSkipSceneMouse || skip == eSkipSceneKeyMouse ||
        (mbut == kMouseRight && skip == eSkipSceneEscOrRMB))
    {
        start_skipping_cutscene();
        return true;
    }
    return false;
}

// Helper functions used by StartCutscene/EndCutscene, but also
// by SkipUntilCharacterStops
void initialize_skippable_cutscene() {
    play.end_cutscene_music = -1;
}

void stop_fast_forwarding() {
    // when the skipping of a cutscene comes to an end, update things
    play.fast_forward = 0;
    setpal();
    if (play.end_cutscene_music >= 0)
        newmusic(play.end_cutscene_music);

    // Restore actual volume of sounds
    for (int aa = 0; aa < TOTAL_AUDIO_CHANNELS; aa++)
    {
        auto* ch = AudioChans::GetChannelIfPlaying(aa);
        if (ch)
        {
            ch->set_mute(false);
        }
    }

    update_music_volume();
}

// allowHotspot0 defines whether Hotspot 0 returns LOCTYPE_HOTSPOT
// or whether it returns 0
int __GetLocationType(int xxx,int yyy, int allowHotspot0) {
    getloctype_index = 0;
    // If it's not in ProcessClick, then return 0 when over a GUI
    if ((GetGUIAt(xxx, yyy) >= 0) && (getloctype_throughgui == 0))
        return 0;

    getloctype_throughgui = 0;

    const int scrx = xxx;
    const int scry = yyy;
    VpPoint vpt = play.ScreenToRoomDivDown(xxx, yyy);
    if (vpt.second < 0)
        return 0;
    xxx = vpt.first.X;
    yyy = vpt.first.Y;
    if ((xxx>=thisroom.Width) | (xxx<0) | (yyy<0) | (yyy>=thisroom.Height))
        return 0;

    // check characters, objects and walkbehinds, work out which is
    // foremost visible to the player
    int charat = is_pos_on_character(xxx,yyy);
    int hsat = get_hotspot_at(xxx,yyy);
    int objat = GetObjectIDAtScreen(scrx, scry);

    data_to_game_coords(&xxx, &yyy);

    int wbat = thisroom.WalkBehindMask->GetPixel(xxx, yyy);

    if (wbat <= 0) wbat = 0;
    else wbat = croom->walkbehind_base[wbat];

    int winner = 0;
    // if it's an Ignore Walkbehinds object, then ignore the walkbehind
    if ((objat >= 0) && ((objs[objat].flags & OBJF_NOWALKBEHINDS) != 0))
        wbat = 0;
    if ((charat >= 0) && ((game.chars[charat].flags & CHF_NOWALKBEHINDS) != 0))
        wbat = 0;

    if ((charat >= 0) && (objat >= 0)) {
        if ((wbat > obj_lowest_yp) && (wbat > char_lowest_yp))
            winner = LOCTYPE_HOTSPOT;
        else if (obj_lowest_yp > char_lowest_yp)
            winner = LOCTYPE_OBJ;
        else
            winner = LOCTYPE_CHAR;
    }
    else if (charat >= 0) {
        if (wbat > char_lowest_yp)
            winner = LOCTYPE_HOTSPOT;
        else
            winner = LOCTYPE_CHAR;
    }
    else if (objat >= 0) {
        if (wbat > obj_lowest_yp)
            winner = LOCTYPE_HOTSPOT;
        else
            winner = LOCTYPE_OBJ;
    }

    if (winner == 0) {
        if (hsat >= 0)
            winner = LOCTYPE_HOTSPOT;
    }

    if ((winner == LOCTYPE_HOTSPOT) && (!allowHotspot0) && (hsat == 0))
        winner = 0;

    if (winner == LOCTYPE_HOTSPOT)
        getloctype_index = hsat;
    else if (winner == LOCTYPE_CHAR)
        getloctype_index = charat;
    else if (winner == LOCTYPE_OBJ)
        getloctype_index = objat;

    return winner;
}

// Called whenever game loses input focus
void display_switch_out()
{
    Debug::Printf("Switching out from the game");
    switched_away = true;
    ags_clear_input_state();
    // Always unlock mouse when switching out from the game
    Mouse::UnlockFromWindow();
}

// Called when game loses input focus and must pause until focus is returned
void display_switch_out_suspend()
{
    Debug::Printf("Suspending the game on switch out");
    switching_away_from_game++;
    game_update_suspend = true;
    display_switch_out();

    platform->PauseApplication();

    // TODO: find out if anything has to be done here for SDL backend

    video_single_pause();
    // Pause all the sounds
    for (int i = 0; i < TOTAL_AUDIO_CHANNELS; i++) {
        auto* ch = AudioChans::GetChannelIfPlaying(i);
        if (ch) {
            ch->pause();
        }
    }

    switching_away_from_game--;
}

// Called whenever game gets input focus
void display_switch_in()
{
    Debug::Printf("Switching back into the game");
    ags_clear_input_state();
    // If fullscreen, or auto lock option is set, lock mouse to the game window
    if ((scsystem.windowed == 0) || usetup.MouseAutoLock)
        Mouse::TryLockToWindow();
    switched_away = false;
}

// Called when game gets input focus and must resume after pause
void display_switch_in_resume()
{
    Debug::Printf("Resuming the game on switch in");
    display_switch_in();

    // Resume all the sounds
    for (int i = 0; i < TOTAL_AUDIO_CHANNELS; i++) {
        auto* ch = AudioChans::GetChannelIfPlaying(i);
        if (ch) {
            ch->resume();
        }
    }
    video_single_resume();

    // release render targets if switching back to the full screen mode;
    // unfortunately, otherwise Direct3D fails to reset device when restoring fullscreen.
    if (gfxDriver && gfxDriver->GetDisplayMode().IsRealFullscreen())
        release_drawobj_rendertargets();
    // clear the screen if necessary
    if (gfxDriver && gfxDriver->UsesMemoryBackBuffer())
        gfxDriver->ClearRectangle(0, 0, game.GetGameRes().Width - 1, game.GetGameRes().Height - 1, nullptr);

    // TODO: find out if anything has to be done here for SDL backend

    platform->ResumeApplication();
    game_update_suspend = false;
}

void replace_tokens(const char*srcmes,char*destm, size_t maxlen) {
    size_t indxdest=0,indxsrc=0;
    const char*srcp;
    char *destp;
    while (srcmes[indxsrc]!=0) {
        srcp=&srcmes[indxsrc];
        destp=&destm[indxdest];
        if ((strncmp(srcp,"@IN",3)==0) || (strncmp(srcp,"@GI",3)==0)) {
            int tokentype=0;
            if (srcp[1]=='I') tokentype=1;
            else tokentype=2;
            int inx=atoi(&srcp[3]);
            srcp++;
            indxsrc+=2;
            while (srcp[0]!='@') {
                if (srcp[0]==0) quit("!Display: special token not terminated");
                srcp++;
                indxsrc++;
            }
            char tval[10];
            if (tokentype==1) {
                if ((inx<1) | (inx>=game.numinvitems))
                    quit("!Display: invalid inv item specified in @IN@");
                snprintf(tval,sizeof(tval),"%d",playerchar->inv[inx]);
            }
            else {
                if ((inx<0) | (inx>=MAXGSVALUES))
                    quit("!Display: invalid global int index speicifed in @GI@");
                snprintf(tval,sizeof(tval),"%d",GetGlobalInt(inx));
            }
            snprintf(destp, maxlen, "%s", tval);
            indxdest+=strlen(tval);
        }
        else {
            destp[0]=srcp[0];
            indxdest++;
            indxsrc++;
        }
        if (indxdest >= maxlen - 3)
            break;
    }
    destm[indxdest]=0;
}

const char *get_global_message (int msnum) {
    if (game.messages[msnum - 500].IsEmpty())
        return "";
    return get_translation(game.messages[msnum - 500].GetCStr());
}

void get_message_text (int msnum, char *buffer, char giveErr) {
    int maxlen = 9999;
    if (!giveErr)
        maxlen = MAX_MAXSTRLEN;

    if (msnum>=500) {

        if ((msnum >= MAXGLOBALMES + 500) || (game.messages[msnum-500].IsEmpty())) {
            if (giveErr)
                quit("!DisplayGlobalMessage: message does not exist");
            buffer[0] = 0;
            return;
        }
        buffer[0] = 0;
        replace_tokens(get_translation(game.messages[msnum-500].GetCStr()), buffer, maxlen);
        return;
    }
    else if (msnum < 0 || (uint32_t)msnum >= thisroom.MessageCount) {
        if (giveErr)
            quit("!DisplayMessage: Invalid message number to display");
        buffer[0] = 0;
        return;
    }

    buffer[0]=0;
    replace_tokens(get_translation(thisroom.Messages[msnum].GetCStr()), buffer, maxlen);
}

void game_sprite_updated(int sprnum, bool deleted)
{
    // Notify draw system about dynamic sprite change
    notify_sprite_changed(sprnum, deleted);

    // GUI still have a special draw route, so cannot rely on object caches;
    // will have to do a per-GUI and per-control check.
    // TODO: devise a way to speed this iteration up, perhaps link GUI objects
    // to the sprite notification block somehow, etc...?
    //
    // gui backgrounds
    for (auto &gui : guis)
    {
        if (gui.GetBgImage() == sprnum)
        {
            gui.MarkChanged();
        }
    }
    // gui buttons
    for (auto &but : guibuts)
    {
        if (but.GetCurrentImage() == sprnum)
        {
            but.MarkChanged();
        }
    }
    // gui sliders
    for (auto &slider : guislider)
    {
        if ((slider.GetBgImage() == sprnum) || (slider.GetHandleImage() == sprnum))
        {
            slider.MarkChanged();
        }
    }
}

void precache_view(int view, int first_loop, int last_loop, bool with_sounds)
{
    if (view < 0)
        return;
    if (first_loop > last_loop)
        return;

    first_loop = Math::Clamp(first_loop, 0, views[view].numLoops - 1);
    last_loop = Math::Clamp(last_loop, 0, views[view].numLoops - 1);

    // Record cache sizes and timestamps, for diagnostic purposes
    const size_t spcache_before = spriteset.GetCacheSize();
    const size_t txcache_before = texturecache_get_size();
    int total_frames = 0, total_sounds = 0;

    int64_t dur_sp_load = 0, dur_tx_make = 0, dur_sound_load = 0;
    for (int i = first_loop; i <= last_loop; ++i)
    {
        for (int j = 0; j < views[view].loops[i].numFrames; ++j, ++total_frames)
        {
            const auto &frame = views[view].loops[i].frames[j];
            const auto tp_detail1 = FastClock::now();
            spriteset.PrecacheSprite(frame.pic);
            const auto tp_detail2 = FastClock::now();
            texturecache_precache(frame.pic);
            const auto tp_detail3 = FastClock::now();

            if (with_sounds && frame.audioclip >= 0)
            {
                ScriptAudioClip *clip = &game.audioClips[frame.audioclip];
                auto assetpath = get_audio_clip_assetpath(clip->bundlingType, clip->fileName);
                soundcache_precache(assetpath);
                dur_sound_load += ToMilliseconds(FastClock::now() - tp_detail3);
                total_sounds++;
            }
            dur_sp_load += ToMilliseconds(tp_detail2 - tp_detail1);
            dur_tx_make += ToMilliseconds(tp_detail3 - tp_detail2);
        }
    }

    if (total_frames == 0)
    {
        Debug::Printf("Precache view %d (loops %d-%d): %d frames found in the view, skip");
        return;
    }

    // Print gathered time and size info
    size_t spcache_after = spriteset.GetCacheSize();
    size_t txcache_after = texturecache_get_size();
    Debug::Printf("Precache view %d (loops %d-%d) with %d frames, total = %lld ms, average file->mem = %lld ms, bm->tx = %lld ms,"
                  "\n\t\tloaded %d sounds = %lld ms",
        view, first_loop, last_loop, total_frames, dur_sp_load + dur_tx_make + dur_sound_load,
        dur_sp_load / total_frames, dur_tx_make / total_frames, total_sounds, dur_sound_load);
    Debug::Printf("\tSprite cache: %zu -> %zu KB, texture cache: %zu -> %zu KB",
        spcache_before / 1024u, spcache_after / 1024u, txcache_before / 1024u, txcache_after / 1024u);
}

//=============================================================================
//
// GameInfo API
//
//=============================================================================

const char *GameInfo_GetTitle()
{
    return CreateNewScriptString(game.GameInfo["title"]);
}

const char *GameInfo_GetDescription()
{
    return CreateNewScriptString(game.GameInfo["description"]);
}

const char *GameInfo_GetDeveloperName()
{
    return CreateNewScriptString(game.GameInfo["dev_name"]);
}

const char *GameInfo_GetDeveloperURL()
{
    return CreateNewScriptString(game.GameInfo["dev_url"]);
}

const char *GameInfo_GetGenre()
{
    return CreateNewScriptString(game.GameInfo["genre"]);
}

ScriptDateTime *GameInfo_GetReleaseDate()
{
    String release_date = game.GameInfo["release_date"];
    if (release_date.IsEmpty())
        return nullptr; // not available
    std::vector<String> parts = release_date.Split('.');
    if (parts.size() == 0 || parts.size() > 3)
        return nullptr; // malformed date
    int year = 0, month = 0, day = 0;
    year = StrUtil::StringToInt(parts[parts.size() - 1], -1);
    if (parts.size() > 1)
        month = StrUtil::StringToInt(parts[parts.size() - 2], -1);
    if (parts.size() > 2)
        day = StrUtil::StringToInt(parts[parts.size() - 3], -1);
    if (year < 0 || month < 0 || day < 0)
        return nullptr; // malformed date
    return DateTime_CreateFromDate(year, month, day, 0, 0, 0);
}

const char *GameInfo_GetVersion()
{
    return CreateNewScriptString(game.GameInfo["version"]);
}

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "debug/out.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

// int  (int audioType);
RuntimeScriptValue Sc_Game_IsAudioPlaying(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(Game_IsAudioPlaying);
}

// void (int audioType, int volumeDrop)
RuntimeScriptValue Sc_Game_SetAudioTypeSpeechVolumeDrop(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(Game_SetAudioTypeSpeechVolumeDrop);
}

// void (int audioType, int volume, int changeType)
RuntimeScriptValue Sc_Game_SetAudioTypeVolume(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(Game_SetAudioTypeVolume);
}

// void (int audioType)
RuntimeScriptValue Sc_Game_StopAudio(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Game_StopAudio);
}

// int (const char *newFilename)
RuntimeScriptValue Sc_Game_ChangeTranslation(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ(Game_ChangeTranslation, const char);
}

RuntimeScriptValue Sc_Game_ChangeSpeechVox(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_BOOL_POBJ(Game_ChangeSpeechVox, const char);
}

// int (const char *token)
RuntimeScriptValue Sc_Game_DoOnceOnly(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ(Game_DoOnceOnly, const char);
}

RuntimeScriptValue Sc_Game_ResetDoOnceOnly(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(Game_ResetDoOnceOnly);
}

// int (int red, int grn, int blu)
RuntimeScriptValue Sc_Game_GetColorFromRGB(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT3(Game_GetColorFromRGB);
}

// int (int viewNumber, int loopNumber)
RuntimeScriptValue Sc_Game_GetFrameCountForLoop(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(Game_GetFrameCountForLoop);
}

// const char* (int x, int y)
RuntimeScriptValue Sc_Game_GetLocationName(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT2(const char, myScriptStringImpl, Game_GetLocationName);
}

// int (int viewNumber)
RuntimeScriptValue Sc_Game_GetLoopCountForView(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(Game_GetLoopCountForView);
}

// int ()
RuntimeScriptValue Sc_Game_GetMODPattern(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetMODPattern);
}

// int (int viewNumber, int loopNumber)
RuntimeScriptValue Sc_Game_GetRunNextSettingForLoop(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(Game_GetRunNextSettingForLoop);
}

// const char* (int slnum)
RuntimeScriptValue Sc_Game_GetSaveSlotDescription(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT(const char, myScriptStringImpl, Game_GetSaveSlotDescription);
}

RuntimeScriptValue Sc_Game_GetSaveSlotTime(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT(ScriptDateTime, Game_GetSaveSlotTime);
}

// ScriptViewFrame* (int viewNumber, int loopNumber, int frame)
RuntimeScriptValue Sc_Game_GetViewFrame(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT3(ScriptViewFrame, Game_GetViewFrame);
}

// const char* (const char *msg)
RuntimeScriptValue Sc_Game_InputBox(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_POBJ(const char, myScriptStringImpl, Game_InputBox, const char);
}

// int (const char *newFolder)
RuntimeScriptValue Sc_Game_SetSaveGameDirectory(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ(Game_SetSaveGameDirectory, const char);
}

// void (int evenAmbient);
RuntimeScriptValue Sc_StopAllSounds(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(StopAllSounds);
}

// int ()
RuntimeScriptValue Sc_Game_GetCharacterCount(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetCharacterCount);
}

// int ()
RuntimeScriptValue Sc_Game_GetDialogCount(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetDialogCount);
}

// const char *()
RuntimeScriptValue Sc_Game_GetFileName(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ(const char, myScriptStringImpl, Game_GetFileName);
}

// int ()
RuntimeScriptValue Sc_Game_GetFontCount(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetFontCount);
}

// const char* (int index)
RuntimeScriptValue Sc_Game_GetGlobalMessages(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT(const char, myScriptStringImpl, Game_GetGlobalMessages);
}

// const char* (int index)
RuntimeScriptValue Sc_Game_GetGlobalStrings(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT(const char, myScriptStringImpl, Game_GetGlobalStrings);
}

// void  (int index, char *newval);
RuntimeScriptValue Sc_SetGlobalString(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT_POBJ(SetGlobalString, const char);
}

// int ()
RuntimeScriptValue Sc_Game_GetGUICount(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetGUICount);
}

// int ()
RuntimeScriptValue Sc_Game_GetIgnoreUserInputAfterTextTimeoutMs(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetIgnoreUserInputAfterTextTimeoutMs);
}

// void (int newValueMs)
RuntimeScriptValue Sc_Game_SetIgnoreUserInputAfterTextTimeoutMs(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Game_SetIgnoreUserInputAfterTextTimeoutMs);
}

// int ()
RuntimeScriptValue Sc_Game_GetInSkippableCutscene(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetInSkippableCutscene);
}

// int ()
RuntimeScriptValue Sc_Game_GetInventoryItemCount(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetInventoryItemCount);
}

// int ()
RuntimeScriptValue Sc_Game_GetMinimumTextDisplayTimeMs(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetMinimumTextDisplayTimeMs);
}

// void (int newTextMinTime)
RuntimeScriptValue Sc_Game_SetMinimumTextDisplayTimeMs(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Game_SetMinimumTextDisplayTimeMs);
}

// int ()
RuntimeScriptValue Sc_Game_GetMouseCursorCount(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetMouseCursorCount);
}

// const char *()
RuntimeScriptValue Sc_Game_GetName(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ(const char, myScriptStringImpl, Game_GetName);
}

// void (const char *newName)
RuntimeScriptValue Sc_Game_SetName(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_POBJ(Game_SetName, const char);
}

// int ()
RuntimeScriptValue Sc_Game_GetNormalFont(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetNormalFont);
}

// void  (int fontnum);
RuntimeScriptValue Sc_SetNormalFont(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(SetNormalFont);
}

// int ()
RuntimeScriptValue Sc_Game_GetSkippingCutscene(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetSkippingCutscene);
}

// int ()
RuntimeScriptValue Sc_Game_GetSpeechFont(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetSpeechFont);
}

// void  (int fontnum);
RuntimeScriptValue Sc_SetSpeechFont(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(SetSpeechFont);
}

// int (int spriteNum)
RuntimeScriptValue Sc_Game_GetSpriteWidth(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(Game_GetSpriteWidth);
}

// int (int spriteNum)
RuntimeScriptValue Sc_Game_GetSpriteHeight(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(Game_GetSpriteHeight);
}

// int ()
RuntimeScriptValue Sc_Game_GetTextReadingSpeed(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetTextReadingSpeed);
}

// void (int newTextSpeed)
RuntimeScriptValue Sc_Game_SetTextReadingSpeed(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Game_SetTextReadingSpeed);
}

// const char* ()
RuntimeScriptValue Sc_Game_GetTranslationFilename(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ(const char, myScriptStringImpl, Game_GetTranslationFilename);
}

RuntimeScriptValue Sc_Game_GetSpeechVoxFilename(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ(const char, myScriptStringImpl, Game_GetSpeechVoxFilename);
}

// int ()
RuntimeScriptValue Sc_Game_GetUseNativeCoordinates(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetUseNativeCoordinates);
}

// int ()
RuntimeScriptValue Sc_Game_GetViewCount(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetViewCount);
}

RuntimeScriptValue Sc_Game_GetAudioClipCount(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetAudioClipCount);
}

RuntimeScriptValue Sc_Game_GetAudioClip(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT(ScriptAudioClip, ccDynamicAudioClip, Game_GetAudioClip);
}

RuntimeScriptValue Sc_Game_IsPluginLoaded(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_BOOL_POBJ(pl_is_plugin_loaded, const char);
}

RuntimeScriptValue Sc_Game_PlayVoiceClip(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_POBJ_PINT_PBOOL(ScriptAudioChannel, ccDynamicAudio, PlayVoiceClip, CharacterInfo);
}

RuntimeScriptValue Sc_Game_GetCamera(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO(ScriptCamera, Game_GetCamera);
}

RuntimeScriptValue Sc_Game_GetCameraCount(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetCameraCount);
}

RuntimeScriptValue Sc_Game_GetAnyCamera(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT(ScriptCamera, Game_GetAnyCamera);
}

RuntimeScriptValue Sc_Game_SimulateKeyPress(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Game_SimulateKeyPress);
}

RuntimeScriptValue Sc_Game_BlockingWaitSkipped(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_BlockingWaitSkipped);
}

RuntimeScriptValue Sc_Game_InBlockingWait(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_BOOL(Game_InBlockingWait);
}

RuntimeScriptValue Sc_Game_PrecacheSprite(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Game_PrecacheSprite);
}

RuntimeScriptValue Sc_Game_PrecacheView(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(Game_PrecacheView);
}

RuntimeScriptValue Sc_Game_GetSaveSlots(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT4(void, globalDynamicArray, Game_GetSaveSlots);
}

RuntimeScriptValue Sc_Game_ScanSaveSlots(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_POBJ_PINT5(Game_ScanSaveSlots, void);
}

RuntimeScriptValue Sc_GameInfo_GetTitle(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ(const char, myScriptStringImpl, GameInfo_GetTitle);
}

RuntimeScriptValue Sc_GameInfo_GetDescription(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ(const char, myScriptStringImpl, GameInfo_GetDescription);
}

RuntimeScriptValue Sc_GameInfo_GetDeveloperName(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ(const char, myScriptStringImpl, GameInfo_GetDeveloperName);
}

RuntimeScriptValue Sc_GameInfo_GetDeveloperURL(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ(const char, myScriptStringImpl, GameInfo_GetDeveloperURL);
}

RuntimeScriptValue Sc_GameInfo_GetGenre(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ(const char, myScriptStringImpl, GameInfo_GetGenre);
}

RuntimeScriptValue Sc_GameInfo_GetReleaseDate(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO(ScriptDateTime, GameInfo_GetReleaseDate);
}

RuntimeScriptValue Sc_GameInfo_GetVersion(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ(const char, myScriptStringImpl, GameInfo_GetVersion);
}


void RegisterGameAPI()
{
    ScFnRegister game_api[] = {
        { "Game::IsAudioPlaying^1",                       API_FN_PAIR(Game_IsAudioPlaying) },
        { "Game::SetAudioTypeSpeechVolumeDrop^2",         API_FN_PAIR(Game_SetAudioTypeSpeechVolumeDrop) },
        { "Game::SetAudioTypeVolume^3",                   API_FN_PAIR(Game_SetAudioTypeVolume) },
        { "Game::StopAudio^1",                            API_FN_PAIR(Game_StopAudio) },
        { "Game::ChangeTranslation^1",                    API_FN_PAIR(Game_ChangeTranslation) },
        { "Game::DoOnceOnly^1",                           API_FN_PAIR(Game_DoOnceOnly) },
        { "Game::GetColorFromRGB^3",                      API_FN_PAIR(Game_GetColorFromRGB) },
        { "Game::GetFrameCountForLoop^2",                 API_FN_PAIR(Game_GetFrameCountForLoop) },
        { "Game::GetLocationName^2",                      API_FN_PAIR(Game_GetLocationName) },
        { "Game::GetLoopCountForView^1",                  API_FN_PAIR(Game_GetLoopCountForView) },
        { "Game::GetMODPattern^0",                        API_FN_PAIR(Game_GetMODPattern) },
        { "Game::GetRunNextSettingForLoop^2",             API_FN_PAIR(Game_GetRunNextSettingForLoop) },
        { "Game::GetSaveSlotDescription^1",               API_FN_PAIR(Game_GetSaveSlotDescription) },
        { "Game::GetSaveSlotTime^1",                      API_FN_PAIR(Game_GetSaveSlotTime) },
        { "Game::GetViewFrame^3",                         API_FN_PAIR(Game_GetViewFrame) },
        { "Game::InputBox^1",                             API_FN_PAIR(Game_InputBox) },
        { "Game::SetSaveGameDirectory^1",                 API_FN_PAIR(Game_SetSaveGameDirectory) },
        { "Game::StopSound^1",                            API_FN_PAIR(StopAllSounds) },
        { "Game::IsPluginLoaded",                         Sc_Game_IsPluginLoaded, pl_is_plugin_loaded },
        { "Game::ChangeSpeechVox",                        API_FN_PAIR(Game_ChangeSpeechVox) },
        { "Game::PlayVoiceClip",                          Sc_Game_PlayVoiceClip, PlayVoiceClip },
        { "Game::SimulateKeyPress",                       API_FN_PAIR(Game_SimulateKeyPress) },
        { "Game::ResetDoOnceOnly",                        API_FN_PAIR(Game_ResetDoOnceOnly) },
        { "Game::PrecacheSprite",                         API_FN_PAIR(Game_PrecacheSprite) },
        { "Game::PrecacheView",                           API_FN_PAIR(Game_PrecacheView) },
        { "Game::GetSaveSlots^4",                         API_FN_PAIR(Game_GetSaveSlots) },
        { "Game::ScanSaveSlots^6",                        API_FN_PAIR(Game_ScanSaveSlots) },
        { "Game::get_AudioClipCount",                     API_FN_PAIR(Game_GetAudioClipCount) },
        { "Game::geti_AudioClips",                        API_FN_PAIR(Game_GetAudioClip) },
        { "Game::get_BlockingWaitSkipped",                API_FN_PAIR(Game_BlockingWaitSkipped) },
        { "Game::get_Camera",                             API_FN_PAIR(Game_GetCamera) },
        { "Game::get_CameraCount",                        API_FN_PAIR(Game_GetCameraCount) },
        { "Game::geti_Cameras",                           API_FN_PAIR(Game_GetAnyCamera) },
        { "Game::get_CharacterCount",                     API_FN_PAIR(Game_GetCharacterCount) },
        { "Game::get_DialogCount",                        API_FN_PAIR(Game_GetDialogCount) },
        { "Game::get_FileName",                           API_FN_PAIR(Game_GetFileName) },
        { "Game::get_FontCount",                          API_FN_PAIR(Game_GetFontCount) },
        { "Game::geti_GlobalMessages",                    API_FN_PAIR(Game_GetGlobalMessages) },
        { "Game::geti_GlobalStrings",                     API_FN_PAIR(Game_GetGlobalStrings) },
        { "Game::seti_GlobalStrings",                     API_FN_PAIR(SetGlobalString) },
        { "Game::get_GUICount",                           API_FN_PAIR(Game_GetGUICount) },
        { "Game::get_IgnoreUserInputAfterTextTimeoutMs",  API_FN_PAIR(Game_GetIgnoreUserInputAfterTextTimeoutMs) },
        { "Game::set_IgnoreUserInputAfterTextTimeoutMs",  API_FN_PAIR(Game_SetIgnoreUserInputAfterTextTimeoutMs) },
        { "Game::get_InBlockingWait",                     API_FN_PAIR(Game_InBlockingWait) },
        { "Game::get_InSkippableCutscene",                API_FN_PAIR(Game_GetInSkippableCutscene) },
        { "Game::get_InventoryItemCount",                 API_FN_PAIR(Game_GetInventoryItemCount) },
        { "Game::get_MinimumTextDisplayTimeMs",           API_FN_PAIR(Game_GetMinimumTextDisplayTimeMs) },
        { "Game::set_MinimumTextDisplayTimeMs",           API_FN_PAIR(Game_SetMinimumTextDisplayTimeMs) },
        { "Game::get_MouseCursorCount",                   API_FN_PAIR(Game_GetMouseCursorCount) },
        { "Game::get_Name",                               API_FN_PAIR(Game_GetName) },
        { "Game::set_Name",                               API_FN_PAIR(Game_SetName) },
        { "Game::get_NormalFont",                         API_FN_PAIR(Game_GetNormalFont) },
        { "Game::set_NormalFont",                         API_FN_PAIR(SetNormalFont) },
        { "Game::get_SkippingCutscene",                   API_FN_PAIR(Game_GetSkippingCutscene) },
        { "Game::get_SpeechFont",                         API_FN_PAIR(Game_GetSpeechFont) },
        { "Game::set_SpeechFont",                         API_FN_PAIR(SetSpeechFont) },
        { "Game::get_SpeechVoxFilename",                  API_FN_PAIR(Game_GetSpeechVoxFilename) },
        { "Game::geti_SpriteWidth",                       API_FN_PAIR(Game_GetSpriteWidth) },
        { "Game::geti_SpriteHeight",                      API_FN_PAIR(Game_GetSpriteHeight) },
        { "Game::get_TextReadingSpeed",                   API_FN_PAIR(Game_GetTextReadingSpeed) },
        { "Game::set_TextReadingSpeed",                   API_FN_PAIR(Game_SetTextReadingSpeed) },
        { "Game::get_TranslationFilename",                API_FN_PAIR(Game_GetTranslationFilename) },
        { "Game::get_UseNativeCoordinates",               API_FN_PAIR(Game_GetUseNativeCoordinates) },
        { "Game::get_ViewCount",                          API_FN_PAIR(Game_GetViewCount) },

        { "GameInfo::get_Title",                          API_FN_PAIR(GameInfo_GetTitle) },
        { "GameInfo::get_Description",                    API_FN_PAIR(GameInfo_GetDescription) },
        { "GameInfo::get_DeveloperName",                  API_FN_PAIR(GameInfo_GetDeveloperName) },
        { "GameInfo::get_DeveloperURL",                   API_FN_PAIR(GameInfo_GetDeveloperURL) },
        { "GameInfo::get_Genre",                          API_FN_PAIR(GameInfo_GetGenre) },
        { "GameInfo::get_ReleaseDate",                    API_FN_PAIR(GameInfo_GetReleaseDate) },
        { "GameInfo::get_Version",                        API_FN_PAIR(GameInfo_GetVersion) },
    };

    ccAddExternalFunctions(game_api);
}

void RegisterStaticObjects()
{
    ccAddExternalScriptObject("game", &play, &GameStaticManager);
    ccAddExternalScriptObject("gs_globals", &play.globalvars[0], &GlobalStaticManager);
    ccAddExternalScriptObject("mouse", &scmouse, &scmouse);
    ccAddExternalScriptObject("palette", &palette[0], &GlobalStaticManager);
    ccAddExternalScriptObject("system", &scsystem, &scsystem);
    ccAddExternalScriptObject("savegameindex", &play.filenumbers[0], &GlobalStaticManager);
}
