
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__HOTSPOT_H
#define __AGS_EE_AC__HOTSPOT_H

#include "ac/dynobj/scripthotspot.h"

void    Hotspot_SetEnabled(ScriptHotspot *hss, int newval);
int     Hotspot_GetEnabled(ScriptHotspot *hss);
int     Hotspot_GetID(ScriptHotspot *hss);
ScriptHotspot *GetHotspotAtLocation(int xx, int yy);
int     Hotspot_GetWalkToX(ScriptHotspot *hss);;
int     Hotspot_GetWalkToY(ScriptHotspot *hss);
void    Hotspot_GetName(ScriptHotspot *hss, char *buffer);
const char* Hotspot_GetName_New(ScriptHotspot *hss);
void    Hotspot_RunInteraction (ScriptHotspot *hss, int mood);

int     Hotspot_GetProperty (ScriptHotspot *hss, const char *property);
void    Hotspot_GetPropertyText (ScriptHotspot *hss, const char *property, char *bufer);
const char* Hotspot_GetTextProperty(ScriptHotspot *hss, const char *property);

int     get_hotspot_at(int xpp,int ypp);

#endif // __AGS_EE_AC__HOTSPOT_H
