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
#include "media/audio/openal.h"

using namespace AGS::Common;
using namespace AGS::Engine;


extern GameSetupStruct game;
extern IGraphicsDriver *gfxDriver;
extern int psp_video_framedrop;


//-----------------------------------------------------------------------------
// VideoPlayer
//-----------------------------------------------------------------------------
namespace AGS
{
namespace Engine
{

VideoPlayer::~VideoPlayer()
{
    Close();
}

bool VideoPlayer::Open(const String &name, int skip, int flags)
{
    int clearScreenAtStart = 1;
    _stretchVideo = (flags % 100) == 0;

    if (flags / 100)
        clearScreenAtStart = 0;

    if (!OpenImpl(name, flags))
        return false;

    /// AUDIO
    /* Start the audio stream */
    if ((_audioFormat > 0) && (_audioChannels > 0) && (_audioFreq > 0))
    {
        _audioOut.reset(new OpenAlSource(_audioFormat, _audioChannels, _audioFreq));
        _audioOut->Play();
        _wantAudio = true;
    }
    ///

    _flags = flags;
    _skip = skip;
    /// THEORA
    if (flags < 10)
    {
        stop_all_sound_and_music();
    }
    /// THEORA

    // TODO: needed for FLIC, but perhaps may be done differently
    if (clearScreenAtStart)
    {
        if (gfxDriver->UsesMemoryBackBuffer())
        {
            Bitmap *screen_bmp = gfxDriver->GetMemoryBackBuffer();
            screen_bmp->Clear();
        }
        render_to_screen();
    }

    _timerPos = 1;
    _sdlTimer = SDL_AddTimer(_frameTime, VideoPlayer::VideoTimerCallback, this);
    _loop = false;
    return true;
}

void VideoPlayer::Close()
{
    // Stop playback timer
    SDL_RemoveTimer(_sdlTimer);
    // Shutdown openal source
    _audioOut.reset();
    // Close video decoder and free resources
    CloseImpl();

    _videoFrame.reset();
    _hicolBuf.reset();
    _targetBitmap.reset();
    if (_videoDDB)
        gfxDriver->DestroyDDB(_videoDDB);
    _videoDDB = nullptr;

    // TODO: needed for FLIC, but perhaps may be done differently
    if (gfxDriver->UsesMemoryBackBuffer())
    {
        Bitmap *screen_bmp = gfxDriver->GetMemoryBackBuffer();
        screen_bmp->Clear();
    }
    render_to_screen();

    ags_clear_input_state();
    invalidate_screen();
}

int VideoPlayer::GetAudioPos()
{
    return _audioOut ? _audioOut->GetPositionMs() : 0;
}

bool VideoPlayer::Poll()
{
    // Acquire next video frame
    if (!NextFrame())
        return false;
    // Render current frame
    if (_audioFrame && !RenderAudio())
        return false;
    if (_videoFrame && !RenderVideo())
        return false;
    // Check user input skipping the video
    if (CheckUserInputSkip())
        return false;
    // Wait for timer
    _timerPos--;
    while (_timerPos <= 0) {
        SDL_Delay(1);
    }
    return true;
}

uint32_t VideoPlayer::VideoTimerCallback(uint32_t interval, void *param)
{
    auto player = reinterpret_cast<VideoPlayer*>(param);
    player->_timerPos++;
    return interval;
}

bool VideoPlayer::CheckUserInputSkip()
{
    KeyInput key;
    int mbut, mwheelz;
    if (run_service_key_controls(key))
    {
        if ((key.Key == eAGSKeyCodeEscape) && (_skip == 1))
            return true;
        if (_skip >= 2)
            return true;  // skip on any key
    }
    if (run_service_mb_controls(mbut, mwheelz) && mbut >= 0 && _skip == 3)
        return true; // skip on mouse click
    return false;
}

bool VideoPlayer::RenderAudio()
{
    assert(_audioFrame);
    assert(_audioOut != nullptr);
    _wantAudio = _audioOut->PutData(_audioFrame) > 0u;
    _audioOut->Poll();
    return true;
}

bool VideoPlayer::RenderVideo()
{
    assert(_videoFrame);
    Bitmap *usebuf = _videoFrame.get();

    // FIXME: for FLIC only!! do we need this always, or only when stretched, or at all??
    // FIXME: test target bitmap, not game's depth?
    if ((game.color_depth > 1) && (usebuf->GetBPP() == 1))
    {
        _hicolBuf->Blit(usebuf);
        usebuf = _hicolBuf.get();
    }

    // FIXME: create on Open?
    if (!_videoDDB)
    {
        _videoDDB = gfxDriver->CreateDDBFromBitmap(usebuf, false, true);
    }

    int drawAtX = 0, drawAtY = 0;
    const Rect &view = play.GetMainViewport();
    if (!_stretchVideo)
    {
        drawAtX = view.GetWidth() / 2 - _targetSize.Width / 2;
        drawAtY = view.GetHeight() / 2 - _targetSize.Height / 2;

        if (!gfxDriver->HasAcceleratedTransform())
        {
            _targetBitmap->StretchBlt(usebuf, RectWH(0, 0, usebuf->GetWidth(), usebuf->GetHeight()),
                RectWH(drawAtX, drawAtY, _targetSize.Width, _targetSize.Height));
            gfxDriver->UpdateDDBFromBitmap(_videoDDB, _targetBitmap.get(), false);
            drawAtX = 0;
            drawAtY = 0;
        }
        else
        {
            gfxDriver->UpdateDDBFromBitmap(_videoDDB, usebuf, false);
            _videoDDB->SetStretch(_targetSize.Width, _targetSize.Height, false);
        }

        /* FROM FLIC:
        fli_target->Blit(usebuf, 0, 0, view.GetWidth() / 2 - fliwidth / 2, view.GetHeight() / 2 - fliheight / 2, view.GetWidth(), view.GetHeight());
        */
    }
    else
    {
        gfxDriver->UpdateDDBFromBitmap(_videoDDB, usebuf, false);
        drawAtX = view.GetWidth() / 2 - usebuf->GetWidth() / 2;
        drawAtY = view.GetHeight() / 2 - usebuf->GetHeight() / 2;
        /* FROM FLIC:
        fli_target->StretchBlt(usebuf, RectWH(0, 0, fliwidth, fliheight), RectWH(0, 0, view.GetWidth(), view.GetHeight()));
        */
    }

    /* FROM FLIC:
    gfxDriver->UpdateDDBFromBitmap(fli_ddb, fli_target, false);
    */
    gfxDriver->DrawSprite(drawAtX, drawAtY, _videoDDB);
    render_to_screen();
    return true;
}

std::unique_ptr<VideoPlayer> gl_Video;

//-----------------------------------------------------------------------------
// FLIC video player implementation
//-----------------------------------------------------------------------------

class FlicPlayer : public VideoPlayer
{
public:
    FlicPlayer() = default;
    ~FlicPlayer();

    void Restore() override;

private:
    bool OpenImpl(const AGS::Common::String &name, int flags) override;
    void CloseImpl() override;
    bool NextFrame() override;

    PACKFILE *_pf = nullptr;
    RGB _oldpal[256]{};
};

FlicPlayer::~FlicPlayer()
{
    CloseImpl();
}

void FlicPlayer::Restore()
{
    // If the FLIC video is playing, restore its palette
    set_palette_range(fli_palette, 0, 255, 0);
}

bool FlicPlayer::OpenImpl(const AGS::Common::String &name, int /* flags */)
{
    Stream *in = AssetMgr->OpenAsset(name);
    if (!in)
        return false;
    in->Seek(8);
    const int fliwidth = in->ReadInt16();
    const int fliheight = in->ReadInt16();
    delete in;

    PACKFILE *pf = PackfileFromAsset(AssetPath(name, "*"));
    if (open_fli_pf(pf) != FLI_OK)
    {
        pack_fclose(pf);
        // This is not a fatal error that should prevent the game from continuing
        Debug::Printf("FLI/FLC animation play error");
        return false;
    }
    _pf = pf;

    get_palette_range(_oldpal, 0, 255);

    if (game.color_depth > 1)
    {
        _hicolBuf.reset(BitmapHelper::CreateClearBitmap(fliwidth, fliheight, game.GetColorDepth()));
    }
    // override the stretch option if necessary
    const Rect &view = play.GetMainViewport();
    if ((fliwidth == view.GetWidth()) && (fliheight == view.GetHeight()))
        _stretchVideo = false;
    else if ((fliwidth > view.GetWidth()) || (fliheight >view.GetHeight()))
        _stretchVideo = true;
    _videoFrame.reset(BitmapHelper::CreateClearBitmap(fliwidth, fliheight, 8));

    _targetBitmap.reset(BitmapHelper::CreateBitmap(view.GetWidth(), view.GetHeight(), game.GetColorDepth()));
    _videoDDB = gfxDriver->CreateDDBFromBitmap(_targetBitmap.get(), false, true);
    _targetSize = view.GetSize();
    _frameTime = fli_speed;
    return true;
}

void FlicPlayer::CloseImpl()
{
    if (_pf)
        pack_fclose(_pf);
    _pf = nullptr;

    set_palette_range(_oldpal, 0, 255, 0);
}

bool FlicPlayer::NextFrame()
{
    // actual FLI playback state, base on original Allegro 4's do_play_fli

    /* get next frame */
    if (next_fli_frame(IsLooping() ? 1 : 0) != FLI_OK)
        return false;

    /* update the palette */
    if (fli_pal_dirty_from <= fli_pal_dirty_to)
        set_palette_range(fli_palette, fli_pal_dirty_from, fli_pal_dirty_to, TRUE);

    /* update the screen */
    if (fli_bmp_dirty_from <= fli_bmp_dirty_to) {
        blit(fli_bitmap, _videoFrame->GetAllegroBitmap(), 0, fli_bmp_dirty_from, 0, fli_bmp_dirty_from,
            fli_bitmap->w, 1 + fli_bmp_dirty_to - fli_bmp_dirty_from);
    }

    reset_fli_variables();
    return true;
}

//-----------------------------------------------------------------------------
// Theora video player implementation
//-----------------------------------------------------------------------------

class TheoraPlayer : public VideoPlayer
{
public:
    TheoraPlayer() = default;
    ~TheoraPlayer();

private:
    bool OpenImpl(const AGS::Common::String &name, int flags) override;
    void CloseImpl() override;
    bool NextFrame() override;

    std::unique_ptr<Stream> _dataStream;
    APEG_STREAM *_apegStream = nullptr;
};

TheoraPlayer::~TheoraPlayer()
{
    CloseImpl();
}

//
// Theora stream reader callbacks. We need these because APEG library does not
// provide means to supply user's PACKFILE directly.
//
// Open stream for reading (return suggested cache buffer size).
int apeg_stream_init(void *ptr)
{
    if (!ptr) return 0;
    ((Stream*)ptr)->Seek(0, kSeekBegin);
    return F_BUF_SIZE;
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


// TODO: use shared utility function for placing rect in rect
static void calculate_destination_size_maintain_aspect_ratio(int vidWidth, int vidHeight, int *targetWidth, int *targetHeight)
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

bool TheoraPlayer::OpenImpl(const AGS::Common::String &name, int flags)
{
    std::unique_ptr<Stream> video_stream(AssetMgr->OpenAsset(name));
    apeg_set_stream_reader(apeg_stream_init, apeg_stream_read, apeg_stream_skip);
    apeg_set_display_depth(game.GetColorDepth());
    // we must disable length detection, otherwise it takes ages to start
    // playing if the file is large because it seeks through the whole thing
    apeg_disable_length_detection(TRUE);
    // Disable framedrop, because after porting to SDL2 and OpenAL, APEG detects
    // audio ahead too often, and with framedrop video does not advance at all.
    apeg_enable_framedrop(/*psp_video_framedrop*/FALSE);
    apeg_ignore_audio((flags >= 10) ? 1 : 0);

    APEG_STREAM* apeg_stream = apeg_open_stream_ex(video_stream.get());
    if (!apeg_stream)
    {
        debug_script_warn("Unable to load theora video '%s'", name.GetCStr());
        return false;
    }
    int video_w, video_h;
    apeg_get_video_size(apeg_stream, &video_w, &video_h);
    if (video_w <= 0 || video_h <= 0)
    {
        debug_script_warn("Unable to load theora video '%s'", name.GetCStr());
        return false;
    }

    _apegStream = apeg_stream;
    calculate_destination_size_maintain_aspect_ratio(video_w, video_h, &_targetSize.Width, &_targetSize.Height);

    if ((_targetSize.Width == video_w) && (_targetSize.Height == video_h))
    {
        // don't need to stretch after all
        _stretchVideo = false;
    }

    if ((_stretchVideo) && (!gfxDriver->HasAcceleratedTransform()))
    {
        _targetBitmap.reset(BitmapHelper::CreateClearBitmap(
            play.GetMainViewport().GetWidth(), play.GetMainViewport().GetHeight(), game.GetColorDepth()));
        _videoDDB = gfxDriver->CreateDDBFromBitmap(_targetBitmap.get(), false, true);
    }
    else
    {
        _videoDDB = nullptr;
    }

    if (gfxDriver->UsesMemoryBackBuffer())
        gfxDriver->GetMemoryBackBuffer()->Clear();

    // Init APEG
    _dataStream = std::move(video_stream);
    _frameTime = 1000 / _apegStream->frame_rate;
    _videoFrame.reset(new Bitmap()); // FIXME: use target directly if possible?
    _audioChannels = _apegStream->audio.channels;
    _audioFreq = _apegStream->audio.freq;
    _audioFormat = AUDIO_S16SYS;
    apeg_set_error(_apegStream, NULL);

    return true;
}

void TheoraPlayer::CloseImpl()
{
    apeg_close_stream(_apegStream);
    _apegStream = nullptr;
}

bool TheoraPlayer::NextFrame()
{
    const int framedrop = 0;

    // reset some data
    //APEG_LAYER *layer = reinterpret_cast<APEG_LAYER*>(_apegStream);
    _apegStream->frame = 0;
    _apegStream->frame_updated = -1;
    _apegStream->audio.flushed = FALSE;

    int ret = 0;
    if ((_apegStream->flags & APEG_HAS_AUDIO) && _wantAudio)
    {
        unsigned char *buf = nullptr;
        int count = 0;
        ret = apeg_get_audio_frame(_apegStream, &buf, &count);
        _audioFrame = SoundBuffer(buf, count);
        if ((_apegStream->flags & APEG_HAS_VIDEO))
        {
            int pos_ms = GetAudioPos();
            if (pos_ms >= 0) {
                double audio_pos_secs = (double)pos_ms / 1000.0;
                double audio_frames = audio_pos_secs * _apegStream->frame_rate;
                _apegStream->timer = audio_frames - _apegStream->frame;
                // could be negative.. so will wait until 0 ?
            }
        }
    }

    if ((_apegStream->flags & APEG_HAS_VIDEO))
    {
        ret = apeg_get_video_frame(_apegStream);

        if (_apegStream->timer > 0)
        {
            // Update frame and timer count
            ++(_apegStream->frame);
            --(_apegStream->timer);

            // If we're not behind, update the display frame
            _apegStream->frame_updated = 0;
            apeg_display_video_frame(_apegStream);
        }
        /* FIXME: how to do here?
        if (_apegStream->frame_updated == 1 || layer->picture)
            ret = APEG_OK;
            */
    }

    _videoFrame->WrapAllegroBitmap(_apegStream->bitmap, true);

    return ret == APEG_OK;
}

} // namespace Engine
} // namespace AGS


//-----------------------------------------------------------------------------
// Running the single video playback
//-----------------------------------------------------------------------------

void video_run(std::unique_ptr<VideoPlayer> video)
{
    gl_Video = std::move(video);
    // Loop until finished or skipped by player
    while (gl_Video->Poll())
    {
        sys_evt_process_pending();
        update_audio_system_on_game_loop();
    }

    gl_Video.reset();
}

void play_flc_file(int numb, int playflags)
{
    if (play.fast_forward)
        return; // skip video
    // AGS 2.x: If the screen is faded out, fade in again when playing a movie.
    if (loaded_game_file_version <= kGameVersion_272)
        play.screen_is_faded_out = 0;

    // Convert flags
    int skip = playflags % 10;
    playflags -= skip;
    if (skip == 2) // convert to PlayVideo-compatible setting
        skip = 3;

    std::unique_ptr<FlicPlayer> video(new FlicPlayer());
    // Try couple of various filename formats
    String flicname = String::FromFormat("flic%d.flc", numb);
    if (!video->Open(flicname, skip, playflags))
    {
        flicname.Format("flic%d.fli", numb);
        if (!video->Open(flicname, skip, playflags))
        {
            debug_script_warn("FLIC animation flic%d.flc nor flic%d.fli not found", numb, numb);
            return;
        }
    }

    video_run(std::move(video));
}

void play_theora_video(const char *name, int skip, int flags)
{
    std::unique_ptr<TheoraPlayer> video(new TheoraPlayer());
    if (!video->Open(name, skip, flags))
    {
        debug_script_warn("Error playing theora video '%s'", name);
        return;
    }

    video_run(std::move(video));
}

void video_on_gfxmode_changed()
{
    if (gl_Video)
        gl_Video->Restore();
}

void video_shutdown()
{
    gl_Video.reset();
}

#else

void play_theora_video(const char *name, int skip, int flags) {}
void play_flc_file(int numb,int playflags) {}
void video_on_gfxmode_changed() {}

#endif
