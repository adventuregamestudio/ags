//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2023 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "ac/common.h"
#include "ac/gamesetup.h"
#include "ac/draw.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_game.h"
#include "ac/global_screen.h"
#include "ac/runtime_defines.h"
#include "ac/screen.h"
#include "debug/debug_log.h"
#include "main/game_run.h"
#include "media/audio/audio.h"
#include "platform/base/agsplatformdriver.h"
#include "gfx/graphicsdriver.h"
#include "gfx/bitmap.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern GameState play;
extern GameSetupStruct game;
extern RoomStruct thisroom;
extern IGraphicsDriver *gfxDriver;
extern AGSPlatformDriver *platform;
extern RGB palette[256];
extern unsigned int loopcounter;

void FlipScreen(int amount) {
    if ((amount<0) | (amount>3)) quit("!FlipScreen: invalid argument (0-3)");
    play.screen_flipped=amount;
}

void ShakeScreen(int severe) {
    EndSkippingUntilCharStops();

    if (play.fast_forward)
        return;


    // TODO: support shaking room viewport separately
    // TODO: rely on game speed setting? and/or provide frequency and duration args
    // TODO: unify blocking and non-blocking shake update

    // FIXME: keeping hardcoded 40 shakes with ~50 ms delay between shakes for now,
    // only use this is a factor, and calculate delay and duration params from this.
    int compat_shake_delay = static_cast<int>(50.f / (1000.f / GetGameSpeed()));
    int compat_duration = static_cast<int>(40 * (50.f / (1000.f / GetGameSpeed())));

    play.shakesc_length = compat_duration;
    play.shakesc_delay = 2 * compat_shake_delay;
    play.shakesc_amount = severe;
    play.mouse_cursor_hidden++;

    // FIXME: we have to sync audio here explicitly, because ShakeScreen
    // does not call any game update function while it works
    sync_audio_playback();
    if (gfxDriver->RequiresFullRedrawEachFrame())
    {
        for (int hh = 0; hh < play.shakesc_length; hh++)
        {
            loopcounter++;
            render_graphics();
            update_polled_stuff();
            WaitForNextFrame();
        }
    }
    else
    {
        // Optimized variant for software render: create game scene once and shake it
        construct_game_scene();
        gfxDriver->RenderToBackBuffer();
        for (int hh = 0; hh < play.shakesc_length; hh++)
        {
            const int yoff = hh % 2 == 0 ? 0 : severe;
            play.shake_screen_yoff = yoff;
            render_to_screen();
            update_polled_stuff();
            WaitForNextFrame();
        }
        render_to_screen();
    }
    sync_audio_playback();

    play.mouse_cursor_hidden--;
    play.shakesc_length = 0;
    play.shakesc_delay = 0;
    play.shakesc_amount = 0;
}

void ShakeScreenBackground (int delay, int amount, int length) {
    if (delay < 2) 
        quit("!ShakeScreenBackground: invalid delay parameter");
    play.shakesc_amount = amount;
    play.shakesc_delay = delay;
    play.shakesc_length = length;
}

void TintScreen(int red, int grn, int blu) {
    if ((red < 0) || (grn < 0) || (blu < 0) || (red > 100) || (grn > 100) || (blu > 100))
        quit("!TintScreen: RGB values must be 0-100");

    invalidate_screen();

    if ((red == 0) && (grn == 0) && (blu == 0)) {
        play.screen_tint = -1;
        return;
    }
    red = (red * 25) / 10;
    grn = (grn * 25) / 10;
    blu = (blu * 25) / 10;
    play.screen_tint = red + (grn << 8) + (blu << 16);
}

void FadeOut(int sppd) {
    EndSkippingUntilCharStops();

    if (play.fast_forward)
        return;

    // FIXME: we have to sync audio here explicitly, because FadeOut
    // does not call any game update function while it works
    sync_audio_playback();
    fadeout_impl(sppd);
    sync_audio_playback();
}

void fadeout_impl(int spdd) {
    if (play.screen_is_faded_out == 0)
    {
        gfxDriver->FadeOut(spdd, play.fade_to_red, play.fade_to_green, play.fade_to_blue);
    }

    if (game.color_depth > 1)
        play.screen_is_faded_out = 1;
}

void SetScreenTransition(int newtrans) {
    if ((newtrans < 0) || (newtrans > FADE_LAST))
        quit("!SetScreenTransition: invalid transition type");

    play.fade_effect = newtrans;

    debug_script_log("Screen transition changed");
}

void SetNextScreenTransition(int newtrans) {
    if ((newtrans < 0) || (newtrans > FADE_LAST))
        quit("!SetNextScreenTransition: invalid transition type");

    play.next_screen_transition = newtrans;

    debug_script_log("SetNextScreenTransition engaged");
}

void SetFadeColor(int red, int green, int blue) {
    if ((red < 0) || (red > 255) || (green < 0) || (green > 255) ||
        (blue < 0) || (blue > 255))
        quit("!SetFadeColor: Red, Green and Blue must be 0-255");

    play.fade_to_red = red;
    play.fade_to_green = green;
    play.fade_to_blue = blue;
}

void FadeIn(int sppd) {
    EndSkippingUntilCharStops();

    if (play.fast_forward)
        return;

    // FIXME: we have to sync audio here explicitly, because FadeIn
    // does not call any game update function while it works
    sync_audio_playback();
    fadein_impl(palette,sppd);
    sync_audio_playback();
}
