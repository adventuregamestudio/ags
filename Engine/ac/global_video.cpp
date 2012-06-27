#define USE_CLIB
#include "ac/global_video.h"
#include "wgt2allg.h"
#include "acmain/ac_maindefines.h"
#include "ac/ac_common.h"
#include "ac/ac_gamesetupstruct.h"
#include "debug/debug.h"
#include "acmain/ac_cutscene.h"
#include "acmain/ac_draw.h"
#include "acmain/ac_mouse.h"
#include "acmain/ac_record.h"
#include "ac/gamestate.h"
#include "ac/gamesetup.h"
#include "media/video/video.h"


extern int loaded_game_file_version;
extern GameSetup usetup;
extern GameSetupStruct game;
extern GameState play;

// defined in media/video/video.h
extern int canabort, stretch_flc;
extern short fliwidth,fliheight;
extern block hicol_buf;
extern block fli_buffer;
extern IDriverDependantBitmap *fli_ddb;
extern BITMAP *fli_target;

// defined in ac_screen
extern int final_scrn_wid,final_scrn_hit,final_col_dep;
extern int scrnwid,scrnhit;

void play_flc_file(int numb,int playflags) {
    color oldpal[256];

    // AGS 2.x: If the screen is faded out, fade in again when playing a movie.
    if (loaded_game_file_version <= 32)
        play.screen_is_faded_out = 0;

    if (play.fast_forward)
        return;

    wreadpalette(0,255,oldpal);

    int clearScreenAtStart = 1;
    canabort = playflags % 10;
    playflags -= canabort;

    if (canabort == 2) // convert to PlayVideo-compatible setting
        canabort = 3;

    if (playflags % 100 == 0)
        stretch_flc = 1;
    else
        stretch_flc = 0;

    if (playflags / 100)
        clearScreenAtStart = 0;

    char flicnam[20]; sprintf(flicnam,"flic%d.flc",numb);
    FILE*iii=clibfopen(flicnam,"rb");
    if (iii==NULL) { sprintf(flicnam,"flic%d.fli",numb);
    iii=clibfopen(flicnam,"rb"); }
    if (iii==NULL) {
        debug_log("FLIC animation FLIC%d.FLC not found",numb);
        return;
    }
    fseek(iii,8,SEEK_CUR);
    fread(&fliwidth,2,1,iii);
    fread(&fliheight,2,1,iii);
    fclose(iii);
    if (game.color_depth > 1) {
        hicol_buf=create_bitmap_ex(final_col_dep,fliwidth,fliheight);
        clear(hicol_buf);
    }
    // override the stretch option if necessary
    if ((fliwidth==scrnwid) && (fliheight==scrnhit))
        stretch_flc = 0;
    else if ((fliwidth > scrnwid) || (fliheight > scrnhit))
        stretch_flc = 1;
    fli_buffer=create_bitmap_ex(8,fliwidth,fliheight); //640,400); //scrnwid,scrnhit);
    if (fli_buffer==NULL) quit("Not enough memory to play animation");
    clear(fli_buffer);

    if (clearScreenAtStart) {
        clear(screen);
        render_to_screen(screen, 0, 0);
    }

    fli_target = create_bitmap_ex(final_col_dep, screen->w, screen->h);
    fli_ddb = gfxDriver->CreateDDBFromBitmap(fli_target, false, true);

    if (play_fli(flicnam,fli_buffer,0,fli_callback)==FLI_ERROR)
        quit("FLI/FLC animation play error");

    wfreeblock(fli_buffer);
    clear(screen);
    wsetpalette(0,255,oldpal);
    render_to_screen(screen, 0, 0);

    destroy_bitmap(fli_target);
    gfxDriver->DestroyDDB(fli_ddb);
    fli_ddb = NULL;

    if (hicol_buf!=NULL) {
        wfreeblock(hicol_buf);
        hicol_buf=NULL; }
    //  wsetscreen(screen); wputblock(0,0,backbuffer,0);
    while (mgetbutton()!=NONE) ;
    invalidate_screen();
}

void scrPlayVideo(const char* name, int skip, int flags) {
    EndSkippingUntilCharStops();

    if (play.fast_forward)
        return;
    if (debug_flags & DBG_NOVIDEO)
        return;

    if ((flags < 10) && (usetup.digicard == DIGI_NONE)) {
        // if game audio is disabled in Setup, then don't
        // play any sound on the video either
        flags += 10;
    }

    pause_sound_if_necessary_and_play_video(name, skip, flags);
}
