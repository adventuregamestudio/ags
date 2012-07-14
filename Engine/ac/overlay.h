
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__OVERLAY_H
#define __AGS_EE_AC__OVERLAY_H

#include "ac/dynobj/scriptoverlay.h"

void Overlay_Remove(ScriptOverlay *sco);
void Overlay_SetText(ScriptOverlay *scover, int wii, int fontid, int clr, char*texx, ...);
int  Overlay_GetX(ScriptOverlay *scover);
void Overlay_SetX(ScriptOverlay *scover, int newx);
int  Overlay_GetY(ScriptOverlay *scover);
void Overlay_SetY(ScriptOverlay *scover, int newy);
int  Overlay_GetValid(ScriptOverlay *scover);
ScriptOverlay* Overlay_CreateGraphical(int x, int y, int slot, int transparent);
ScriptOverlay* Overlay_CreateTextual(int x, int y, int width, int font, int colour, const char* text, ...);

int  find_overlay_of_type(int typ);
void remove_screen_overlay(int type);
void get_overlay_position(int overlayidx, int *x, int *y);
int  add_screen_overlay(int x,int y,int type,block piccy, bool alphaChannel = false);
void remove_screen_overlay_index(int cc);

#endif // __AGS_EE_AC__OVERLAY_H
