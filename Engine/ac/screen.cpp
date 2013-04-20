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
#include "ac/screen.h"
#include "game/game_objects.h"
#include "platform/base/agsplatformdriver.h"
#include "plugin/agsplugin.h"
#include "gfx/graphicsdriver.h"
#include "gfx/graphics.h"

using AGS::Common::Bitmap;
using AGS::Common::Graphics;
namespace BitmapHelper = AGS::Common::BitmapHelper;

extern IGraphicsDriver *gfxDriver;
extern AGSPlatformDriver *platform;
extern Bitmap *virtual_screen;
extern int scrnwid,scrnhit;

void my_fade_in(PALLETE p, int speed) {
    if (game.ColorDepth > 1) {
        set_palette (p);

        play.ScreenIsFadedOut = 0;

        if (play.NoHicolorFadeIn) {
            return;
        }
    }

    gfxDriver->FadeIn(speed, p, play.FadeToRed, play.FadeToGreen, play.FadeToBlue);
}

//#define _get_script_data_stack_size() (256*sizeof(int)+((int*)&scrpt[10*4])[0]+((int*)&scrpt[12*4])[0])
//#define _get_script_data_stack_size(instac) ((int*)instac->code)[10]
Bitmap *temp_virtual = NULL;
color old_palette[256];
void current_fade_out_effect () {
    if (platform->RunPluginHooks(AGSE_TRANSITIONOUT, 0))
        return;

    // get the screen transition type
    int theTransition = play.TransitionStyle;
    // was a temporary transition selected? if so, use it
    if (play.NextRoomTransition >= 0)
        theTransition = play.NextRoomTransition;

    if ((theTransition == FADE_INSTANT) || (play.ScreenTint >= 0)) {
        if (!play.KeepScreenDuringInstantTransition)
            set_palette_range(black_palette, 0, 255, 0);
    }
    else if (theTransition == FADE_NORMAL)
    {
        my_fade_out(5);
    }
    else if (theTransition == FADE_BOXOUT) 
    {
        gfxDriver->BoxOutEffect(true, get_fixed_pixel_size(16), 1000 / GetGameSpeed());
        play.ScreenIsFadedOut = 1;
    }
    else 
    {
        get_palette(old_palette);
        Common::Graphics *g = GetVirtualScreenGraphics();
        temp_virtual = BitmapHelper::CreateBitmap(virtual_screen->GetWidth(),virtual_screen->GetHeight(),g->GetBitmap()->GetColorDepth());
        //->Blit(abuf,temp_virtual,0,0,0,0,abuf->GetWidth(),abuf->GetHeight());
        gfxDriver->GetCopyOfScreenIntoBitmap(temp_virtual);
    }
}

IDriverDependantBitmap* prepare_screen_for_transition_in()
{
    if (temp_virtual == NULL)
        quit("Crossfade: buffer is null attempting transition");

    temp_virtual = gfxDriver->ConvertBitmapToSupportedColourDepth(temp_virtual);
    if (temp_virtual->GetHeight() < scrnhit)
    {
        Bitmap *enlargedBuffer = BitmapHelper::CreateBitmap(temp_virtual->GetWidth(), scrnhit, temp_virtual->GetColorDepth());
        Graphics graphics(enlargedBuffer);
        graphics.Blit(temp_virtual, 0, 0, 0, (scrnhit - temp_virtual->GetHeight()) / 2, temp_virtual->GetWidth(), temp_virtual->GetHeight());
        delete temp_virtual;
        temp_virtual = enlargedBuffer;
    }
    else if (temp_virtual->GetHeight() > scrnhit)
    {
        Bitmap *clippedBuffer = BitmapHelper::CreateBitmap(temp_virtual->GetWidth(), scrnhit, temp_virtual->GetColorDepth());
        Graphics graphics(clippedBuffer);
        graphics.Blit(temp_virtual, 0, (temp_virtual->GetHeight() - scrnhit) / 2, 0, 0, temp_virtual->GetWidth(), temp_virtual->GetHeight());
        delete temp_virtual;
        temp_virtual = clippedBuffer;
    }
    temp_virtual->Acquire();
    IDriverDependantBitmap *ddb = gfxDriver->CreateDDBFromBitmap(temp_virtual, false);
    return ddb;
}
