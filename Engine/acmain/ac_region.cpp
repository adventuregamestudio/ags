
#include <stdio.h>
#include "wgt2allg.h"
#include "acmain/ac_maindefines.h"
#include "acmain/ac_region.h"
#include "acmain/ac_commonheaders.h"

void generate_light_table() {
    int cc;
    if ((game.color_depth == 1) && (color_map == NULL)) {
        // in 256-col mode, check if we need the light table this room
        for (cc=0;cc < MAX_REGIONS;cc++) {
            if (thisroom.regionLightLevel[cc] < 0) {
                create_light_table(&maincoltable,palette,0,0,0,NULL);
                color_map=&maincoltable;
                break;
            }
        }
    }
}

void SetAreaLightLevel(int area, int brightness) {
    if ((area < 0) || (area > MAX_REGIONS))
        quit("!SetAreaLightLevel: invalid region");
    if (brightness < -100) brightness = -100;
    if (brightness > 100) brightness = 100;
    thisroom.regionLightLevel[area] = brightness;
    // disable RGB tint for this area
    thisroom.regionTintLevel[area] &= ~TINT_IS_ENABLED;
    generate_light_table();
    DEBUG_CONSOLE("Region %d light level set to %d", area, brightness);
}

void Region_SetLightLevel(ScriptRegion *ssr, int brightness) {
    SetAreaLightLevel(ssr->id, brightness);
}

int Region_GetLightLevel(ScriptRegion *ssr) {
    return thisroom.regionLightLevel[ssr->id];
}

void SetRegionTint (int area, int red, int green, int blue, int amount) {
    if ((area < 0) || (area > MAX_REGIONS))
        quit("!SetRegionTint: invalid region");

    if ((red < 0) || (red > 255) || (green < 0) || (green > 255) ||
        (blue < 0) || (blue > 255)) {
            quit("!SetRegionTint: RGB values must be 0-255");
    }

    // originally the value was passed as 0
    if (amount == 0)
        amount = 100;

    if ((amount < 1) || (amount > 100))
        quit("!SetRegionTint: amount must be 1-100");

    DEBUG_CONSOLE("Region %d tint set to %d,%d,%d", area, red, green, blue);

    /*red -= 100;
    green -= 100;
    blue -= 100;*/

    unsigned char rred = red;
    unsigned char rgreen = green;
    unsigned char rblue = blue;

    thisroom.regionTintLevel[area] = TINT_IS_ENABLED;
    thisroom.regionTintLevel[area] |= rred & 0x000000ff;
    thisroom.regionTintLevel[area] |= (int(rgreen) << 8) & 0x0000ff00;
    thisroom.regionTintLevel[area] |= (int(rblue) << 16) & 0x00ff0000;
    thisroom.regionLightLevel[area] = amount;
}

int Region_GetTintEnabled(ScriptRegion *srr) {
    if (thisroom.regionTintLevel[srr->id] & TINT_IS_ENABLED)
        return 1;
    return 0;
}

int Region_GetTintRed(ScriptRegion *srr) {

    return thisroom.regionTintLevel[srr->id] & 0x000000ff;
}

int Region_GetTintGreen(ScriptRegion *srr) {

    return (thisroom.regionTintLevel[srr->id] >> 8) & 0x000000ff;
}

int Region_GetTintBlue(ScriptRegion *srr) {

    return (thisroom.regionTintLevel[srr->id] >> 16) & 0x000000ff;
}

int Region_GetTintSaturation(ScriptRegion *srr) {

    return thisroom.regionLightLevel[srr->id];
}

void Region_Tint(ScriptRegion *srr, int red, int green, int blue, int amount) {
    SetRegionTint(srr->id, red, green, blue, amount);
}




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

