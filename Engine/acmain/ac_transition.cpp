
#include "wgt2allg.h"
#include "ali3d.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_game.h"
#include "acmain/ac_maindefines.h"
#include "acmain/ac_transition.h"
#include "acmain/ac_commonheaders.h"
#include "plugin/agsplugin.h"

extern GameSetupStruct game;
extern GameState play;

void my_fade_out(int spdd) {
    EndSkippingUntilCharStops();

    if (play.fast_forward)
        return;

    if (play.screen_is_faded_out == 0)
        gfxDriver->FadeOut(spdd, play.fade_to_red, play.fade_to_green, play.fade_to_blue);

    if (game.color_depth > 1)
        play.screen_is_faded_out = 1;
}

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




//#define _get_script_data_stack_size() (256*sizeof(int)+((int*)&scrpt[10*4])[0]+((int*)&scrpt[12*4])[0])
//#define _get_script_data_stack_size(instac) ((int*)instac->code)[10]
block temp_virtual = NULL;
color old_palette[256];
void current_fade_out_effect () {
    if (platform->RunPluginHooks(AGSE_TRANSITIONOUT, 0))
        return;

    // get the screen transition type
    int theTransition = play.fade_effect;
    // was a temporary transition selected? if so, use it
    if (play.next_screen_transition >= 0)
        theTransition = play.next_screen_transition;

    if ((theTransition == FADE_INSTANT) || (play.screen_tint >= 0)) {
        if (!play.keep_screen_during_instant_transition)
            wsetpalette(0,255,black_palette);
    }
    else if (theTransition == FADE_NORMAL)
    {
        my_fade_out(5);
    }
    else if (theTransition == FADE_BOXOUT) 
    {
        gfxDriver->BoxOutEffect(true, get_fixed_pixel_size(16), 1000 / GetGameSpeed());
        play.screen_is_faded_out = 1;
    }
    else 
    {
        get_palette(old_palette);
        temp_virtual = create_bitmap_ex(bitmap_color_depth(abuf),virtual_screen->w,virtual_screen->h);
        //blit(abuf,temp_virtual,0,0,0,0,abuf->w,abuf->h);
        gfxDriver->GetCopyOfScreenIntoBitmap(temp_virtual);
    }
}


IDriverDependantBitmap* prepare_screen_for_transition_in()
{
    if (temp_virtual == NULL)
        quit("Crossfade: buffer is null attempting transition");

    temp_virtual = gfxDriver->ConvertBitmapToSupportedColourDepth(temp_virtual);
    if (temp_virtual->h < scrnhit)
    {
        block enlargedBuffer = create_bitmap_ex(bitmap_color_depth(temp_virtual), temp_virtual->w, scrnhit);
        blit(temp_virtual, enlargedBuffer, 0, 0, 0, (scrnhit - temp_virtual->h) / 2, temp_virtual->w, temp_virtual->h);
        destroy_bitmap(temp_virtual);
        temp_virtual = enlargedBuffer;
    }
    else if (temp_virtual->h > scrnhit)
    {
        block clippedBuffer = create_bitmap_ex(bitmap_color_depth(temp_virtual), temp_virtual->w, scrnhit);
        blit(temp_virtual, clippedBuffer, 0, (temp_virtual->h - scrnhit) / 2, 0, 0, temp_virtual->w, temp_virtual->h);
        destroy_bitmap(temp_virtual);
        temp_virtual = clippedBuffer;
    }
    acquire_bitmap(temp_virtual);
    IDriverDependantBitmap *ddb = gfxDriver->CreateDDBFromBitmap(temp_virtual, false);
    return ddb;
}


void SetScreenTransition(int newtrans) {
    if ((newtrans < 0) || (newtrans > FADE_LAST))
        quit("!SetScreenTransition: invalid transition type");

    play.fade_effect = newtrans;

    DEBUG_CONSOLE("Screen transition changed");
}

void SetNextScreenTransition(int newtrans) {
    if ((newtrans < 0) || (newtrans > FADE_LAST))
        quit("!SetNextScreenTransition: invalid transition type");

    play.next_screen_transition = newtrans;

    DEBUG_CONSOLE("SetNextScreenTransition engaged");
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

    my_fade_in(palette,sppd);
}

