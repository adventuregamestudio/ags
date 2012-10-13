//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================

#include "ac/global_region.h"
#include "util/wgt2allg.h"
#include "ac/common.h"
#include "ac/draw.h"
#include "ac/region.h"
#include "ac/roomstatus.h"
#include "ac/roomstruct.h"
#include "debug/debug_log.h"
#include "script/script.h"
#include "gfx/bitmap.h"

using AGS::Common::Bitmap;

extern roomstruct thisroom;
extern RoomStatus*croom;
extern char*evblockbasename;
extern int evblocknum;

int GetRegionAt (int xxx, int yyy) {
    // if the co-ordinates are off the edge of the screen,
    // correct them to be just within
    // this fixes walk-off-screen problems
    xxx = convert_to_low_res(xxx);
    yyy = convert_to_low_res(yyy);

    if (xxx >= thisroom.regions->GetWidth())
        xxx = thisroom.regions->GetWidth() - 1;
    if (yyy >= thisroom.regions->GetHeight())
        yyy = thisroom.regions->GetHeight() - 1;
    if (xxx < 0)
        xxx = 0;
    if (yyy < 0)
        yyy = 0;

    int hsthere = thisroom.regions->GetPixel (xxx, yyy);
    if (hsthere < 0)
        hsthere = 0;

    if (hsthere >= MAX_REGIONS) {
        char tempmsg[300];
        sprintf(tempmsg, "!An invalid pixel was found on the room region mask (colour %d, location: %d, %d)", hsthere, xxx, yyy);
        quit(tempmsg);
    }

    if (croom->region_enabled[hsthere] == 0)
        return 0;
    return hsthere;
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

void RunRegionInteraction (int regnum, int mood) {
    if ((regnum < 0) || (regnum >= MAX_REGIONS))
        quit("!RunRegionInteraction: invalid region speicfied");
    if ((mood < 0) || (mood > 2))
        quit("!RunRegionInteraction: invalid event specified");

    // We need a backup, because region interactions can run
    // while another interaction (eg. hotspot) is in a Wait
    // command, and leaving our basename would call the wrong
    // script later on
    char *oldbasename = evblockbasename;
    int   oldblocknum = evblocknum;

    evblockbasename = "region%d";
    evblocknum = regnum;

    if (thisroom.regionScripts != NULL)
    {
        run_interaction_script(thisroom.regionScripts[regnum], mood);
    }
    else
    {
        run_interaction_event(&croom->intrRegion[regnum], mood);
    }

    evblockbasename = oldbasename;
    evblocknum = oldblocknum;
}
