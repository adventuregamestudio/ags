#ifndef __AC_OVERLAY_H
#define __AC_OVERLAY_H

#include "ac/dynobj/scriptoverlay.h"
#include "ac/screenoverlay.h"
#include "ac/rundefines.h"

int find_overlay_of_type(int typ);
void remove_screen_overlay(int type);
void get_overlay_position(int overlayidx, int *x, int *y);
int add_screen_overlay(int x,int y,int type,block piccy, bool alphaChannel = false);
void remove_screen_overlay_index(int cc);


void RemoveOverlay(int ovrid);
void Overlay_Remove(ScriptOverlay *sco);
int CreateGraphicOverlay(int xx,int yy,int slott,int trans);
int CreateTextOverlayCore(int xx, int yy, int wii, int fontid, int clr, const char *tex, int allowShrink);
int CreateTextOverlay(int xx,int yy,int wii,int fontid,int clr,char*texx, ...);
void SetTextOverlay(int ovrid,int xx,int yy,int wii,int fontid,int clr,char*texx,...);
void Overlay_SetText(ScriptOverlay *scover, int wii, int fontid, int clr, char*texx, ...);
int Overlay_GetX(ScriptOverlay *scover);
void Overlay_SetX(ScriptOverlay *scover, int newx);
int Overlay_GetY(ScriptOverlay *scover);
void Overlay_SetY(ScriptOverlay *scover, int newy);
void MoveOverlay(int ovrid, int newx,int newy);
int IsOverlayValid(int ovrid);
int Overlay_GetValid(ScriptOverlay *scover);
ScriptOverlay* Overlay_CreateGraphical(int x, int y, int slot, int transparent);
ScriptOverlay* Overlay_CreateTextual(int x, int y, int width, int font, int colour, const char* text, ...);


extern ScreenOverlay screenover[MAX_SCREEN_OVERLAYS];
extern int is_complete_overlay,is_text_overlay;

#endif // __AC_OVERLAY_H