
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__GLOBALREGION_H
#define __AGS_EE_AC__GLOBALREGION_H

int  GetRegionAt (int xxx, int yyy);
void SetAreaLightLevel(int area, int brightness);
void SetRegionTint (int area, int red, int green, int blue, int amount);
void DisableRegion(int hsnum);
void EnableRegion(int hsnum);
void DisableGroundLevelAreas(int alsoEffects);
void EnableGroundLevelAreas();
void RunRegionInteraction (int regnum, int mood);

#endif // __AGS_EE_AC__GLOBALREGION_H
