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

#include "gfx/ali3d.h"
#include "ac/common.h"
#include "ac/draw.h"
#include "ac/global_game.h"
#include "ac/global_screen.h"
#include "ac/runtime_defines.h"
#include "ac/screen.h"
#include "debug/debug_log.h"
#include "game/game_objects.h"
#include "platform/base/agsplatformdriver.h"
#include "gfx/graphicsdriver.h"
#include "gfx/bitmap.h"

using AGS::Common::Bitmap;
namespace BitmapHelper = AGS::Common::BitmapHelper;

extern IGraphicsDriver *gfxDriver;
extern AGSPlatformDriver *platform;
extern color palette[256];
extern unsigned int loopcounter;

int scrnwid,scrnhit;
int current_screen_resolution_multiplier = 1;
int force_letterbox = 0;

int final_scrn_wid=0,final_scrn_hit=0,final_col_dep=0;
int screen_reset = 0;

int GetMaxScreenHeight () {
    int maxhit = BASEHEIGHT;
    if ((maxhit == 200) || (maxhit == 400))
    {
        // uh ... BASEHEIGHT depends on Native Coordinates setting so be careful
        if ((usetup.WantLetterbox) && (thisroom.Height > maxhit)) 
            maxhit = divide_down_coordinate(multiply_up_coordinate(maxhit) + get_fixed_pixel_size(40));
    }
    return maxhit;
}

void FlipScreen(int amount) {
    if ((amount<0) | (amount>3)) quit("!FlipScreen: invalid argument (0-3)");
    play.ScreenFlipped=amount;
}

void ShakeScreen(int severe) {
    EndSkippingUntilCharStops();

    if (play.FastForwardCutscene)
        return;

    int hh;
    //Bitmap *oldsc=abuf; // CHECKME!!!
    severe = multiply_up_coordinate(severe);

    if (gfxDriver->RequiresFullRedrawEachFrame())
    {
        play.ShakeScreenLength = 10;
        play.ShakeScreenDelay = 2;
        play.ShakeScreenAmount = severe;
        play.MouseCursorHidden++;

        for (hh = 0; hh < 40; hh++) {
            loopcounter++;
            platform->Delay(50);

            render_graphics();

            update_polled_stuff_if_runtime();
        }

        play.MouseCursorHidden--;
        clear_letterbox_borders();
        play.ShakeScreenLength = 0;
    }
    else
    {
        Bitmap *tty = BitmapHelper::CreateBitmap(scrnwid, scrnhit);
        gfxDriver->GetCopyOfScreenIntoBitmap(tty);
        for (hh=0;hh<40;hh++) {
            platform->Delay(50);

            if (hh % 2 == 0) 
                render_to_screen(tty, 0, 0);
            else
                render_to_screen(tty, 0, severe);

            update_polled_stuff_if_runtime();
        }
        clear_letterbox_borders();
        render_to_screen(tty, 0, 0);
        delete tty;
    }

    //abuf=oldsc;// CHECKME!!!
}

void ShakeScreenBackground (int delay, int amount, int length) {
    if (delay < 2) 
        quit("!ShakeScreenBackground: invalid delay parameter");

    if (amount < play.ShakeScreenAmount)
    {
        // from a bigger to smaller shake, clear up the borders
        clear_letterbox_borders();
    }

    play.ShakeScreenAmount = amount;
    play.ShakeScreenDelay = delay;
    play.ShakeScreenLength = length;
}

void TintScreen(int red, int grn, int blu) {
    if ((red < 0) || (grn < 0) || (blu < 0) || (red > 100) || (grn > 100) || (blu > 100))
        quit("!TintScreen: RGB values must be 0-100");

    invalidate_screen();

    if ((red == 0) && (grn == 0) && (blu == 0)) {
        play.ScreenTint = -1;
        return;
    }
    red = (red * 25) / 10;
    grn = (grn * 25) / 10;
    blu = (blu * 25) / 10;
    play.ScreenTint = red + (grn << 8) + (blu << 16);
}

void my_fade_out(int spdd) {
    EndSkippingUntilCharStops();

    if (play.FastForwardCutscene)
        return;

    if (play.ScreenIsFadedOut == 0)
        gfxDriver->FadeOut(spdd, play.FadeToRed, play.FadeToGreen, play.FadeToBlue);

    if (game.ColorDepth > 1)
        play.ScreenIsFadedOut = 1;
}

void SetScreenTransition(int newtrans) {
    if ((newtrans < 0) || (newtrans > FADE_LAST))
        quit("!SetScreenTransition: invalid transition type");

    play.TransitionStyle = newtrans;

    DEBUG_CONSOLE("Screen transition changed");
}

void SetNextScreenTransition(int newtrans) {
    if ((newtrans < 0) || (newtrans > FADE_LAST))
        quit("!SetNextScreenTransition: invalid transition type");

    play.NextRoomTransition = newtrans;

    DEBUG_CONSOLE("SetNextScreenTransition engaged");
}

void SetFadeColor(int red, int green, int blue) {
    if ((red < 0) || (red > 255) || (green < 0) || (green > 255) ||
        (blue < 0) || (blue > 255))
        quit("!SetFadeColor: Red, Green and Blue must be 0-255");

    play.FadeToRed = red;
    play.FadeToGreen = green;
    play.FadeToBlue = blue;
}

void FadeIn(int sppd) {
    EndSkippingUntilCharStops();

    if (play.FastForwardCutscene)
        return;

    my_fade_in(palette,sppd);
}
