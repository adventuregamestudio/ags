
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__REGION_H
#define __AGS_EE_AC__REGION_H

#include "ac/dynobj/scriptregion.h"

ScriptRegion *GetRegionAtLocation(int xx, int yy);
void    Region_SetLightLevel(ScriptRegion *ssr, int brightness);
int     Region_GetLightLevel(ScriptRegion *ssr);
int     Region_GetTintEnabled(ScriptRegion *srr);
int     Region_GetTintRed(ScriptRegion *srr);
int     Region_GetTintGreen(ScriptRegion *srr);
int     Region_GetTintBlue(ScriptRegion *srr);
int     Region_GetTintSaturation(ScriptRegion *srr);
void    Region_Tint(ScriptRegion *srr, int red, int green, int blue, int amount);
void    Region_SetEnabled(ScriptRegion *ssr, int enable);
int     Region_GetEnabled(ScriptRegion *ssr);
int     Region_GetID(ScriptRegion *ssr);

void    generate_light_table();

#endif // __AGS_EE_AC__REGION_H
