
void PauseGame();
int IsGamePaused();
void SetGameSpeed(int newspd);
void SetGlobalInt(int index,int valu);
int LoadSaveSlotScreenshot(int slnum, int width, int height);

extern char saveGameDirectory[260];
extern int want_quit;

