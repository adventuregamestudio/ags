#ifndef __AC_LOCATION_H
#define __AC_LOCATION_H

#include "ac/dynobj/scriptgui.h"
#include "ac/dynobj/scriptobject.h"
#include "ac/dynobj/scriptregion.h"
#include "ac/ac_characterinfo.h"

int get_walkable_area_at_location(int xx, int yy);
int get_walkable_area_at_character (int charnum);
int GetRegionAt (int xxx, int yyy);
ScriptRegion *GetRegionAtLocation(int xx, int yy);
int get_hotspot_at(int xpp,int ypp);
int GetGUIAt (int xx,int yy);
ScriptGUI *GetGUIAtLocation(int xx, int yy);
int isposinbox(int mmx,int mmy,int lf,int tp,int rt,int bt);

int is_pos_in_sprite(int xx,int yy,int arx,int ary, block sprit, int spww,int sphh, int flipped = 0);

int GetObjectAt(int xx,int yy);
ScriptObject *GetObjectAtLocation(int xx, int yy);
// X and Y co-ordinates must be in 320x200 format
int check_click_on_object(int xx,int yy,int mood);
int is_pos_on_character(int xx,int yy);
int GetCharacterAt (int xx, int yy);
CharacterInfo *GetCharacterAtLocation(int xx, int yy);

int __GetLocationType(int xxx,int yyy, int allowHotspot0);

int GetLocationType(int xxx,int yyy);
void SaveCursorForLocationChange();
void GetLocationName(int xxx,int yyy,char*tempo);
const char* Game_GetLocationName(int x, int y);

extern int getloctype_index, getloctype_throughgui;

#endif // __AC_LOCATION_H


