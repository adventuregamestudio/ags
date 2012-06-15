
#include "acmain/ac_maindefines.h"


void DisableRegion(int hsnum) {
  if ((hsnum < 0) || (hsnum >= MAX_REGIONS))
    quit("!DisableRegion: invalid region specified");

  croom->region_enabled[hsnum] = 0;
  DEBUG_CONSOLE("Region %d disabled", hsnum);
}

void EnableRegion(int hsnum) {
  if ((hsnum < 0) || (hsnum >= MAX_REGIONS))
    quit("!EnableRegion: invalid region specified");

  croom->region_enabled[hsnum] = 1;
  DEBUG_CONSOLE("Region %d enabled", hsnum);
}

void Region_SetEnabled(ScriptRegion *ssr, int enable) {
  if (enable)
    EnableRegion(ssr->id);
  else
    DisableRegion(ssr->id);
}

int Region_GetEnabled(ScriptRegion *ssr) {
  return croom->region_enabled[ssr->id];
}

int Region_GetID(ScriptRegion *ssr) {
  return ssr->id;
}


void DisableGroundLevelAreas(int alsoEffects) {
  if ((alsoEffects < 0) || (alsoEffects > 1))
    quit("!DisableGroundLevelAreas: invalid parameter: must be 0 or 1");

  play.ground_level_areas_disabled = GLED_INTERACTION;

  if (alsoEffects)
    play.ground_level_areas_disabled |= GLED_EFFECTS;

  DEBUG_CONSOLE("Ground-level areas disabled");
}

void EnableGroundLevelAreas() {
  play.ground_level_areas_disabled = 0;

  DEBUG_CONSOLE("Ground-level areas re-enabled");
}

