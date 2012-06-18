
#include "acrun/ac_scriptviewframe.h"

void restart_game();
void set_game_speed(int fps);
void setup_for_dialog();
void restore_after_dialog();
void RestoreGameSlot(int slnum);
void get_save_game_path(int slotNum, char *buffer);
void DeleteSaveSlot (int slnum);
int Game_SetSaveGameDirectory(const char *newFolder);
int GetSaveSlotDescription(int slnum,char*desbuf);
const char* Game_GetSaveSlotDescription(int slnum);
int LoadSaveSlotScreenshot(int slnum, int width, int height);
int load_game_and_print_error(int toload);
void restore_game_dialog();
void save_game_dialog();
void PauseGame();
void UnPauseGame();
int IsGamePaused();
void setup_sierra_interface();

int load_game_file();
void free_do_once_tokens();
// Free all the memory associated with the game
void unload_game_file();
void SetGlobalInt(int index,int valu);
int GetGlobalInt(int index);
void SetGlobalString (int index, char *newval);
void GetGlobalString (int index, char *strval);
const char* Game_GetGlobalStrings(int index);

int RunAGSGame (char *newgame, unsigned int mode, int data);

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

int GetGameParameter (int parm, int data1, int data2, int data3);
int Game_GetTextReadingSpeed();
void Game_SetTextReadingSpeed(int newTextSpeed);
int Game_GetMinimumTextDisplayTimeMs();
void Game_SetMinimumTextDisplayTimeMs(int newTextMinTime);
int Game_GetIgnoreUserInputAfterTextTimeoutMs();
void Game_SetIgnoreUserInputAfterTextTimeoutMs(int newValueMs);
const char *Game_GetFileName();
const char *Game_GetName();
void Game_SetName(const char *newName);
void QuitGame(int dialog);
void SetRestartPoint();
void SetGameSpeed(int newspd);
int GetGameSpeed();
int SetGameOption (int opt, int setting);
int GetGameOption (int opt);

void serialize_bitmap(block thispic, FILE*ooo);

void safeguard_string (unsigned char *descript);
// On Windows we could just use IIDFromString but this is platform-independant
void convert_guid_from_text_to_binary(const char *guidText, unsigned char *buffer);
block read_serialized_bitmap(FILE* ooo);
long write_screen_shot_for_vista(FILE *ooo, block screenshot);

void save_game_data (FILE *ooo, block screenshot);
void save_game(int slotn, const char*descript);

int restore_game_data (FILE *ooo, const char *nametouse);

int do_game_load(const char *nametouse, int slotNumber, char *descrp, int *wantShot);
int load_game(int slotn, char*descrp, int *wantShot);;





extern char saveGameDirectory[260];
extern int want_quit;
extern char pexbuf[STD_BUFFER_SIZE];
extern int game_paused;

extern const char *load_game_errors[9];

extern unsigned int load_new_game;
extern int load_new_game_restore;

extern int gameHasBeenRestored;

