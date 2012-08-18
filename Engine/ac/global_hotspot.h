
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__GLOBALHOTSPOT_H
#define __AGS_EE_AC__GLOBALHOTSPOT_H

void DisableHotspot(int hsnum);
void EnableHotspot(int hsnum);
int  GetHotspotPointX (int hotspot);
int  GetHotspotPointY (int hotspot);
int  GetHotspotAt(int xxx,int yyy);
void GetHotspotName(int hotspot, char *buffer);
void RunHotspotInteraction (int hotspothere, int mood);

int  GetHotspotProperty (int hss, const char *property);
void GetHotspotPropertyText (int item, const char *property, char *bufer);


#endif // __AGS_EE_AC__GLOBALHOTSPOT_H
