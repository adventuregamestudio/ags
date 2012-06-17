
void my_fade_out(int spdd);
void my_fade_in(PALLETE p, int speed);

void current_fade_out_effect ();
IDriverDependantBitmap* prepare_screen_for_transition_in();
void SetScreenTransition(int newtrans);
void SetNextScreenTransition(int newtrans);
void SetFadeColor(int red, int green, int blue);
void FadeIn(int sppd);

extern block temp_virtual;
extern color old_palette[256];
