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
// AGS Runtime header
//
//=============================================================================

#ifndef __AGS_EE_AC__GAME_H
#define __AGS_EE_AC__GAME_H

#include "ac/interaction.h"
#include "ac/spritecache.h"
#include "ac/dynobj/scriptviewframe.h"
#include "game/savedgame.h"

// Forward declaration
namespace AGS { namespace Common { class Bitmap; class Stream; class String; } }
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

int Game_SetSaveGameDirectory(const char *newFolder);
const char* Game_GetSaveSlotDescription(int slnum);

const char* Game_GetGlobalStrings(int index);

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

//=============================================================================

void set_game_speed(int fps);
void setup_for_dialog();
void restore_after_dialog();
Common::String get_save_game_path(int slotNum);
void restore_game_dialog();
void save_game_dialog();
void setup_sierra_interface();
int  load_game_file();
void free_do_once_tokens();
// Free all the memory associated with the game
void unload_game_file();
void save_game_data (Common::Stream *out, Common::Bitmap *screenshot);
void save_game(int slotn, const char*descript);
Common::Bitmap *restore_game_screenshot(Common::Stream *in);
AGS::Engine::SavedGameError restore_game_data (Common::Stream *in);
void load_game_or_quit(int slotNumber);
void load_game_or_quit(const Common::String &path, int slotNumber);
void serialize_bitmap(Common::Bitmap *thispic, Common::Stream *out);
void safeguard_string (Common::String &descript);
// On Windows we could just use IIDFromString but this is platform-independant
void convert_guid_from_text_to_binary(const char *guidText, unsigned char *buffer);
Common::Bitmap *read_serialized_bitmap(Common::Stream *in);
long write_screen_shot_for_vista(Common::Stream *out, Common::Bitmap *screenshot);

void start_skipping_cutscene ();
void check_skip_cutscene_keypress (int kgn);
void initialize_skippable_cutscene();
void stop_fast_forwarding();

int __GetLocationType(int xxx,int yyy, int allowHotspot0);

void display_switch_out();
void display_switch_in();

void replace_tokens(char*srcmes,char*destm, int maxlen = 99999);
char *get_global_message (int msnum);
void get_message_text (int msnum, char *buffer, char giveErr = 1);

InteractionVariable *get_interaction_variable (int varindx);
InteractionVariable *FindGraphicalVariable(const char *varName);



extern int frames_per_second;
extern int displayed_room;
extern unsigned int loopcounter;
extern int game_paused;

struct ViewStruct;
extern ViewStruct*views;
extern SpriteCache spriteset;

extern int in_new_room;

#endif // __AGS_EE_AC__GAME_H
