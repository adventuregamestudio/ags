
#include "acrun/ac_scriptoverlay.h"
#include "acrun/ac_screenoverlay.h"

int find_overlay_of_type(int typ);
void remove_screen_overlay(int type);

extern ScreenOverlay screenover[MAX_SCREEN_OVERLAYS];
extern int is_complete_overlay,is_text_overlay;
