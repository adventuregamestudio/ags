
#include "ac/dynobj/scriptregion.h"

void generate_light_table();
void SetAreaLightLevel(int area, int brightness);
void Region_SetLightLevel(ScriptRegion *ssr, int brightness);
int Region_GetLightLevel(ScriptRegion *ssr);
void SetRegionTint (int area, int red, int green, int blue, int amount);
int Region_GetTintEnabled(ScriptRegion *srr);
int Region_GetTintRed(ScriptRegion *srr);
int Region_GetTintGreen(ScriptRegion *srr);
int Region_GetTintBlue(ScriptRegion *srr);
int Region_GetTintSaturation(ScriptRegion *srr);
void Region_Tint(ScriptRegion *srr, int red, int green, int blue, int amount);
void DisableRegion(int hsnum);
void EnableRegion(int hsnum);
void Region_SetEnabled(ScriptRegion *ssr, int enable);
int Region_GetEnabled(ScriptRegion *ssr);
int Region_GetID(ScriptRegion *ssr);
void DisableGroundLevelAreas(int alsoEffects);
void EnableGroundLevelAreas(); 