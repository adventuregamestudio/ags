
#include <stdio.h>
#include "wgt2allg.h"
#include "acmain/ac_maindefines.h"
#include "acmain/ac_display.h"
#include "acmain/ac_commonheaders.h"
#include "gui/guiinv.h"
#include "media/audio/audio.h"


int adjust_pixel_size_for_loaded_data(int size, int filever)
{
    if (filever < 37)
    {
        return multiply_up_coordinate(size);
    }
    return size;
}

void adjust_pixel_sizes_for_loaded_data(int *x, int *y, int filever)
{
    x[0] = adjust_pixel_size_for_loaded_data(x[0], filever);
    y[0] = adjust_pixel_size_for_loaded_data(y[0], filever);
}

void adjust_sizes_for_resolution(int filever)
{
    int ee;
    for (ee = 0; ee < game.numcursors; ee++) 
    {
        game.mcurs[ee].hotx = adjust_pixel_size_for_loaded_data(game.mcurs[ee].hotx, filever);
        game.mcurs[ee].hoty = adjust_pixel_size_for_loaded_data(game.mcurs[ee].hoty, filever);
    }

    for (ee = 0; ee < game.numinvitems; ee++) 
    {
        adjust_pixel_sizes_for_loaded_data(&game.invinfo[ee].hotx, &game.invinfo[ee].hoty, filever);
    }

    for (ee = 0; ee < game.numgui; ee++) 
    {
        GUIMain*cgp=&guis[ee];
        adjust_pixel_sizes_for_loaded_data(&cgp->x, &cgp->y, filever);
        if (cgp->wid < 1)
            cgp->wid = 1;
        if (cgp->hit < 1)
            cgp->hit = 1;
        // Temp fix for older games
        if (cgp->wid == BASEWIDTH - 1)
            cgp->wid = BASEWIDTH;

        adjust_pixel_sizes_for_loaded_data(&cgp->wid, &cgp->hit, filever);

        cgp->popupyp = adjust_pixel_size_for_loaded_data(cgp->popupyp, filever);

        for (ff = 0; ff < cgp->numobjs; ff++) 
        {
            adjust_pixel_sizes_for_loaded_data(&cgp->objs[ff]->x, &cgp->objs[ff]->y, filever);
            adjust_pixel_sizes_for_loaded_data(&cgp->objs[ff]->wid, &cgp->objs[ff]->hit, filever);
            cgp->objs[ff]->activated=0;
        }
    }

    if ((filever >= 37) && (game.options[OPT_NATIVECOORDINATES] == 0) &&
        (game.default_resolution > 2))
    {
        // New 3.1 format game file, but with Use Native Coordinates off

        for (ee = 0; ee < game.numcharacters; ee++) 
        {
            game.chars[ee].x /= 2;
            game.chars[ee].y /= 2;
        }

        for (ee = 0; ee < numguiinv; ee++)
        {
            guiinv[ee].itemWidth /= 2;
            guiinv[ee].itemHeight /= 2;
        }
    }

}



int SaveScreenShot(char*namm) {
    char fileName[MAX_PATH];

    if (strchr(namm,'.') == NULL)
        sprintf(fileName, "%s%s.bmp", saveGameDirectory, namm);
    else
        sprintf(fileName, "%s%s", saveGameDirectory, namm);

    if (gfxDriver->RequiresFullRedrawEachFrame()) 
    {
        BITMAP *buffer = create_bitmap_ex(32, scrnwid, scrnhit);
        gfxDriver->GetCopyOfScreenIntoBitmap(buffer);

        if (save_bitmap(fileName, buffer, palette)!=0)
        {
            destroy_bitmap(buffer);
            return 0;
        }
        destroy_bitmap(buffer);
    }
    else if (save_bitmap(fileName, virtual_screen, palette)!=0)
        return 0; // failed

    return 1;  // successful
}



static void display_switch_out() {
    // this is only called if in SWITCH_PAUSE mode
    //debug_log("display_switch_out");

    switching_away_from_game++;

    platform->DisplaySwitchOut();

    // allow background running temporarily to halt the sound
    if (set_display_switch_mode(SWITCH_BACKGROUND) == -1)
        set_display_switch_mode(SWITCH_BACKAMNESIA);

    // stop the sound stuttering
    for (int i = 0; i <= MAX_SOUND_CHANNELS; i++) {
        if ((channels[i] != NULL) && (channels[i]->done == 0)) {
            channels[i]->pause();
        }
    }

    rest(1000);

    // restore the callbacks
    SetMultitasking(0);

    switching_away_from_game--;
}

static void display_switch_in() {
    for (int i = 0; i <= MAX_SOUND_CHANNELS; i++) {
        if ((channels[i] != NULL) && (channels[i]->done == 0)) {
            channels[i]->resume();
        }
    }

    // This can cause a segfault on Linux
#ifndef (LINUX_VERSION)
    if (gfxDriver->UsesMemoryBackBuffer())  // make sure all borders are cleared
        gfxDriver->ClearRectangle(0, 0, final_scrn_wid - 1, final_scrn_hit - 1, NULL);
#endif

    platform->DisplaySwitchIn();
}

void SetMultitasking (int mode) {
    if ((mode < 0) | (mode > 1))
        quit("!SetMultitasking: invalid mode parameter");

    // Don't allow background running if full screen
    if ((mode == 1) && (usetup.windowed == 0))
        mode = 0;

    if (mode == 0) {
        if (set_display_switch_mode(SWITCH_PAUSE) == -1)
            set_display_switch_mode(SWITCH_AMNESIA);
        // install callbacks to stop the sound when switching away
        set_display_switch_callback(SWITCH_IN, display_switch_in);
        set_display_switch_callback(SWITCH_OUT, display_switch_out);
    }
    else {
        if (set_display_switch_mode (SWITCH_BACKGROUND) == -1)
            set_display_switch_mode(SWITCH_BACKAMNESIA);
    }
}
