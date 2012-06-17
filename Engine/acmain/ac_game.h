
void PauseGame();
void UnPauseGame();
int IsGamePaused();
void SetGameSpeed(int newspd);
void SetGlobalInt(int index,int valu);
int LoadSaveSlotScreenshot(int slnum, int width, int height);
int GetGameSpeed();
int load_game(int slotn, char*descrp, int *wantShot);
void save_game(int slotn, const char*descript);
int GetSaveSlotDescription(int slnum,char*desbuf);
void setup_for_dialog();
void restore_after_dialog();
void QuitGame(int dialog);
void SetRestartPoint();
int restore_game_data (FILE *ooo, const char *nametouse);
int do_game_load(const char *nametouse, int slotNumber, char *descrp, int *wantShot);
int RunAGSGame (char *newgame, unsigned int mode, int data);
void set_game_speed(int fps);
int load_game_file();
int Game_SetSaveGameDirectory(const char *newFolder);
int GetGlobalInt(int index);
void SetGlobalString (int index, char *newval);
void save_game_data (FILE *ooo, block screenshot);

extern char saveGameDirectory[260];
extern int want_quit;
extern char pexbuf[STD_BUFFER_SIZE];
extern int game_paused;

extern const char *load_game_errors[9];

extern unsigned int load_new_game;
extern int load_new_game_restore;

