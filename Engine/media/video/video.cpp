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

#include "video.h"
#include "apeg.h"
#include "debug/debug_log.h"
#include "debug/out.h"
#include "ac/asset_helper.h"
#include "ac/common.h"
#include "ac/draw.h"
#include "ac/game_version.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_display.h"
#include "ac/mouse.h"
#include "ac/record.h"
#include "ac/runtime_defines.h"
#include "ac/system.h"
#include "core/assetmanager.h"
#include "gfx/bitmap.h"
#include "gfx/ddb.h"
#include "gfx/graphicsdriver.h"
#include "main/game_run.h"
#include "media/audio/audio.h"
#include "util/stream.h"

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
Bitmap *fli_buffer = NULL;
short fliwidth,fliheight;
int canabort=0, stretch_flc = 1;
Bitmap *hicol_buf=NULL;
IDriverDependantBitmap *fli_ddb = NULL;
Bitmap *fli_target = NULL;
int fliTargetWidth, fliTargetHeight;
int check_if_user_input_should_cancel_video()
{
    int key;
    if (run_service_key_controls(key)) {
        if ((key==27) && (canabort==1))
            return 1;
        if (canabort >= 2)
            return 1;  // skip on any key
    }
    if (canabort == 3) {  // skip on mouse click
        if (mgetbutton()!=NONE) return 1;
    }
    return 0;
}

#if defined(WINDOWS_VERSION)
int __cdecl fli_callback() {
#else
extern "C" int fli_callback() {
#endif
    Bitmap *usebuf = fli_buffer;

    update_polled_audio_and_crossfade ();

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

    String flicname = String::FromFormat("flic%d.flc", numb);
    Stream *in = AssetManager::OpenAsset(flicname);
    if (!in)
    {
        flicname.Format("flic%d.fli", numb);
        in = AssetManager::OpenAsset(flicname);
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
    if (fli_buffer==NULL) quit("Not enough memory to play animation");
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

    // TODO: find a better solution.
    // Make only certain versions of the engineuse play_fli_pf from the patched version of Allegro for now.
    // Add more versions as their Allegro lib becomes patched too, or they use newer version of Allegro 4.
    // Ports can still play FLI if separate file is put into game's directory.
#if defined (WINDOWS_VERSION) || defined (ANDROID_VERSION)
    PACKFILE *pf = PackfileFromAsset(AssetPath("", flicname));
    if (play_fli_pf(pf, (BITMAP*)fli_buffer->GetAllegroBitmap(), fli_callback)==FLI_ERROR)
#else
    if (play_fli(flicname, (BITMAP*)fli_buffer->GetAllegroBitmap(), 0, fli_callback)==FLI_ERROR)
#endif
    {
        // This is not a fatal error that should prevent the game from continuing
        Debug::Printf("FLI/FLC animation play error");
    }
#if defined WINDOWS_VERSION
    pack_fclose(pf);
#endif

    video_type = kVideoNone;
    delete fli_buffer;
    fli_buffer = NULL;
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
    fli_target = NULL;
    fli_ddb = NULL;


    delete hicol_buf;
    hicol_buf=NULL;
    //  SetVirtualScreen(screen); wputblock(0,0,backbuffer,0);
    while (mgetbutton()!=NONE) ;
    invalidate_screen();
}

// FLIC player end

// Theora player begin
// TODO: find a way to take Bitmap here?
Bitmap gl_TheoraBuffer;
int theora_playing_callback(BITMAP *theoraBuffer)
{
	if (theoraBuffer == NULL)
    {
        // No video, only sound
        return check_if_user_input_should_cancel_video();
    }

    gl_TheoraBuffer.WrapAllegroBitmap(theoraBuffer, true);

    int drawAtX = 0, drawAtY = 0;
    const Rect &viewport = play.GetMainViewport();
    if (fli_ddb == NULL)
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
    render_to_screen();
    update_polled_audio_and_crossfade ();

    return check_if_user_input_should_cancel_video();
}

//
// Theora stream reader callbacks. We need these since we are creating PACKFILE
// stream by our own rules, and APEG library does not provide means to supply
// user's PACKFILE.
//
// Ironically, internally those callbacks will become a part of another
// PACKFILE's vtable, of APEG's library, so that will be a PACKFILE reading
// data using proxy PACKFILE through callback system...
//
class ApegStreamReader
{
public:
    ApegStreamReader(const AssetPath path) : _path(path), _pf(NULL) {}
    ~ApegStreamReader() { Close(); }

    bool Open()
    {
        Close(); // PACKFILE cannot be rewinded, sadly
        _pf = PackfileFromAsset(_path);
        return _pf != NULL;
    }

    void Close()
    {
        if (_pf)
            pack_fclose(_pf);
        _pf = NULL;
    }

    int Read(void *buffer, int bytes)
    {
        return _pf ? pack_fread(buffer, bytes, _pf) : 0;
    }

    void Skip(int bytes)
    {
        if (_pf)
            pack_fseek(_pf, bytes);
    }

private:
    AssetPath _path; // path to the asset
    PACKFILE *_pf;   // our stream
};

// Open stream for reading (return suggested cache buffer size).
int apeg_stream_init(void *ptr)
{
    return ((ApegStreamReader*)ptr)->Open() ? F_BUF_SIZE : 0;
}

// Read requested number of bytes into provided buffer,
// return actual number of bytes managed to read.
int apeg_stream_read(void *buffer, int bytes, void *ptr)
{
    return ((ApegStreamReader*)ptr)->Read(buffer, bytes);
}

// Skip requested number of bytes
void apeg_stream_skip(int bytes, void *ptr)
{
    ((ApegStreamReader*)ptr)->Skip(bytes);
}

APEG_STREAM* get_theora_size(ApegStreamReader &reader, int *width, int *height)
{
    APEG_STREAM* oggVid = apeg_open_stream_ex(&reader);
    if (oggVid != NULL)
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
    ApegStreamReader reader(AssetPath("", name));
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
    APEG_STREAM *oggVid = get_theora_size(reader, &videoWidth, &videoHeight);

    if (videoWidth == 0)
    {
        Display("Unable to load theora video '%s'", name);
        return;
    }

    if (flags < 10)
    {
        stop_all_sound_and_music();
    }

    //fli_buffer = BitmapHelper::CreateBitmap_(scsystem.coldepth, videoWidth, videoHeight);
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
        fli_ddb = NULL;
    }

    update_polled_stuff_if_runtime();

    if (gfxDriver->UsesMemoryBackBuffer())
        gfxDriver->GetMemoryBackBuffer()->Clear();

    video_type = kVideoTheora;
    if (apeg_play_apeg_stream(oggVid, NULL, 0, theora_playing_callback) == APEG_ERROR)
    {
        Display("Error playing theora video '%s'", name);
    }
    apeg_close_stream(oggVid);
    video_type = kVideoNone;

    //destroy_bitmap(fli_buffer);
    delete fli_target;
    gfxDriver->DestroyDDB(fli_ddb);
    fli_target = NULL;
    fli_ddb = NULL;
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
