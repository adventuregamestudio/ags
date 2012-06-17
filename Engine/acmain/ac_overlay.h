#ifndef __AC_OVERLAY_H
#define __AC_OVERLAY_H

#include "acrun/ac_scriptoverlay.h"
#include "acrun/ac_screenoverlay.h"

int find_overlay_of_type(int typ);
void remove_screen_overlay(int type);
void get_overlay_position(int overlayidx, int *x, int *y);
int add_screen_overlay(int x,int y,int type,block piccy, bool alphaChannel = false);

int Overlay_GetValid(ScriptOverlay *scover);

extern ScreenOverlay screenover[MAX_SCREEN_OVERLAYS];
extern int is_complete_overlay,is_text_overlay;

#endif // __AC_OVERLAY_H