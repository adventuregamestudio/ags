/*
  AGS Runtime header

  This is UNPUBLISHED PROPRIETARY SOURCE CODE;
  the contents of this file may not be disclosed to third parties,
  copied or duplicated in any form, in whole or in part, without
  prior express permission from Chris Jones.
*/
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__GAME_H
#define __AGS_EE_AC__GAME_H

#include "ac/dynobj/scriptviewframe.h"
#include "platform/file.h"

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
void get_save_game_path(int slotNum, char *buffer);
int  load_game_and_print_error(int toload);
void restore_game_dialog();
void save_game_dialog();
void setup_sierra_interface();
int  load_game_file();
void free_do_once_tokens();
// Free all the memory associated with the game
void unload_game_file();
void save_game_data (FILE *ooo, block screenshot);
void save_game(int slotn, const char*descript);
int  restore_game_data (FILE *ooo, const char *nametouse);
int  do_game_load(const char *nametouse, int slotNumber, char *descrp, int *wantShot);
int  load_game(int slotn, char*descrp, int *wantShot);
void serialize_bitmap(block thispic, FILE*ooo);
void safeguard_string (unsigned char *descript);
// On Windows we could just use IIDFromString but this is platform-independant
void convert_guid_from_text_to_binary(const char *guidText, unsigned char *buffer);
block read_serialized_bitmap(FILE* ooo);
long write_screen_shot_for_vista(FILE *ooo, block screenshot);

void start_skipping_cutscene ();
void check_skip_cutscene_keypress (int kgn);
void initialize_skippable_cutscene();
void stop_fast_forwarding();

int __GetLocationType(int xxx,int yyy, int allowHotspot0);

#endif // __AGS_EE_AC__GAME_H
