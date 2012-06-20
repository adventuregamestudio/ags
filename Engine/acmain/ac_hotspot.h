
#include "ac/dynobj/scripthotspot.h"

void DisableHotspot(int hsnum);
void EnableHotspot(int hsnum);
void Hotspot_SetEnabled(ScriptHotspot *hss, int newval);
int Hotspot_GetEnabled(ScriptHotspot *hss);
int Hotspot_GetID(ScriptHotspot *hss);
int GetHotspotPointX (int hotspot);
int Hotspot_GetWalkToX(ScriptHotspot *hss);;
int GetHotspotPointY (int hotspot);
int Hotspot_GetWalkToY(ScriptHotspot *hss);
int GetHotspotAt(int xxx,int yyy);
ScriptHotspot *GetHotspotAtLocation(int xx, int yy);
void GetHotspotName(int hotspot, char *buffer);
void Hotspot_GetName(ScriptHotspot *hss, char *buffer);
const char* Hotspot_GetName_New(ScriptHotspot *hss);
