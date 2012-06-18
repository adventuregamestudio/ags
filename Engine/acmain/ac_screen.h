

int GetMaxScreenHeight ();
void FlipScreen(int amount);
void ShakeScreen(int severe);
void ShakeScreenBackground (int delay, int amount, int length);

extern int scrnwid,scrnhit;
extern int current_screen_resolution_multiplier;
extern int force_letterbox;

extern int final_scrn_wid,final_scrn_hit,final_col_dep;
extern int screen_reset;
extern int numscreenover;