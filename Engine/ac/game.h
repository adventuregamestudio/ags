//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// AGS Runtime header
//
//=============================================================================

#ifndef __AGS_EE_AC__GAME_H
#define __AGS_EE_AC__GAME_H

#include "ac/dynobj/scriptviewframe.h"
#include "main/game_file.h"
#include "util/string.h"

// Forward declaration
namespace AGS { namespace Common { class Bitmap; class Stream; } }
using namespace AGS; // FIXME later

#define RAGMODE_PRESERVEGLOBALINT 1
#define RAGMODE_LOADNOW 0x8000000  // just to make sure it's non-zero

// Game parameter constants for backward-compatibility functions
#define GP_SPRITEWIDTH   1
#define GP_SPRITEHEIGHT  2
#define GP_NUMLOOPS      3
#define GP_NUMFRAMES     4
#define GP_ISRUNNEXTLOOP 5
#define GP_FRAMESPEED    6
#define GP_FRAMEIMAGE    7
#define GP_FRAMESOUND    8
#define GP_NUMGUIS       9
#define GP_NUMOBJECTS    10
#define GP_NUMCHARACTERS 11
#define GP_NUMINVITEMS   12
#define GP_ISFRAMEFLIPPED 13

enum CutsceneSkipStyle
{
    kSkipSceneUndefined = 0,
    eSkipSceneEscOnly = 1,
    eSkipSceneAnyKey = 2,
    eSkipSceneMouse = 3,
    eSkipSceneKeyMouse = 4,
    eSkipSceneEscOrRMB = 5,
    eSkipSceneScriptOnly = 6
};

//=============================================================================
// Audio
//=============================================================================
#define VOL_CHANGEEXISTING   1678
#define VOL_SETFUTUREDEFAULT 1679
#define VOL_BOTH             1680

void Game_StopAudio(int audioType);
int  Game_IsAudioPlaying(int audioType);
void Game_SetAudioTypeSpeechVolumeDrop(int audioType, int volumeDrop);
void Game_SetAudioTypeVolume(int audioType, int volume, int changeType);

int Game_GetMODPattern();

//=============================================================================
// ---
//=============================================================================
int Game_GetDialogCount();

// Sets a default save directory, based on platform driver settings and user config
void SetDefaultSaveDirectory();
// Sets a new save directory within the save parent; copies "restart" slot if available
int Game_SetSaveGameDirectory(const char *newFolder);
const char* Game_GetSaveSlotDescription(int slnum);

const char* Game_GetGlobalStrings(int index);

// View, loop, frame parameter assertions.
// WARNING: these functions assume that view is already in an internal 0-based range.
void AssertView(const char *apiname, int view);
void AssertViewHasLoops(const char *apiname, int view);
void AssertLoop(const char *apiname, int view, int loop);
void AssertFrame(const char *apiname, int view, int loop, int frame);

int Game_GetInventoryItemCount();
int Game_GetFontCount();
int Game_GetMouseCursorCount();
int Game_GetCharacterCount();
int Game_GetGUICount();
int Game_GetViewCount();
int Game_GetUseNativeCoordinates();
int Game_GetSpriteWidth(int spriteNum);
int Game_GetSpriteHeight(int spriteNum);
int Game_GetLoopCountForView(int viewNumber);
int Game_GetRunNextSettingForLoop(int viewNumber, int loopNumber);
int Game_GetFrameCountForLoop(int viewNumber, int loopNumber);
ScriptViewFrame* Game_GetViewFrame(int viewNumber, int loopNumber, int frame);
int Game_DoOnceOnly(const char *token);

int  Game_GetTextReadingSpeed();
void Game_SetTextReadingSpeed(int newTextSpeed);
int  Game_GetMinimumTextDisplayTimeMs();
void Game_SetMinimumTextDisplayTimeMs(int newTextMinTime);
int  Game_GetIgnoreUserInputAfterTextTimeoutMs();
void Game_SetIgnoreUserInputAfterTextTimeoutMs(int newValueMs);
const char *Game_GetFileName();
const char *Game_GetName();
void Game_SetName(const char *newName);

int Game_GetSkippingCutscene();
int Game_GetInSkippableCutscene();

int Game_GetColorFromRGB(int red, int grn, int blu);
const char* Game_InputBox(const char *msg);
const char* Game_GetLocationName(int x, int y);

const char* Game_GetGlobalMessages(int index);

int Game_GetSpeechFont();
int Game_GetNormalFont();

const char* Game_GetTranslationFilename();
int Game_ChangeTranslation(const char *newFilename);
const char* Game_GetSpeechVoxFilename();
bool Game_ChangeSpeechVox(const char *newFilename);

//=============================================================================

void set_debug_mode(bool on);
void set_game_speed(int new_fps);
void setup_for_dialog();
void restore_after_dialog();
Common::String get_save_game_directory();
Common::String get_save_game_suffix();
void set_save_game_suffix(const Common::String &suffix);
// Returns full path to the save for the given slot number
Common::String get_save_game_path(int slotNum);
// Parses filename and retrieves save slot number, if present
bool get_save_slotnum(const Common::String &filename, int &slot);
// Try calling built-in restore game dialog;
// NOTE: this is a script command; may be aborted according to the game & room settings
void restore_game_dialog();
// Unconditionally display a built-in restore game dialog
bool do_restore_game_dialog();
// Try calling built-in save game dialog;
// NOTE: this is a script command; may be aborted according to the game & room settings
void save_game_dialog();
// Unconditionally display a built-in save game dialog
bool do_save_game_dialog();
void free_do_once_tokens();
// Free all the memory associated with the game
void unload_game_file();
void save_game(int slotn, const char*descript);
bool read_savedgame_description(const Common::String &savedgame, Common::String &description);
std::unique_ptr<Common::Bitmap> read_savedgame_screenshot(const Common::String &savedgame);
// Tries to restore saved game and displays an error on failure; if the error occured
// too late, when the game data was already overwritten, shuts engine down.
bool try_restore_save(int slot);
bool try_restore_save(const Common::String &path, int slot);
void serialize_bitmap(const Common::Bitmap *thispic, Common::Stream *out);
Common::Bitmap *read_serialized_bitmap(Common::Stream *in);
void skip_serialized_bitmap(Common::Stream *in);

bool is_in_cutscene();
CutsceneSkipStyle get_cutscene_skipstyle();
void start_skipping_cutscene ();
bool check_skip_cutscene_keypress(int kgn);
bool check_skip_cutscene_mclick(int mbut);
void initialize_skippable_cutscene();
void stop_fast_forwarding();

int __GetLocationType(int xxx,int yyy, int allowHotspot0);

// Called whenever game loses input focus
void display_switch_out();
// Called whenever game gets input focus
void display_switch_in();
// Called when the game loses input focus and must suspend
void display_switch_out_suspend();
// Called when the game gets input focus and should resume
void display_switch_in_resume();

void replace_tokens(const char*srcmes,char*destm, int maxlen = 99999);
const char *get_global_message (int msnum);
void get_message_text (int msnum, char *buffer, char giveErr = 1);

// Notifies the game objects that certain sprite was updated.
// This make them update their render states, caches, and so on.
void game_sprite_updated(int sprnum, bool deleted = false);
// Precaches sprites for a view, within a selected range of loops.
void precache_view(int view, int first_loop = 0, int last_loop = INT32_MAX, bool with_sounds = false);

extern int in_new_room;
extern int new_room_pos;
extern int new_room_x, new_room_y, new_room_loop;
extern bool new_room_placeonwalkable;
extern int displayed_room;
extern int frames_per_second; // fixed game fps, set by script
extern unsigned int loopcounter;
extern void set_loop_counter(unsigned int new_counter);
extern int game_paused;

#endif // __AGS_EE_AC__GAME_H
