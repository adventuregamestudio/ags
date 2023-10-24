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
#include "ac/roomstatus.h"
#include "ac/sprite.h"
#include "ac/spritecache.h"
#include "ac/string.h"
#include "ac/translation.h"
#include "ac/dynobj/all_dynamicclasses.h"
#include "ac/dynobj/all_scriptclasses.h"
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
#include "gui/guislider.h"
#include "gui/guidialog.h"
#include "main/engine.h"
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
GameState play;
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
int displayed_room=-10,starting_room = -1;
int in_new_room=0, new_room_was = 0;  // 1 in new room, 2 first time in new room, 3 loading saved game
int new_room_pos=0;
int new_room_x = SCR_NO_VALUE, new_room_y = SCR_NO_VALUE;
int new_room_loop = SCR_NO_VALUE;
bool new_room_placeonwalkable = false;
int proper_exit=0,our_eip=0;

AGS::Common::SpriteCache::Callbacks spritecallbacks = {
    nullptr,
    initialize_sprite,
    post_init_sprite,
    nullptr
};
SpriteCache spriteset(game.SpriteInfos, spritecallbacks);

std::vector<GUIMain> guis;

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
ScriptString myScriptStringImpl;
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
char pexbuf[STD_BUFFER_SIZE];

unsigned int load_new_game = 0;
int load_new_game_restore = -1;

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

extern int cbuttfont;
extern int acdialog_font;

int oldmouse;
void setup_for_dialog() {
    cbuttfont = play.normal_font;
    acdialog_font = play.normal_font;
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
    else
    {
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


void restore_game_dialog() {
    can_run_delayed_command();
    if (inside_script) {
        curscript->queue_action(ePSARestoreGameDialog, 0, "RestoreGameDialog");
        return;
    }
    do_restore_game_dialog();
}

bool do_restore_game_dialog() {
    setup_for_dialog();
    int toload = loadgamedialog();
    restore_after_dialog();
    if (toload >= 0)
        try_restore_save(toload);
    return toload >= 0;
}

void save_game_dialog() {
    if (inside_script) {
        curscript->queue_action(ePSASaveGameDialog, 0, "SaveGameDialog");
        return;
    }
    do_save_game_dialog();
}

bool do_save_game_dialog() {
    setup_for_dialog();
    int toload = savegamedialog();
    restore_after_dialog();
    if (toload >= 0)
        save_game(toload, get_gui_dialog_buffer());
    return toload >= 0;
}

void free_do_once_tokens()
{
    play.do_once_tokens.clear();
}


// Free all the memory associated with the game
void unload_game_file()
{
    dispose_game_drawdata();
    // NOTE: fonts should be freed prior to stopping plugins,
    // as plugins may provide font renderer interface.
    free_all_fonts();
    close_translation();

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

    // Free game state and game struct
    play = GameState();
    game = GameSetupStruct();

    // Reset all resource caches
    // IMPORTANT: this is hard reset, including locked items
    spriteset.Reset();
    soundcache_clear();
}

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

int Game_GetSpriteWidth(int spriteNum) {
    if (spriteNum < 0)
        return 0;

    if (!spriteset.DoesSpriteExist(spriteNum))
        return 0;

    return game.SpriteInfos[spriteNum].Width;
}

int Game_GetSpriteHeight(int spriteNum) {
    if (spriteNum < 0)
        return 0;

    if (!spriteset.DoesSpriteExist(spriteNum))
        return 0;

    return game.SpriteInfos[spriteNum].Height;
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

    play.text_speed = newTextSpeed;
}

int Game_GetMinimumTextDisplayTimeMs()
{
    return play.text_min_display_time_ms;
}

void Game_SetMinimumTextDisplayTimeMs(int newTextMinTime)
{
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
    strncpy(play.game_name, newName, 99);
    play.game_name[99] = 0;
    sys_window_set_title(play.game_name);
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
    sc_inputbox(msg, buffer);
    return CreateNewScriptString(buffer);
}

const char* Game_GetLocationName(int x, int y) {
    char buffer[STD_BUFFER_SIZE];
    GetLocationName(x, y, buffer);
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
    GetTranslationName(buffer);
    return CreateNewScriptString(buffer);
}

int Game_ChangeTranslation(const char *newFilename)
{
    if ((newFilename == nullptr) || (newFilename[0] == 0))
    { // switch back to default translation
        close_translation();
        usetup.translation = "";
        GUI::MarkForTranslationUpdate();
        return 1;
    }

    String oldTransFileName = get_translation_name();
    if (!init_translation(newFilename, oldTransFileName))
        return 0; // failed, kept previous translation

    usetup.translation = newFilename;
    GUI::MarkForTranslationUpdate();
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
    ags_simulate_keypress(static_cast<eAGSKeyCode>(key));
}

int Game_BlockingWaitSkipped()
{
    return play.GetWaitSkipResult();
}

//=============================================================================

// save game functions



void serialize_bitmap(const Common::Bitmap *thispic, Stream *out) {
    if (thispic != nullptr) {
        out->WriteInt32(thispic->GetWidth());
        out->WriteInt32(thispic->GetHeight());
        out->WriteInt32(thispic->GetColorDepth());
        for (int cc=0;cc<thispic->GetHeight();cc++)
        {
          switch (thispic->GetColorDepth())
          {
          case 8:
          // CHECKME: originally, AGS does not use real BPP here, but simply divides color depth by 8;
          // therefore 15-bit bitmaps are saved only partially? is this a bug? or?
          case 15:
            out->WriteArray(&thispic->GetScanLine(cc)[0], thispic->GetWidth(), 1);
            break;
          case 16:
            out->WriteArrayOfInt16((const int16_t*)&thispic->GetScanLine(cc)[0], thispic->GetWidth());
            break;
          case 32:
            out->WriteArrayOfInt32((const int32_t*)&thispic->GetScanLine(cc)[0], thispic->GetWidth());
            break;
          }
        }
    }
}

Bitmap *read_serialized_bitmap(Stream *in) {
    Bitmap *thispic;
    int picwid = in->ReadInt32();
    int pichit = in->ReadInt32();
    int piccoldep = in->ReadInt32();
    thispic = BitmapHelper::CreateBitmap(picwid,pichit,piccoldep);
    if (thispic == nullptr)
        return nullptr;
    for (int vv=0; vv < pichit; vv++)
    {
      switch (piccoldep)
      {
      case 8:
      // CHECKME: originally, AGS does not use real BPP here, but simply divides color depth by 8
      case 15:
        in->ReadArray(thispic->GetScanLineForWriting(vv), picwid, 1);
        break;
      case 16:
        in->ReadArrayOfInt16((int16_t*)thispic->GetScanLineForWriting(vv), picwid);
        break;
      case 32:
        in->ReadArrayOfInt32((int32_t*)thispic->GetScanLineForWriting(vv), picwid);
        break;
      }
    }

    return thispic;
}

void skip_serialized_bitmap(Stream *in)
{
    int picwid = in->ReadInt32();
    int pichit = in->ReadInt32();
    int piccoldep = in->ReadInt32();
    // CHECKME: originally, AGS does not use real BPP here, but simply divides color depth by 8
    int bpp = piccoldep / 8;
    in->Seek(picwid * pichit * bpp);
}

Bitmap *create_savegame_screenshot()
{
    int usewid = play.screenshot_width;
    int usehit = play.screenshot_height;
    const Rect &viewport = play.GetMainViewport();
    if (usewid > viewport.GetWidth())
        usewid = viewport.GetWidth();
    if (usehit > viewport.GetHeight())
        usehit = viewport.GetHeight();

    if ((play.screenshot_width < 16) || (play.screenshot_height < 16))
        quit("!Invalid game.screenshot_width/height, must be from 16x16 to screen res");

    return CopyScreenIntoBitmap(usewid, usehit);
}

void save_game(int slotn, const char*descript) {

    // dont allow save in rep_exec_always, because we dont save
    // the state of blocked scripts
    can_run_delayed_command();

    if (inside_script) {
        snprintf(curscript->postScriptSaveSlotDescription[curscript->queue_action(ePSASaveGame, slotn, "SaveGameSlot")],
            MAX_QUEUED_ACTION_DESC, "%s", descript);
        return;
    }

    if (platform->GetDiskFreeSpaceMB() < 2) {
        Display("ERROR: There is not enough disk space free to save the game. Clear some disk space and try again.");
        return;
    }

    VALIDATE_STRING(descript);
    String nametouse = get_save_game_path(slotn);
    std::unique_ptr<Bitmap> screenShot;
    if (game.options[OPT_SAVESCREENSHOT] != 0)
        screenShot.reset(create_savegame_screenshot());

    std::unique_ptr<Stream> out(StartSavegame(nametouse, descript, screenShot.get()));
    if (out == nullptr)
    {
        Display("ERROR: Unable to open savegame file for writing!");
        return;
    }

    // Save dynamic game data
    SaveGameState(out.get());
    // call "After Save" event callback
    run_on_event(GE_SAVE_GAME, RuntimeScriptValue().SetInt32(slotn));
}

int gameHasBeenRestored = 0;
int oldeip;

bool read_savedgame_description(const String &savedgame, String &description)
{
    SavegameDescription desc;
    HSaveError err = OpenSavegame(savedgame, desc, kSvgDesc_UserText);
    if (!err)
    {
        Debug::Printf(kDbgMsg_Error, "Unable to read save's description.\n%s", err->FullMessage().GetCStr());
        return false;
    }
    description = desc.UserText;
    return true;
}

std::unique_ptr<Bitmap> read_savedgame_screenshot(const String &savedgame)
{
    SavegameDescription desc;
    HSaveError err = OpenSavegame(savedgame, desc, kSvgDesc_UserImage);
    if (!err)
    {
        Debug::Printf(kDbgMsg_Error, "Unable to read save's screenshot.\n%s", err->FullMessage().GetCStr());
        return {};
    }
    if (desc.UserImage)
    {
        desc.UserImage.reset(PrepareSpriteForUse(desc.UserImage.release(), true /* force opaque */));
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
    PreReadGameData(g, src.InputStream.get(), src.DataVersion);
    if (!guid.IsEmpty())
        return guid.CompareNoCase(g.guid) == 0;
    return legacy_id == g.uniqueid;
}

HSaveError load_game(const String &path, int slotNumber, bool &data_overwritten)
{
    data_overwritten = false;
    gameHasBeenRestored++;

    oldeip = our_eip;
    our_eip = 2050;

    HSaveError err;
    SavegameSource src;
    SavegameDescription desc;
    err = OpenSavegame(path, src, desc, kSvgDesc_EnvInfo);

    // saved in incompatible enviroment
    if (!err)
        return err;
    // CHECKME: is this color depth test still essential? if yes, is there possible workaround?
    else if (desc.ColorDepth != game.GetColorDepth())
        return new SavegameError(kSvgErr_DifferentColorDepth, String::FromFormat("Running: %d-bit, saved in: %d-bit.", game.GetColorDepth(), desc.ColorDepth));

    // saved with different game file
    // if savegame is modern enough then test game GUIDs
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
    // if it's old then do the stupid old-style filename test
    // TODO: remove filename test after deprecating old saves
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

    // do the actual restore
    err = RestoreGameState(src.InputStream.get(), src.Version);
    data_overwritten = true;
    if (!err)
        return err;
    src.InputStream.reset();
    our_eip = oldeip;

    // ensure input state is reset
    ags_clear_input_state();
    // call "After Restore" event callback
    run_on_event(GE_RESTORE_GAME, RuntimeScriptValue().SetInt32(slotNumber));
    return HSaveError::None();
}

bool try_restore_save(int slot)
{
    return try_restore_save(get_save_game_path(slot), slot);
}

bool try_restore_save(const Common::String &path, int slot)
{
    bool data_overwritten;
    Debug::Printf(kDbgMsg_Info, "Restoring saved game '%s'", path.GetCStr());
    HSaveError err = load_game(path, slot, data_overwritten);
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

bool check_skip_cutscene_gamepad(int gbut)
{
    CutsceneSkipStyle skip = get_cutscene_skipstyle();
    if (gbut>0 && skip == eSkipSceneAnyKey)
    {
        start_skipping_cutscene();
        return true;
    }
    return false;
}

// Helper functions used by StartCutscene/EndCutscene, but also
// by SkipUntilCharacterStops
void initialize_skippable_cutscene() {
    // TODO: move related variable init here
}

void stop_fast_forwarding() {
    // when the skipping of a cutscene comes to an end, update things
    play.fast_forward = 0;
    setpal();

    // Restore actual volume of sounds
    for (int aa = 0; aa < TOTAL_AUDIO_CHANNELS; aa++)
    {
        auto* ch = AudioChans::GetChannelIfPlaying(aa);
        if (ch)
        {
            ch->set_mute(false);
        }
    }
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
    VpPoint vpt = play.ScreenToRoom(xxx, yyy);
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

    video_pause();
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
    // If auto lock option is set, lock mouse to the game window
    if (usetup.mouse_auto_lock && scsystem.windowed)
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
    video_resume();

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

void game_sprite_updated(int sprnum, bool deleted)
{
    // Notify draw system about dynamic sprite change
    notify_sprite_changed(sprnum, deleted);

    // GUI still have a special draw route, so cannot rely on object caches;
    // will have to do a per-GUI and per-control check.
    //
    // gui backgrounds
    for (auto &gui : guis)
    {
        if (gui.BgImage == sprnum)
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
        if ((slider.BgImage == sprnum) || (slider.HandleImage == sprnum))
        {
            slider.MarkChanged();
        }
    }
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
        { "Game::GetRunNextSettingForLoop^2",             API_FN_PAIR(Game_GetRunNextSettingForLoop) },
        { "Game::GetSaveSlotDescription^1",               API_FN_PAIR(Game_GetSaveSlotDescription) },
        { "Game::GetViewFrame^3",                         API_FN_PAIR(Game_GetViewFrame) },
        { "Game::InputBox^1",                             API_FN_PAIR(Game_InputBox) },
        { "Game::SetSaveGameDirectory^1",                 API_FN_PAIR(Game_SetSaveGameDirectory) },
        { "Game::ResetDoOnceOnly",                        API_FN_PAIR(Game_ResetDoOnceOnly) },
        { "Game::get_CharacterCount",                     API_FN_PAIR(Game_GetCharacterCount) },
        { "Game::get_DialogCount",                        API_FN_PAIR(Game_GetDialogCount) },
        { "Game::get_FileName",                           API_FN_PAIR(Game_GetFileName) },
        { "Game::get_FontCount",                          API_FN_PAIR(Game_GetFontCount) },
        { "Game::get_GUICount",                           API_FN_PAIR(Game_GetGUICount) },
        { "Game::get_IgnoreUserInputAfterTextTimeoutMs",  API_FN_PAIR(Game_GetIgnoreUserInputAfterTextTimeoutMs) },
        { "Game::set_IgnoreUserInputAfterTextTimeoutMs",  API_FN_PAIR(Game_SetIgnoreUserInputAfterTextTimeoutMs) },
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
        { "Game::geti_SpriteWidth",                       API_FN_PAIR(Game_GetSpriteWidth) },
        { "Game::geti_SpriteHeight",                      API_FN_PAIR(Game_GetSpriteHeight) },
        { "Game::get_TextReadingSpeed",                   API_FN_PAIR(Game_GetTextReadingSpeed) },
        { "Game::set_TextReadingSpeed",                   API_FN_PAIR(Game_SetTextReadingSpeed) },
        { "Game::get_TranslationFilename",                API_FN_PAIR(Game_GetTranslationFilename) },
        { "Game::get_ViewCount",                          API_FN_PAIR(Game_GetViewCount) },
        { "Game::get_AudioClipCount",                     API_FN_PAIR(Game_GetAudioClipCount) },
        { "Game::geti_AudioClips",                        API_FN_PAIR(Game_GetAudioClip) },
        { "Game::IsPluginLoaded",                         Sc_Game_IsPluginLoaded, pl_is_plugin_loaded },
        { "Game::ChangeSpeechVox",                        API_FN_PAIR(Game_ChangeSpeechVox) },
        { "Game::PlayVoiceClip",                          Sc_Game_PlayVoiceClip, PlayVoiceClip },
        { "Game::SimulateKeyPress",                       API_FN_PAIR(Game_SimulateKeyPress) },
        { "Game::get_BlockingWaitSkipped",                API_FN_PAIR(Game_BlockingWaitSkipped) },
        { "Game::get_SpeechVoxFilename",                  API_FN_PAIR(Game_GetSpeechVoxFilename) },
        { "Game::get_Camera",                             API_FN_PAIR(Game_GetCamera) },
        { "Game::get_CameraCount",                        API_FN_PAIR(Game_GetCameraCount) },
        { "Game::geti_Cameras",                           API_FN_PAIR(Game_GetAnyCamera) },
    };

    ccAddExternalFunctions(game_api);
}

void RegisterStaticObjects()
{
    ccAddExternalScriptObject("game", &play, &GameStaticManager);
	ccAddExternalScriptObject("mouse", &scmouse, &scmouse);
	ccAddExternalScriptObject("palette", &palette[0], &GlobalStaticManager);
}
