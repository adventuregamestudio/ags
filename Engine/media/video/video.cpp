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

#include "media/video/video.h"

#ifndef AGS_NO_VIDEO_PLAYER
#include <SDL.h>
#include "apeg.h"
#include "core/platform.h"
#include "debug/debug_log.h"
#include "debug/out.h"
#include "ac/asset_helper.h"
#include "ac/common.h"
#include "ac/draw.h"
#include "ac/game_version.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_display.h"
#include "ac/keycode.h"
#include "ac/mouse.h"
#include "ac/sys_events.h"
#include "ac/runtime_defines.h"
#include "ac/system.h"
#include "core/assetmanager.h"
#include "gfx/bitmap.h"
#include "gfx/ddb.h"
#include "gfx/graphicsdriver.h"
#include "main/game_run.h"
#include "util/stream.h"
#include "media/audio/audio_system.h"

using namespace AGS::Common;
using namespace AGS::Engine;


extern GameSetupStruct game;
extern IGraphicsDriver *gfxDriver;
extern int psp_video_framedrop;

enum VideoPlaybackType
{
    kVideoNone,
    kVideoFlic,
    kVideoTheora
};

VideoPlaybackType video_type = kVideoNone;

// FLIC player start
Bitmap *fli_buffer = nullptr;
short fliwidth,fliheight;
int canabort=0, stretch_flc = 1;
Bitmap *hicol_buf=nullptr;
IDriverDependantBitmap *fli_ddb = nullptr;
Bitmap *fli_target = nullptr;
int fliTargetWidth, fliTargetHeight;
volatile int fli_timer = 0; // TODO: use SDL thread conditions instead?
int check_if_user_input_should_cancel_video()
{
    KeyInput key;
    int mbut, mwheelz;
    if (run_service_key_controls(key)) {
        if ((key.Key==eAGSKeyCodeEscape) && (canabort==1))
            return 1;
        if (canabort >= 2)
            return 1;  // skip on any key
    }
    if (run_service_mb_controls(mbut, mwheelz) && mbut >= 0 && canabort == 3) {
        return 1; // skip on mouse click
    }
    return 0;
}

Uint32 fli_timer_callback(Uint32 interval, void *param)
{
    fli_timer++;
    return interval;
}

int fli_callback() {
    Bitmap *usebuf = fli_buffer;

    update_audio_system_on_game_loop ();

    if (game.color_depth > 1) {
        hicol_buf->Blit(fli_buffer,0,0,0,0,fliwidth,fliheight);
        usebuf=hicol_buf;
    }

    const Rect &view = play.GetMainViewport();
    if (stretch_flc == 0)
        fli_target->Blit(usebuf, 0,0, view.GetWidth()/2-fliwidth/2, view.GetHeight()/2-fliheight/2, view.GetWidth(), view.GetHeight());
    else 
        fli_target->StretchBlt(usebuf, RectWH(0,0,fliwidth,fliheight), RectWH(0,0, view.GetWidth(), view.GetHeight()));

    gfxDriver->UpdateDDBFromBitmap(fli_ddb, fli_target, false);
    gfxDriver->DrawSprite(0, 0, fli_ddb);
    render_to_screen();

    return check_if_user_input_should_cancel_video();
}

void play_flc_file(int numb,int playflags) {
    RGB oldpal[256];

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

    String flicname = String::FromFormat("flic%d.flc", numb);
    Stream *in = AssetMgr->OpenAsset(flicname);
    if (!in)
    {
        flicname.Format("flic%d.fli", numb);
        in = AssetMgr->OpenAsset(flicname);
    }
    if (!in)
    {
        debug_script_warn("FLIC animation flic%d.flc nor flic%d.fli not found", numb, numb);
        return;
    }

    in->Seek(8);
    fliwidth = in->ReadInt16();
    fliheight = in->ReadInt16();
    delete in;

    if (game.color_depth > 1) {
        hicol_buf=BitmapHelper::CreateBitmap(fliwidth,fliheight,game.GetColorDepth());
        hicol_buf->Clear();
    }
    // override the stretch option if necessary
    const Rect &view = play.GetMainViewport();
    if ((fliwidth == view.GetWidth()) && (fliheight == view.GetHeight()))
        stretch_flc = 0;
    else if ((fliwidth > view.GetWidth()) || (fliheight >view.GetHeight()))
        stretch_flc = 1;
    fli_buffer=BitmapHelper::CreateBitmap(fliwidth,fliheight,8);
    if (fli_buffer==nullptr) quit("Not enough memory to play animation");
    fli_buffer->Clear();

    if (clearScreenAtStart)
    {
        if (gfxDriver->UsesMemoryBackBuffer())
        {
            Bitmap *screen_bmp = gfxDriver->GetMemoryBackBuffer();
            screen_bmp->Clear();
        }
        render_to_screen();
    }

    video_type = kVideoFlic;
    fli_target = BitmapHelper::CreateBitmap(view.GetWidth(), view.GetHeight(), game.GetColorDepth());
    fli_ddb = gfxDriver->CreateDDBFromBitmap(fli_target, false, true);

    size_t asset_size;
    PACKFILE *pf = PackfileFromAsset(AssetPath("", flicname), asset_size);
    if (open_fli_pf(pf) == FLI_OK)
    {
        // TODO: refactor all this later!!!
        SDL_AddTimer(fli_speed, fli_timer_callback, nullptr);
        const int loop = 0; // TODO: add looping FLIC support to API?

        // actual FLI playback state, base on original Allegro 4's do_play_fli
        fli_timer = 1;
        int ret = next_fli_frame(loop);
        while (ret == FLI_OK) {
            /* update the palette */
            if (fli_pal_dirty_from <= fli_pal_dirty_to)
                set_palette_range(fli_palette, fli_pal_dirty_from, fli_pal_dirty_to, TRUE);

            /* update the screen */
            if (fli_bmp_dirty_from <= fli_bmp_dirty_to) {
                blit(fli_bitmap, fli_buffer->GetAllegroBitmap(), 0, fli_bmp_dirty_from, 0, fli_bmp_dirty_from,
                    fli_bitmap->w, 1 + fli_bmp_dirty_to - fli_bmp_dirty_from);
            }

            reset_fli_variables();

            ret = fli_callback();
            if (ret != FLI_OK)
                break;

            ret = next_fli_frame(loop);
            fli_timer--;

            while (fli_timer <= 0) {
                /* wait a bit */
                SDL_Delay(1);
            }
        }
    }
    else
    {
        // This is not a fatal error that should prevent the game from continuing
        Debug::Printf("FLI/FLC animation play error");
    }
    pack_fclose(pf);
    

    video_type = kVideoNone;
    delete fli_buffer;
    fli_buffer = nullptr;
    // NOTE: the screen bitmap could change in the meanwhile, if the display mode has changed
    if (gfxDriver->UsesMemoryBackBuffer())
    {
        Bitmap *screen_bmp = gfxDriver->GetMemoryBackBuffer();
        screen_bmp->Clear();
    }
    set_palette_range(oldpal, 0, 255, 0);
    render_to_screen();

    delete fli_target;
    gfxDriver->DestroyDDB(fli_ddb);
    fli_target = nullptr;
    fli_ddb = nullptr;


    delete hicol_buf;
    hicol_buf=nullptr;
    //  SetVirtualScreen(screen); wputblock(0,0,backbuffer,0);
    while (ags_mgetbutton()!= MouseNone) { } // clear any queued mouse events.
    invalidate_screen();
}

// FLIC player end

// Theora player begin
// TODO: find a way to take Bitmap here?
Bitmap gl_TheoraBuffer;
int theora_playing_callback(BITMAP *theoraBuffer)
{
    sys_evt_process_pending();
	if (theoraBuffer == nullptr)
    {
        // No video, only sound
        return check_if_user_input_should_cancel_video();
    }

    gl_TheoraBuffer.WrapAllegroBitmap(theoraBuffer, true);

    int drawAtX = 0, drawAtY = 0;
    const Rect &viewport = play.GetMainViewport();
    if (fli_ddb == nullptr)
    {
        fli_ddb = gfxDriver->CreateDDBFromBitmap(&gl_TheoraBuffer, false, true);
    }
    if (stretch_flc) 
    {
        drawAtX = viewport.GetWidth() / 2 - fliTargetWidth / 2;
        drawAtY = viewport.GetHeight() / 2 - fliTargetHeight / 2;
        if (!gfxDriver->HasAcceleratedTransform())
        {
            fli_target->StretchBlt(&gl_TheoraBuffer, RectWH(0, 0, gl_TheoraBuffer.GetWidth(), gl_TheoraBuffer.GetHeight()), 
                RectWH(drawAtX, drawAtY, fliTargetWidth, fliTargetHeight));
            gfxDriver->UpdateDDBFromBitmap(fli_ddb, fli_target, false);
            drawAtX = 0;
            drawAtY = 0;
        }
        else
        {
            gfxDriver->UpdateDDBFromBitmap(fli_ddb, &gl_TheoraBuffer, false);
            fli_ddb->SetStretch(fliTargetWidth, fliTargetHeight, false);
        }
    }
    else
    {
        gfxDriver->UpdateDDBFromBitmap(fli_ddb, &gl_TheoraBuffer, false);
        drawAtX = viewport.GetWidth() / 2 - gl_TheoraBuffer.GetWidth() / 2;
        drawAtY = viewport.GetHeight() / 2 - gl_TheoraBuffer.GetHeight() / 2;
    }

    gfxDriver->DrawSprite(drawAtX, drawAtY, fli_ddb);
    update_audio_system_on_game_loop ();
    render_to_screen();

    return check_if_user_input_should_cancel_video();
}

//
// Theora stream reader callbacks. We need these because APEG library does not
// provide means to supply user's PACKFILE directly.
//
// Open stream for reading (return suggested cache buffer size).
int apeg_stream_init(void *ptr)
{
    return ptr != nullptr ? F_BUF_SIZE : 0;
}
// Read requested number of bytes into provided buffer,
// return actual number of bytes managed to read.
int apeg_stream_read(void *buffer, int bytes, void *ptr)
{
    return ((Stream*)ptr)->Read(buffer, bytes);
}
// Skip requested number of bytes
void apeg_stream_skip(int bytes, void *ptr)
{
    ((Stream*)ptr)->Seek(bytes);
}
//

APEG_STREAM* get_theora_size(Stream *video_stream, int *width, int *height)
{
    APEG_STREAM* oggVid = apeg_open_stream_ex(video_stream);
    if (oggVid != nullptr)
    {
        apeg_get_video_size(oggVid, width, height);
    }
    else
    {
        *width = 0;
        *height = 0;
    }
    return oggVid;
}

// TODO: use shared utility function for placing rect in rect
void calculate_destination_size_maintain_aspect_ratio(int vidWidth, int vidHeight, int *targetWidth, int *targetHeight)
{
    const Rect &viewport = play.GetMainViewport();
    float aspectRatioVideo = (float)vidWidth / (float)vidHeight;
    float aspectRatioScreen = (float)viewport.GetWidth() / (float)viewport.GetHeight();

    if (aspectRatioVideo == aspectRatioScreen)
    {
        *targetWidth = viewport.GetWidth();
        *targetHeight = viewport.GetHeight();
    }
    else if (aspectRatioVideo > aspectRatioScreen)
    {
        *targetWidth = viewport.GetWidth();
        *targetHeight = (int)(((float)viewport.GetWidth() / aspectRatioVideo) + 0.5f);
    }
    else
    {
        *targetHeight = viewport.GetHeight();
        *targetWidth = (float)viewport.GetHeight() * aspectRatioVideo;
    }

}

void play_theora_video(const char *name, int skip, int flags)
{
    std::unique_ptr<Stream> video_stream(AssetMgr->OpenAsset(name));
    apeg_set_stream_reader(apeg_stream_init, apeg_stream_read, apeg_stream_skip);
    apeg_set_display_depth(game.GetColorDepth());
    // we must disable length detection, otherwise it takes ages to start
    // playing if the file is large because it seeks through the whole thing
    apeg_disable_length_detection(TRUE);
    // Disable framedrop because it can lead to the PSP not playing the video at all.
    apeg_enable_framedrop(psp_video_framedrop);
    update_polled_stuff_if_runtime();

    stretch_flc = (flags % 10);
    canabort = skip;
    apeg_ignore_audio((flags >= 10) ? 1 : 0);

    int videoWidth, videoHeight;
    APEG_STREAM *oggVid = get_theora_size(video_stream.get(), &videoWidth, &videoHeight);

    if (videoWidth == 0)
    {
        Display("Unable to load theora video '%s'", name);
        return;
    }

    if (flags < 10)
    {
        stop_all_sound_and_music();
    }

    calculate_destination_size_maintain_aspect_ratio(videoWidth, videoHeight, &fliTargetWidth, &fliTargetHeight);

    if ((fliTargetWidth == videoWidth) && (fliTargetHeight == videoHeight) && (stretch_flc))
    {
        // don't need to stretch after all
        stretch_flc = 0;
    }

    if ((stretch_flc) && (!gfxDriver->HasAcceleratedTransform()))
    {
        fli_target = BitmapHelper::CreateBitmap(play.GetMainViewport().GetWidth(), play.GetMainViewport().GetHeight(), game.GetColorDepth());
        fli_target->Clear();
        fli_ddb = gfxDriver->CreateDDBFromBitmap(fli_target, false, true);
    }
    else
    {
        fli_ddb = nullptr;
    }

    update_polled_stuff_if_runtime();

    if (gfxDriver->UsesMemoryBackBuffer())
        gfxDriver->GetMemoryBackBuffer()->Clear();

    video_type = kVideoTheora;
    if (apeg_play_apeg_stream(oggVid, nullptr, 0, theora_playing_callback) == APEG_ERROR)
    {
        Display("Error playing theora video '%s'", name);
    }
    apeg_close_stream(oggVid);
    video_type = kVideoNone;

    //destroy_bitmap(fli_buffer);
    delete fli_target;
    gfxDriver->DestroyDDB(fli_ddb);
    fli_target = nullptr;
    fli_ddb = nullptr;
    invalidate_screen();
}
// Theora player end

void video_on_gfxmode_changed()
{
    if (video_type == kVideoFlic)
    {
        // If the FLIC video is playing, restore its palette
        set_palette_range(fli_palette, 0, 255, 0);
    }
}

#else

void play_theora_video(const char *name, int skip, int flags) {}
void play_flc_file(int numb,int playflags) {}
void video_on_gfxmode_changed() {}

#endif