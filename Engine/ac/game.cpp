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
#include "ac/common.h"
#include "ac/view.h"
#include "ac/audiochannel.h"
#include "ac/button.h"
#include "ac/character.h"
#include "ac/charactercache.h"
#include "ac/dialogtopic.h"
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
#include "ac/objectcache.h"
#include "ac/overlay.h"
#include "ac/path_helper.h"
#include "ac/sys_events.h"
#include "ac/richgamemedia.h"
#include "ac/roomstatus.h"
#include "ac/spritecache.h"
#include "ac/string.h"
#include "ac/translation.h"
#include "ac/dynobj/all_dynamicclasses.h"
#include "ac/dynobj/all_scriptclasses.h"
#include "ac/dynobj/scriptcamera.h"
#include "debug/debug_log.h"
#include "debug/out.h"
#include "device/mousew32.h"
#include "font/fonts.h"
#include "game/savegame.h"
#include "gfx/bitmap.h"
#include "gfx/graphicsdriver.h"
#include "gui/guibutton.h"
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

extern ScriptAudioChannel scrAudioChannel[MAX_GAME_CHANNELS];
extern SpeechLipSyncLine *splipsync;
extern int numLipLines, curLipLine, curLipLinePhoneme;

extern DialogTopic *dialog;

extern int obj_lowest_yp, char_lowest_yp;

extern RGB palette[256];
extern IGraphicsDriver *gfxDriver;

//=============================================================================
GameState play;
GameSetup usetup;
GameSetupStruct game;
RoomStatus troom;    // used for non-saveable rooms, eg. intro
RoomObject*objs;
RoomStatus*croom=nullptr;
RoomStruct thisroom;

volatile int switching_away_from_game = 0;
volatile bool switched_away = false;
volatile bool game_update_suspend = false;
volatile char want_exit = 0, abort_engine = 0;
GameDataVersion loaded_game_file_version = kGameVersion_Undefined;
Version game_compiled_version;
int frames_per_second=40;
int displayed_room=-10,starting_room = -1;
int in_new_room=0, new_room_was = 0;  // 1 in new room, 2 first time in new room, 3 loading saved game
int new_room_pos=0;
int new_room_x = SCR_NO_VALUE, new_room_y = SCR_NO_VALUE;
int new_room_loop = SCR_NO_VALUE;
bool new_room_placeonwalkable = false;

// initially size 1, this will be increased by the initFile function
SpriteCache spriteset(game.SpriteInfos);
int proper_exit=0,our_eip=0;

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
// TODO: IMPORTANT!!
// we cannot simply replace these arrays with vectors, or other C++ containers,
// until we implement safe management of such containers in script exports
// system. Noteably we would need an alternate to StaticArray class to track
// access to their elements.
ScriptObject scrObj[MAX_ROOM_OBJECTS];
ScriptGUI    *scrGui = nullptr;
ScriptHotspot scrHotspot[MAX_ROOM_HOTSPOTS];
ScriptRegion scrRegion[MAX_ROOM_REGIONS];
ScriptInvItem scrInv[MAX_INV];
ScriptDialog *scrDialog;

std::vector<ViewStruct> views;

CharacterCache *charcache = nullptr;
ObjectCache objcache[MAX_ROOM_OBJECTS];

MoveList *mls = nullptr;

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

    Debug::Printf("Game.SetAudioTypeVolume: type: %d, volume: %d, change: %d", audioType, volume, changeType);
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
    debug_set_console(on);
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
    String restartGamePath = Path::ConcatPaths(saveGameDirectory, get_save_game_filename(RESTART_POINT_SAVE_GAME_NUMBER));
    Stream *restartGameFile = File::OpenFileRead(restartGamePath);
    if (restartGameFile != nullptr)
    {
        long fileSize = restartGameFile->GetLength();
        char *mbuffer = (char*)malloc(fileSize);
        restartGameFile->Read(mbuffer, fileSize);
        delete restartGameFile;

        restartGamePath = Path::ConcatPaths(newSaveGameDir, get_save_game_filename(RESTART_POINT_SAVE_GAME_NUMBER));
        restartGameFile = File::CreateFile(restartGamePath);
        restartGameFile->Write(mbuffer, fileSize);
        delete restartGameFile;
        free(mbuffer);
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
    if (thisroom.Options.SaveLoadDisabled == 1) {
        DisplayMessage (983);
        return;
    }
    if (inside_script) {
        curscript->queue_action(ePSARestoreGameDialog, 0, "RestoreGameDialog");
        return;
    }
    setup_for_dialog();
    int toload=loadgamedialog();
    restore_after_dialog();
    if (toload>=0) {
        try_restore_save(toload);
    }
}

void save_game_dialog() {
    if (thisroom.Options.SaveLoadDisabled == 1) {
        DisplayMessage (983);
        return;
    }
    if (inside_script) {
        curscript->queue_action(ePSASaveGameDialog, 0, "SaveGameDialog");
        return;
    }
    setup_for_dialog();
    int toload=savegamedialog();
    restore_after_dialog();
    if (toload>=0)
        save_game(toload, get_gui_dialog_buffer());
}

void free_do_once_tokens()
{
    play.do_once_tokens.resize(0);
}


// Free all the memory associated with the game
// TODO: call this when exiting the game (currently only called in RunAGSGame)
void unload_game_file()
{
    close_translation();

    play.FreeViewportsAndCameras();

    characterScriptObjNames.clear();
    free(charextra);
    free(mls);

    dispose_game_drawdata();

    if ((gameinst != nullptr) && (gameinst->pc != 0))
    {
        quit("Error: unload_game called while script still running");
    }
    else
    {
        delete gameinstFork;
        delete gameinst;
        gameinstFork = nullptr;
        gameinst = nullptr;
    }

    gamescript.reset();

    if ((dialogScriptsInst != nullptr) && (dialogScriptsInst->pc != 0))
    {
        quit("Error: unload_game called while dialog script still running");
    }
    else if (dialogScriptsInst != nullptr)
    {
        delete dialogScriptsInst;
        dialogScriptsInst = nullptr;
    }

    dialogScriptsScript.reset();

    for (size_t i = 0; i < numScriptModules; ++i)
    {
        delete moduleInstFork[i];
        delete moduleInst[i];
        scriptModules[i].reset();
    }
    moduleInstFork.resize(0);
    moduleInst.resize(0);
    scriptModules.resize(0);
    repExecAlways.moduleHasFunction.resize(0);
    lateRepExecAlways.moduleHasFunction.resize(0);
    getDialogOptionsDimensionsFunc.moduleHasFunction.resize(0);
    renderDialogOptionsFunc.moduleHasFunction.resize(0);
    getDialogOptionUnderCursorFunc.moduleHasFunction.resize(0);
    runDialogOptionMouseClickHandlerFunc.moduleHasFunction.resize(0);
    runDialogOptionKeyPressHandlerFunc.moduleHasFunction.resize(0);
    runDialogOptionTextInputHandlerFunc.moduleHasFunction.resize(0);
    runDialogOptionRepExecFunc.moduleHasFunction.resize(0);
    numScriptModules = 0;

    views.clear();

    free(charcache);
    charcache = nullptr;

    if (splipsync != nullptr)
    {
        for (int i = 0; i < numLipLines; ++i)
        {
            free(splipsync[i].endtimeoffs);
            free(splipsync[i].frame);
        }
        free(splipsync);
        splipsync = nullptr;
        numLipLines = 0;
        curLipLine = -1;
    }

    for (int i = 0; i < game.numdialog; ++i)
    {
        if (dialog[i].optionscripts != nullptr)
            free(dialog[i].optionscripts);
        dialog[i].optionscripts = nullptr;
    }
    free(dialog);
    dialog = nullptr;
    delete[] scrDialog;
    scrDialog = nullptr;

    guiScriptObjNames.clear();
    guis.clear();
    free(scrGui);

    free_all_fonts();

    pl_stop_plugins();
    ccRemoveAllSymbols();
    ccUnregisterAllObjects();

    free_do_once_tokens();
    free(play.gui_draw_order);

    resetRoomStatuses();

    // free game struct last because it contains object counts
    game.Free();
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

int Game_GetLoopCountForView(int viewNumber) {
    if ((viewNumber < 1) || (viewNumber > game.numviews))
        quit("!GetGameParameter: invalid view specified");

    return views[viewNumber - 1].numLoops;
}

int Game_GetRunNextSettingForLoop(int viewNumber, int loopNumber) {
    if ((viewNumber < 1) || (viewNumber > game.numviews))
        quit("!GetGameParameter: invalid view specified");
    if ((loopNumber < 0) || (loopNumber >= views[viewNumber - 1].numLoops))
        quit("!GetGameParameter: invalid loop specified");

    return (views[viewNumber - 1].loops[loopNumber].RunNextLoop()) ? 1 : 0;
}

int Game_GetFrameCountForLoop(int viewNumber, int loopNumber) {
    if ((viewNumber < 1) || (viewNumber > game.numviews))
        quit("!GetGameParameter: invalid view specified");
    if ((loopNumber < 0) || (loopNumber >= views[viewNumber - 1].numLoops))
        quit("!GetGameParameter: invalid loop specified");

    return views[viewNumber - 1].loops[loopNumber].numFrames;
}

ScriptViewFrame* Game_GetViewFrame(int viewNumber, int loopNumber, int frame) {
    if ((viewNumber < 1) || (viewNumber > game.numviews))
        quit("!GetGameParameter: invalid view specified");
    if ((loopNumber < 0) || (loopNumber >= views[viewNumber - 1].numLoops))
        quit("!GetGameParameter: invalid loop specified");
    if ((frame < 0) || (frame >= views[viewNumber - 1].loops[loopNumber].numFrames))
        quit("!GetGameParameter: invalid frame specified");

    ScriptViewFrame *sdt = new ScriptViewFrame(viewNumber - 1, loopNumber, frame);
    ccRegisterManagedObject(sdt, sdt);
    return sdt;
}

int Game_DoOnceOnly(const char *token)
{
    for (int i = 0; i < (int)play.do_once_tokens.size(); i++)
    {
        if (play.do_once_tokens[i] == token)
        {
            return 0;
        }
    }
    play.do_once_tokens.push_back(token);
    return 1;
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

const char* Game_GetGlobalMessages(int index) {
    if ((index < 500) || (index >= MAXGLOBALMES + 500)) {
        return nullptr;
    }
    char buffer[STD_BUFFER_SIZE];
    buffer[0] = 0;
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
    GetTranslationName(buffer);
    return CreateNewScriptString(buffer);
}

int Game_ChangeTranslation(const char *newFilename)
{
    if ((newFilename == nullptr) || (newFilename[0] == 0))
    { // switch back to default translation
        close_translation();
        usetup.translation = "";
        return 1;
    }

    String oldTransFileName = get_translation_name();
    if (!init_translation(newFilename, oldTransFileName))
        return 0; // failed, kept previous translation

    usetup.translation = newFilename;
    return 1;
}

const char* Game_GetSpeechVoxFilename()
{
    return CreateNewScriptString(get_voicepak_name());
}

bool Game_ChangeSpeechVox(const char *newFilename)
{
    if (!init_voicepak(newFilename))
    {
        // if failed (and was not default)- fallback to default
        if (strlen(newFilename) > 0)
            init_voicepak();
        return false;
    }
    return true;
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

// On Windows we could just use IIDFromString but this is platform-independant
void convert_guid_from_text_to_binary(const char *guidText, unsigned char *buffer)
{
    guidText++; // skip {
    for (int bytesDone = 0; bytesDone < 16; bytesDone++)
    {
        if (*guidText == '-')
            guidText++;

        char tempString[3];
        tempString[0] = guidText[0];
        tempString[1] = guidText[1];
        tempString[2] = 0;
        int thisByte = 0;
        sscanf(tempString, "%X", &thisByte);

        buffer[bytesDone] = thisByte;
        guidText += 2;
    }

    // Swap bytes to give correct GUID order
    unsigned char temp;
    temp = buffer[0]; buffer[0] = buffer[3]; buffer[3] = temp;
    temp = buffer[1]; buffer[1] = buffer[2]; buffer[2] = temp;
    temp = buffer[4]; buffer[4] = buffer[5]; buffer[5] = temp;
    temp = buffer[6]; buffer[6] = buffer[7]; buffer[7] = temp;
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

long write_screen_shot_for_vista(Stream *out, Bitmap *screenshot)
{
    long fileSize = 0;
    String tempFileName = String::FromFormat("%s""_tmpscht.bmp", saveGameDirectory.GetCStr());

    screenshot->SaveToFile(tempFileName, palette);

    update_polled_stuff_if_runtime();

    if (File::IsFile(tempFileName))
    {
        fileSize = File::GetFileSize(tempFileName);
        char *buffer = (char*)malloc(fileSize);

        Stream *temp_in = Common::File::OpenFileRead(tempFileName);
        temp_in->Read(buffer, fileSize);
        delete temp_in;
        File::DeleteFile(tempFileName);

        out->Write(buffer, fileSize);
        free(buffer);
    }
    return fileSize;
}

Bitmap *create_savegame_screenshot()
{
    int usewid = data_to_game_coord(play.screenshot_width);
    int usehit = data_to_game_coord(play.screenshot_height);
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
        strcpy(curscript->postScriptSaveSlotDescription[curscript->queue_action(ePSASaveGame, slotn, "SaveGameSlot")], descript);
        return;
    }

    if (platform->GetDiskFreeSpaceMB() < 2) {
        Display("ERROR: There is not enough disk space free to save the game. Clear some disk space and try again.");
        return;
    }

    VALIDATE_STRING(descript);
    String nametouse = get_save_game_path(slotn);
    UBitmap screenShot;
    if (game.options[OPT_SAVESCREENSHOT] != 0)
        screenShot.reset(create_savegame_screenshot());

    Engine::UStream out(StartSavegame(nametouse, descript, screenShot.get()));
    if (out == nullptr)
    {
        Display("ERROR: Unable to open savegame file for writing!");
        return;
    }

    update_polled_stuff_if_runtime();

    // Actual dynamic game data is saved here
    SaveGameState(out.get());

    if (screenShot != nullptr)
    {
        int screenShotOffset = out->GetPosition() - sizeof(RICH_GAME_MEDIA_HEADER);
        int screenShotSize = write_screen_shot_for_vista(out.get(), screenShot.get());

        update_polled_stuff_if_runtime();

        out.reset(Common::File::OpenFile(nametouse, Common::kFile_Open, Common::kFile_ReadWrite));
        out->Seek(12, kSeekBegin);
        out->WriteInt32(screenShotOffset);
        out->Seek(4);
        out->WriteInt32(screenShotSize);
    }
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

bool read_savedgame_screenshot(const String &savedgame, int &want_shot)
{
    want_shot = 0;

    SavegameDescription desc;
    HSaveError err = OpenSavegame(savedgame, desc, kSvgDesc_UserImage);
    if (!err)
    {
        Debug::Printf(kDbgMsg_Error, "Unable to read save's screenshot.\n%s", err->FullMessage().GetCStr());
        return false;
    }

    if (desc.UserImage.get())
    {
        int slot = spriteset.GetFreeIndex();
        if (slot > 0)
        {
            // add it into the sprite set
            add_dynamic_sprite(slot, PrepareSpriteForUse(desc.UserImage.release(), false));
            want_shot = slot;
        }
    }
    return true;
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
            if (Common::File::TestReadFile(gamefile))
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
        if (Common::File::TestReadFile(gamefile))
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
        (mbut == MouseRight && skip == eSkipSceneEscOrRMB))
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

// Called whenever game looses input focus
void display_switch_out()
{
    switched_away = true;
    ags_clear_input_state();
    // Always unlock mouse when switching out from the game
    Mouse::UnlockFromWindow();
}

// Called when game looses input focus and must pause until focus is returned
void display_switch_out_suspend()
{
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

    // restore the callbacks
    SetMultitasking(0);

    switching_away_from_game--;
}

// Called whenever game gets input focus
void display_switch_in()
{
    ags_clear_input_state();
    // If auto lock option is set, lock mouse to the game window
    if (usetup.mouse_auto_lock && scsystem.windowed)
        Mouse::TryLockToWindow();
    switched_away = false;
}

// Called when game gets input focus and must resume after pause
void display_switch_in_resume()
{
    display_switch_in();

    // Resume all the sounds
    for (int i = 0; i < TOTAL_AUDIO_CHANNELS; i++) {
        auto* ch = AudioChans::GetChannelIfPlaying(i);
        if (ch) {
            ch->resume();
        }
    }
    video_resume();

    // clear the screen if necessary
    if (gfxDriver && gfxDriver->UsesMemoryBackBuffer())
        gfxDriver->ClearRectangle(0, 0, game.GetGameRes().Width - 1, game.GetGameRes().Height - 1, nullptr);

    // TODO: find out if anything has to be done here for SDL backend

    platform->ResumeApplication();
    game_update_suspend = false;
}

void replace_tokens(const char*srcmes,char*destm, int maxlen) {
    int indxdest=0,indxsrc=0;
    const char*srcp;
    char *destp;
    while (srcmes[indxsrc]!=0) {
        srcp=&srcmes[indxsrc];
        destp=&destm[indxdest];
        if ((strncmp(srcp,"@IN",3)==0) | (strncmp(srcp,"@GI",3)==0)) {
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
            strcpy(destp,tval);
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
    if (game.messages[msnum-500] == nullptr)
        return "";
    return get_translation(game.messages[msnum-500]);
}

void get_message_text (int msnum, char *buffer, char giveErr) {
    int maxlen = 9999;
    if (!giveErr)
        maxlen = MAX_MAXSTRLEN;

    if (msnum>=500) {

        if ((msnum >= MAXGLOBALMES + 500) || (game.messages[msnum-500]==nullptr)) {
            if (giveErr)
                quit("!DisplayGlobalMessage: message does not exist");
            buffer[0] = 0;
            return;
        }
        buffer[0] = 0;
        replace_tokens(get_translation(game.messages[msnum-500]), buffer, maxlen);
        return;
    }
    else if (msnum < 0 || (size_t)msnum >= thisroom.MessageCount) {
        if (giveErr)
            quit("!DisplayMessage: Invalid message number to display");
        buffer[0] = 0;
        return;
    }

    buffer[0]=0;
    replace_tokens(get_translation(thisroom.Messages[msnum].GetCStr()), buffer, maxlen);
}

bool unserialize_audio_script_object(int index, const char *objectType, Stream *in, size_t data_sz)
{
    if (strcmp(objectType, "AudioChannel") == 0)
    {
        ccDynamicAudio.Unserialize(index, in, data_sz);
    }
    else if (strcmp(objectType, "AudioClip") == 0)
    {
        ccDynamicAudioClip.Unserialize(index, in, data_sz);
    }
    else
    {
        return false;
    }
    return true;
}

void game_sprite_updated(int sprnum)
{
    // Check if this sprite is assigned to any game object, and update them if necessary
    // room objects cache
    if (croom != nullptr)
    {
        for (size_t i = 0; i < (size_t)croom->numobj; ++i)
        {
            if (objs[i].num == sprnum)
                objcache[i].sppic = -1;
        }
    }
    // character cache
    for (size_t i = 0; i < (size_t)game.numcharacters; ++i)
    {
        if (charcache[i].sppic == sprnum)
            charcache[i].sppic = -1;
    }
    // gui backgrounds
    for (size_t i = 0; i < (size_t)game.numgui; ++i)
    {
        if (guis[i].BgImage == sprnum)
        {
            guis[i].MarkChanged();
        }
    }
    // gui buttons
    for (size_t i = 0; i < (size_t)numguibuts; ++i)
    {
        if (guibuts[i].CurrentImage == sprnum)
        {
            guibuts[i].NotifyParentChanged();
        }
    }
}

void game_sprite_deleted(int sprnum)
{
    // Check if this sprite is assigned to any game object, and update them if necessary
    // room objects and their cache
    if (croom != nullptr)
    {
        for (size_t i = 0; i < (size_t)croom->numobj; ++i)
        {
            if (objs[i].num == sprnum)
            {
                objs[i].num = 0;
                objcache[i].sppic = -1;
            }
        }
    }
    // character cache
    for (size_t i = 0; i < (size_t)game.numcharacters; ++i)
    {
        if (charcache[i].sppic == sprnum)
            charcache[i].sppic = -1;
    }
    // gui backgrounds
    for (size_t i = 0; i < (size_t)game.numgui; ++i)
    {
        if (guis[i].BgImage == sprnum)
        {
            guis[i].BgImage = 0;
            guis[i].MarkChanged();
        }
    }
    // gui buttons
    for (size_t i = 0; i < (size_t)numguibuts; ++i)
    {
        if (guibuts[i].Image == sprnum)
            guibuts[i].Image = 0;
        if (guibuts[i].MouseOverImage == sprnum)
            guibuts[i].MouseOverImage = 0;
        if (guibuts[i].PushedImage == sprnum)
            guibuts[i].PushedImage = 0;

        if (guibuts[i].CurrentImage == sprnum)
        {
            guibuts[i].CurrentImage = 0;
            guibuts[i].NotifyParentChanged();
        }
    }
    // views
    for (size_t v = 0; v < (size_t)game.numviews; ++v)
    {
        for (size_t l = 0; l < (size_t)views[v].numLoops; ++l)
        {
            for (size_t f = 0; f < (size_t)views[v].loops[l].numFrames; ++f)
            {
                if (views[v].loops[l].frames[f].pic == sprnum)
                    views[v].loops[l].frames[f].pic = 0;
            }
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
    API_VARGET_INT(game.audioClips.size());
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
    ccAddExternalStaticFunction("Game::IsAudioPlaying^1",                       Sc_Game_IsAudioPlaying);
    ccAddExternalStaticFunction("Game::SetAudioTypeSpeechVolumeDrop^2",         Sc_Game_SetAudioTypeSpeechVolumeDrop);
    ccAddExternalStaticFunction("Game::SetAudioTypeVolume^3",                   Sc_Game_SetAudioTypeVolume);
    ccAddExternalStaticFunction("Game::StopAudio^1",                            Sc_Game_StopAudio);
    ccAddExternalStaticFunction("Game::ChangeTranslation^1",                    Sc_Game_ChangeTranslation);
    ccAddExternalStaticFunction("Game::DoOnceOnly^1",                           Sc_Game_DoOnceOnly);
    ccAddExternalStaticFunction("Game::GetColorFromRGB^3",                      Sc_Game_GetColorFromRGB);
    ccAddExternalStaticFunction("Game::GetFrameCountForLoop^2",                 Sc_Game_GetFrameCountForLoop);
    ccAddExternalStaticFunction("Game::GetLocationName^2",                      Sc_Game_GetLocationName);
    ccAddExternalStaticFunction("Game::GetLoopCountForView^1",                  Sc_Game_GetLoopCountForView);
    ccAddExternalStaticFunction("Game::GetMODPattern^0",                        Sc_Game_GetMODPattern);
    ccAddExternalStaticFunction("Game::GetRunNextSettingForLoop^2",             Sc_Game_GetRunNextSettingForLoop);
    ccAddExternalStaticFunction("Game::GetSaveSlotDescription^1",               Sc_Game_GetSaveSlotDescription);
    ccAddExternalStaticFunction("Game::GetViewFrame^3",                         Sc_Game_GetViewFrame);
    ccAddExternalStaticFunction("Game::InputBox^1",                             Sc_Game_InputBox);
    ccAddExternalStaticFunction("Game::SetSaveGameDirectory^1",                 Sc_Game_SetSaveGameDirectory);
    ccAddExternalStaticFunction("Game::StopSound^1",                            Sc_StopAllSounds);
    ccAddExternalStaticFunction("Game::get_CharacterCount",                     Sc_Game_GetCharacterCount);
    ccAddExternalStaticFunction("Game::get_DialogCount",                        Sc_Game_GetDialogCount);
    ccAddExternalStaticFunction("Game::get_FileName",                           Sc_Game_GetFileName);
    ccAddExternalStaticFunction("Game::get_FontCount",                          Sc_Game_GetFontCount);
    ccAddExternalStaticFunction("Game::geti_GlobalMessages",                    Sc_Game_GetGlobalMessages);
    ccAddExternalStaticFunction("Game::geti_GlobalStrings",                     Sc_Game_GetGlobalStrings);
    ccAddExternalStaticFunction("Game::seti_GlobalStrings",                     Sc_SetGlobalString);
    ccAddExternalStaticFunction("Game::get_GUICount",                           Sc_Game_GetGUICount);
    ccAddExternalStaticFunction("Game::get_IgnoreUserInputAfterTextTimeoutMs",  Sc_Game_GetIgnoreUserInputAfterTextTimeoutMs);
    ccAddExternalStaticFunction("Game::set_IgnoreUserInputAfterTextTimeoutMs",  Sc_Game_SetIgnoreUserInputAfterTextTimeoutMs);
    ccAddExternalStaticFunction("Game::get_InSkippableCutscene",                Sc_Game_GetInSkippableCutscene);
    ccAddExternalStaticFunction("Game::get_InventoryItemCount",                 Sc_Game_GetInventoryItemCount);
    ccAddExternalStaticFunction("Game::get_MinimumTextDisplayTimeMs",           Sc_Game_GetMinimumTextDisplayTimeMs);
    ccAddExternalStaticFunction("Game::set_MinimumTextDisplayTimeMs",           Sc_Game_SetMinimumTextDisplayTimeMs);
    ccAddExternalStaticFunction("Game::get_MouseCursorCount",                   Sc_Game_GetMouseCursorCount);
    ccAddExternalStaticFunction("Game::get_Name",                               Sc_Game_GetName);
    ccAddExternalStaticFunction("Game::set_Name",                               Sc_Game_SetName);
    ccAddExternalStaticFunction("Game::get_NormalFont",                         Sc_Game_GetNormalFont);
    ccAddExternalStaticFunction("Game::set_NormalFont",                         Sc_SetNormalFont);
    ccAddExternalStaticFunction("Game::get_SkippingCutscene",                   Sc_Game_GetSkippingCutscene);
    ccAddExternalStaticFunction("Game::get_SpeechFont",                         Sc_Game_GetSpeechFont);
    ccAddExternalStaticFunction("Game::set_SpeechFont",                         Sc_SetSpeechFont);
    ccAddExternalStaticFunction("Game::geti_SpriteWidth",                       Sc_Game_GetSpriteWidth);
    ccAddExternalStaticFunction("Game::geti_SpriteHeight",                      Sc_Game_GetSpriteHeight);
    ccAddExternalStaticFunction("Game::get_TextReadingSpeed",                   Sc_Game_GetTextReadingSpeed);
    ccAddExternalStaticFunction("Game::set_TextReadingSpeed",                   Sc_Game_SetTextReadingSpeed);
    ccAddExternalStaticFunction("Game::get_TranslationFilename",                Sc_Game_GetTranslationFilename);
    ccAddExternalStaticFunction("Game::get_UseNativeCoordinates",               Sc_Game_GetUseNativeCoordinates);
    ccAddExternalStaticFunction("Game::get_ViewCount",                          Sc_Game_GetViewCount);
    ccAddExternalStaticFunction("Game::get_AudioClipCount",                     Sc_Game_GetAudioClipCount);
    ccAddExternalStaticFunction("Game::geti_AudioClips",                        Sc_Game_GetAudioClip);
    ccAddExternalStaticFunction("Game::IsPluginLoaded",                         Sc_Game_IsPluginLoaded);
    ccAddExternalStaticFunction("Game::ChangeSpeechVox",                        Sc_Game_ChangeSpeechVox);
    ccAddExternalStaticFunction("Game::PlayVoiceClip",                          Sc_Game_PlayVoiceClip);
    ccAddExternalStaticFunction("Game::SimulateKeyPress",                       Sc_Game_SimulateKeyPress);
    ccAddExternalStaticFunction("Game::get_BlockingWaitSkipped",                Sc_Game_BlockingWaitSkipped);
    ccAddExternalStaticFunction("Game::get_SpeechVoxFilename",                  Sc_Game_GetSpeechVoxFilename);

    ccAddExternalStaticFunction("Game::get_Camera",                             Sc_Game_GetCamera);
    ccAddExternalStaticFunction("Game::get_CameraCount",                        Sc_Game_GetCameraCount);
    ccAddExternalStaticFunction("Game::geti_Cameras",                           Sc_Game_GetAnyCamera);

    /* ----------------------- Registering unsafe exports for plugins -----------------------*/

    ccAddExternalFunctionForPlugin("Game::IsAudioPlaying^1",                       (void*)Game_IsAudioPlaying);
    ccAddExternalFunctionForPlugin("Game::SetAudioTypeSpeechVolumeDrop^2",         (void*)Game_SetAudioTypeSpeechVolumeDrop);
    ccAddExternalFunctionForPlugin("Game::SetAudioTypeVolume^3",                   (void*)Game_SetAudioTypeVolume);
    ccAddExternalFunctionForPlugin("Game::StopAudio^1",                            (void*)Game_StopAudio);
    ccAddExternalFunctionForPlugin("Game::ChangeTranslation^1",                    (void*)Game_ChangeTranslation);
    ccAddExternalFunctionForPlugin("Game::DoOnceOnly^1",                           (void*)Game_DoOnceOnly);
    ccAddExternalFunctionForPlugin("Game::GetColorFromRGB^3",                      (void*)Game_GetColorFromRGB);
    ccAddExternalFunctionForPlugin("Game::GetFrameCountForLoop^2",                 (void*)Game_GetFrameCountForLoop);
    ccAddExternalFunctionForPlugin("Game::GetLocationName^2",                      (void*)Game_GetLocationName);
    ccAddExternalFunctionForPlugin("Game::GetLoopCountForView^1",                  (void*)Game_GetLoopCountForView);
    ccAddExternalFunctionForPlugin("Game::GetMODPattern^0",                        (void*)Game_GetMODPattern);
    ccAddExternalFunctionForPlugin("Game::GetRunNextSettingForLoop^2",             (void*)Game_GetRunNextSettingForLoop);
    ccAddExternalFunctionForPlugin("Game::GetSaveSlotDescription^1",               (void*)Game_GetSaveSlotDescription);
    ccAddExternalFunctionForPlugin("Game::GetViewFrame^3",                         (void*)Game_GetViewFrame);
    ccAddExternalFunctionForPlugin("Game::InputBox^1",                             (void*)Game_InputBox);
    ccAddExternalFunctionForPlugin("Game::SetSaveGameDirectory^1",                 (void*)Game_SetSaveGameDirectory);
    ccAddExternalFunctionForPlugin("Game::StopSound^1",                            (void*)StopAllSounds);
    ccAddExternalFunctionForPlugin("Game::get_CharacterCount",                     (void*)Game_GetCharacterCount);
    ccAddExternalFunctionForPlugin("Game::get_DialogCount",                        (void*)Game_GetDialogCount);
    ccAddExternalFunctionForPlugin("Game::get_FileName",                           (void*)Game_GetFileName);
    ccAddExternalFunctionForPlugin("Game::get_FontCount",                          (void*)Game_GetFontCount);
    ccAddExternalFunctionForPlugin("Game::geti_GlobalMessages",                    (void*)Game_GetGlobalMessages);
    ccAddExternalFunctionForPlugin("Game::geti_GlobalStrings",                     (void*)Game_GetGlobalStrings);
    ccAddExternalFunctionForPlugin("Game::seti_GlobalStrings",                     (void*)SetGlobalString);
    ccAddExternalFunctionForPlugin("Game::get_GUICount",                           (void*)Game_GetGUICount);
    ccAddExternalFunctionForPlugin("Game::get_IgnoreUserInputAfterTextTimeoutMs",  (void*)Game_GetIgnoreUserInputAfterTextTimeoutMs);
    ccAddExternalFunctionForPlugin("Game::set_IgnoreUserInputAfterTextTimeoutMs",  (void*)Game_SetIgnoreUserInputAfterTextTimeoutMs);
    ccAddExternalFunctionForPlugin("Game::get_InSkippableCutscene",                (void*)Game_GetInSkippableCutscene);
    ccAddExternalFunctionForPlugin("Game::get_InventoryItemCount",                 (void*)Game_GetInventoryItemCount);
    ccAddExternalFunctionForPlugin("Game::get_MinimumTextDisplayTimeMs",           (void*)Game_GetMinimumTextDisplayTimeMs);
    ccAddExternalFunctionForPlugin("Game::set_MinimumTextDisplayTimeMs",           (void*)Game_SetMinimumTextDisplayTimeMs);
    ccAddExternalFunctionForPlugin("Game::get_MouseCursorCount",                   (void*)Game_GetMouseCursorCount);
    ccAddExternalFunctionForPlugin("Game::get_Name",                               (void*)Game_GetName);
    ccAddExternalFunctionForPlugin("Game::set_Name",                               (void*)Game_SetName);
    ccAddExternalFunctionForPlugin("Game::get_NormalFont",                         (void*)Game_GetNormalFont);
    ccAddExternalFunctionForPlugin("Game::set_NormalFont",                         (void*)SetNormalFont);
    ccAddExternalFunctionForPlugin("Game::get_SkippingCutscene",                   (void*)Game_GetSkippingCutscene);
    ccAddExternalFunctionForPlugin("Game::get_SpeechFont",                         (void*)Game_GetSpeechFont);
    ccAddExternalFunctionForPlugin("Game::set_SpeechFont",                         (void*)SetSpeechFont);
    ccAddExternalFunctionForPlugin("Game::geti_SpriteWidth",                       (void*)Game_GetSpriteWidth);
    ccAddExternalFunctionForPlugin("Game::geti_SpriteHeight",                      (void*)Game_GetSpriteHeight);
    ccAddExternalFunctionForPlugin("Game::get_TextReadingSpeed",                   (void*)Game_GetTextReadingSpeed);
    ccAddExternalFunctionForPlugin("Game::set_TextReadingSpeed",                   (void*)Game_SetTextReadingSpeed);
    ccAddExternalFunctionForPlugin("Game::get_TranslationFilename",                (void*)Game_GetTranslationFilename);
    ccAddExternalFunctionForPlugin("Game::get_UseNativeCoordinates",               (void*)Game_GetUseNativeCoordinates);
    ccAddExternalFunctionForPlugin("Game::get_ViewCount",                          (void*)Game_GetViewCount);
    ccAddExternalFunctionForPlugin("Game::PlayVoiceClip",                          (void*)PlayVoiceClip);
}

void RegisterStaticObjects()
{
    ccAddExternalStaticObject("game",&play, &GameStaticManager);
	ccAddExternalStaticObject("gs_globals",&play.globalvars[0], &GlobalStaticManager);
	ccAddExternalStaticObject("mouse",&scmouse, &GlobalStaticManager);
	ccAddExternalStaticObject("palette",&palette[0], &GlobalStaticManager);
	ccAddExternalStaticObject("system",&scsystem, &GlobalStaticManager);
	ccAddExternalStaticObject("savegameindex",&play.filenumbers[0], &GlobalStaticManager);
}
