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

#define USE_CLIB
#include <stdio.h>
#include "ac/global_video.h"
#include "gfx/ali3d.h"
#include "ac/common.h"
#include "ac/draw.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_game.h"
#include "ac/mouse.h"
#include "ac/record.h"
#include "debug/debug_log.h"
#include "debug/debugger.h"
#include "debug/out.h"
#include "media/video/video.h"
#include "util/stream.h"
#include "gfx/graphicsdriver.h"
#include "gfx/bitmap.h"
#include "core/assetmanager.h"

using AGS::Common::Stream;

using AGS::Common::Bitmap;
namespace BitmapHelper = AGS::Common::BitmapHelper;
namespace Out = AGS::Common::Out;

extern GameSetup usetup;
extern GameSetupStruct game;
extern GameState play;

// defined in media/video/video.h
extern int canabort, stretch_flc;
extern short fliwidth,fliheight;
extern Bitmap *hicol_buf;
extern Bitmap *fli_buffer;
extern IDriverDependantBitmap *fli_ddb;
extern Bitmap *fli_target;
extern IGraphicsDriver *gfxDriver;

// defined in ac_screen
extern int final_scrn_wid,final_scrn_hit,final_col_dep;
extern int scrnwid,scrnhit;

void play_flc_file(int numb,int playflags) {
    color oldpal[256];

    // AGS 2.x: If the screen is faded out, fade in again when playing a movie.
    if (loaded_game_file_version <= kGameVersion_272)
        play.screen_is_faded_out = 0;

    if (play.fast_forward)
        return;

    get_palette_range(oldpal, 0, 255);

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
    Stream*in=Common::AssetManager::OpenAsset(flicnam);
    if (in==NULL) { sprintf(flicnam,"flic%d.fli",numb);
    in=Common::AssetManager::OpenAsset(flicnam); }
    if (in==NULL) {
        debug_log("FLIC animation FLIC%d.FLC not found",numb);
        return;
    }
    in->Seek(Common::kSeekCurrent,8);
    fliwidth = in->ReadInt16();
    fliheight = in->ReadInt16();
    delete in;
    if (game.color_depth > 1) {
        hicol_buf=BitmapHelper::CreateBitmap(fliwidth,fliheight,final_col_dep);
        hicol_buf->Clear();
    }
    // override the stretch option if necessary
    if ((fliwidth==scrnwid) && (fliheight==scrnhit))
        stretch_flc = 0;
    else if ((fliwidth > scrnwid) || (fliheight > scrnhit))
        stretch_flc = 1;
    fli_buffer=BitmapHelper::CreateBitmap(fliwidth,fliheight,8); //640,400); //scrnwid,scrnhit);
    if (fli_buffer==NULL) quit("Not enough memory to play animation");
    fli_buffer->Clear();

    Bitmap *screen_bmp = BitmapHelper::GetScreenBitmap();

    if (clearScreenAtStart) {
        screen_bmp->Clear();
        render_to_screen(screen_bmp, 0, 0);
    }

    fli_target = BitmapHelper::CreateBitmap(screen_bmp->GetWidth(), screen_bmp->GetHeight(), final_col_dep);
    fli_ddb = gfxDriver->CreateDDBFromBitmap(fli_target, false, true);

    if (play_fli(flicnam,(BITMAP*)fli_buffer->GetAllegroBitmap(),0,fli_callback)==FLI_ERROR)
    {
        // This is not a fatal error that should prevent the game from continuing
        //quit("FLI/FLC animation play error");
        Out::FPrint("FLI/FLC animation play error");
    }

    delete fli_buffer;
    screen_bmp->Clear();
    set_palette_range(oldpal, 0, 255, 0);
    render_to_screen(screen_bmp, 0, 0);

    delete fli_target;
    gfxDriver->DestroyDDB(fli_ddb);
    fli_ddb = NULL;


    delete hicol_buf;
    hicol_buf=NULL;
    //  SetVirtualScreen(screen); wputblock(0,0,backbuffer,0);
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
