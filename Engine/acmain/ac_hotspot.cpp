
#include "acmain/ac_maindefines.h"


void DisableHotspot(int hsnum) {
  if ((hsnum<1) | (hsnum>=MAX_HOTSPOTS))
    quit("!DisableHotspot: invalid hotspot specified");
  croom->hotspot_enabled[hsnum]=0;
  DEBUG_CONSOLE("Hotspot %d disabled", hsnum);
}

void EnableHotspot(int hsnum) {
  if ((hsnum<1) | (hsnum>=MAX_HOTSPOTS))
    quit("!EnableHotspot: invalid hotspot specified");
  croom->hotspot_enabled[hsnum]=1;
  DEBUG_CONSOLE("Hotspot %d re-enabled", hsnum);
}

void Hotspot_SetEnabled(ScriptHotspot *hss, int newval) {
  if (newval)
    EnableHotspot(hss->id);
  else
    DisableHotspot(hss->id);
}

int Hotspot_GetEnabled(ScriptHotspot *hss) {
  return croom->hotspot_enabled[hss->id];
}

int Hotspot_GetID(ScriptHotspot *hss) {
  return hss->id;
}


int GetHotspotPointX (int hotspot) {
  if ((hotspot < 0) || (hotspot >= MAX_HOTSPOTS))
    quit("!GetHotspotPointX: invalid hotspot");

  if (thisroom.hswalkto[hotspot].x < 1)
    return -1;

  return thisroom.hswalkto[hotspot].x;
}

int Hotspot_GetWalkToX(ScriptHotspot *hss) {
  return GetHotspotPointX(hss->id);
}

int GetHotspotPointY (int hotspot) {
  if ((hotspot < 0) || (hotspot >= MAX_HOTSPOTS))
    quit("!GetHotspotPointY: invalid hotspot");

  if (thisroom.hswalkto[hotspot].x < 1)
    return -1;

  return thisroom.hswalkto[hotspot].y;
}

int Hotspot_GetWalkToY(ScriptHotspot *hss) {
  return GetHotspotPointY(hss->id);
}

int GetHotspotAt(int xxx,int yyy) {
  xxx += divide_down_coordinate(offsetx);
  yyy += divide_down_coordinate(offsety);
  if ((xxx>=thisroom.width) | (xxx<0) | (yyy<0) | (yyy>=thisroom.height))
    return 0;
  return get_hotspot_at(xxx,yyy);
}

ScriptHotspot *GetHotspotAtLocation(int xx, int yy) {
  int hsnum = GetHotspotAt(xx, yy);
  if (hsnum <= 0)
    return &scrHotspot[0];
  return &scrHotspot[hsnum];
}



void GetHotspotName(int hotspot, char *buffer) {
  VALIDATE_STRING(buffer);
  if ((hotspot < 0) || (hotspot >= MAX_HOTSPOTS))
    quit("!GetHotspotName: invalid hotspot number");

  strcpy(buffer, get_translation(thisroom.hotspotnames[hotspot]));
}

void Hotspot_GetName(ScriptHotspot *hss, char *buffer) {
  GetHotspotName(hss->id, buffer);
}

const char* Hotspot_GetName_New(ScriptHotspot *hss) {
  return CreateNewScriptString(get_translation(thisroom.hotspotnames[hss->id]));
}
