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

#include "ac/common.h"
#include "ac/draw.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_game.h"
#include "ac/global_screen.h"
#include "ac/screen.h"
#include "platform/base/agsplatformdriver.h"
#include "plugin/agsplugin.h"
#include "plugin/plugin_engine.h"
#include "gfx/bitmap.h"
#include "gfx/graphicsdriver.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern GameSetupStruct game;
extern GameState play;
extern IGraphicsDriver *gfxDriver;
extern AGSPlatformDriver *platform;
extern Bitmap *virtual_screen;

// CLNUP why not using PALETTE instead of the mispelled term and how to take away the 8bit portion ?
void my_fade_in(PALLETE p, int speed) {
    if (game.color_depth > 1) {
        set_palette (p);

        play.screen_is_faded_out = 0;

        if (play.no_hicolor_fadein) {
            return;
        }
    }

    gfxDriver->FadeIn(speed, p, play.fade_to_red, play.fade_to_green, play.fade_to_blue);
}

Bitmap *saved_viewport_bitmap = NULL;
color old_palette[256];
void current_fade_out_effect () {
    if (pl_run_plugin_hooks(AGSE_TRANSITIONOUT, 0))
        return;

    // get the screen transition type
    int theTransition = play.fade_effect;
    // was a temporary transition selected? if so, use it
    if (play.next_screen_transition >= 0)
        theTransition = play.next_screen_transition;

    if ((theTransition == FADE_INSTANT) || (play.screen_tint >= 0)) {
        if (!play.keep_screen_during_instant_transition)
            set_palette_range(black_palette, 0, 255, 0);
    }
    else if (theTransition == FADE_NORMAL)
    {
        my_fade_out(5);
    }
    else if (theTransition == FADE_BOXOUT) 
    {
        gfxDriver->BoxOutEffect(true, 16, 1000 / GetGameSpeed());
        play.screen_is_faded_out = 1;
    }
    else 
    {
        get_palette(old_palette);
        Bitmap *ds = GetVirtualScreen();
        saved_viewport_bitmap = BitmapHelper::CreateBitmap(virtual_screen->GetWidth(),virtual_screen->GetHeight(),ds->GetColorDepth());
        gfxDriver->GetCopyOfScreenIntoBitmap(saved_viewport_bitmap);
    }
}

IDriverDependantBitmap* prepare_screen_for_transition_in()
{
    if (saved_viewport_bitmap == NULL)
        quit("Crossfade: buffer is null attempting transition");

    saved_viewport_bitmap = ReplaceBitmapWithSupportedFormat(saved_viewport_bitmap);
    const Rect &viewport = play.GetMainViewport();
    if (saved_viewport_bitmap->GetHeight() < viewport.GetHeight())
    {
        Bitmap *enlargedBuffer = BitmapHelper::CreateBitmap(saved_viewport_bitmap->GetWidth(), viewport.GetHeight(), saved_viewport_bitmap->GetColorDepth());
        enlargedBuffer->Blit(saved_viewport_bitmap, 0, 0, 0, (viewport.GetHeight() - saved_viewport_bitmap->GetHeight()) / 2, saved_viewport_bitmap->GetWidth(), saved_viewport_bitmap->GetHeight());
        delete saved_viewport_bitmap;
        saved_viewport_bitmap = enlargedBuffer;
    }
    else if (saved_viewport_bitmap->GetHeight() > viewport.GetHeight())
    {
        Bitmap *clippedBuffer = BitmapHelper::CreateBitmap(saved_viewport_bitmap->GetWidth(), viewport.GetHeight(), saved_viewport_bitmap->GetColorDepth());
        clippedBuffer->Blit(saved_viewport_bitmap, 0, (saved_viewport_bitmap->GetHeight() - viewport.GetHeight()) / 2, 0, 0, saved_viewport_bitmap->GetWidth(), saved_viewport_bitmap->GetHeight());
        delete saved_viewport_bitmap;
        saved_viewport_bitmap = clippedBuffer;
    }
    saved_viewport_bitmap->Acquire();
    IDriverDependantBitmap *ddb = gfxDriver->CreateDDBFromBitmap(saved_viewport_bitmap, false);
    return ddb;
}
