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

bool VideoPlayer::Open(const String &name, int flags)
{
    if (!OpenImpl(name, flags))
        return false;

    _flags = flags;
    // Start the audio stream
    if ((flags & kVideo_EnableAudio) != 0)
    {
        stop_all_sound_and_music();
        if ((_audioFormat > 0) && (_audioChannels > 0) && (_audioFreq > 0))
        {
            _audioOut.reset(new OpenAlSource(_audioFormat, _audioChannels, _audioFreq));
            _audioOut->Play();
            _wantAudio = true;
        }
    }
    // Setup video
    if ((flags & kVideo_EnableVideo) != 0)
    {
        _dstRect = PlaceInRect(play.GetMainViewport(), RectWH(_frameSize), kPlaceStretchProportional);
        // override the stretch option if necessary
        if (_frameSize == _dstRect.GetSize())
            _flags &= ~kVideo_Stretch;
        else
            _flags |= kVideo_Stretch;

        if (gfxDriver->HasAcceleratedTransform() || ((_flags & kVideo_Stretch) == 0))
        {
            _videoDDB = gfxDriver->CreateDDB(_frameSize.Width, _frameSize.Height, _frameDepth, true);
        }
        else
        {
            // For software rendering - create helper bitmaps in case of stretching;
            // If we are decoding a 8-bit frame in a hi-color game, and stretching,
            // then create a hi-color buffer, as bitmap lib cannot stretch with depth change
            if ((_frameDepth == 8) && (game.GetColorDepth() > 8))
                _hicolBuf.reset(BitmapHelper::CreateBitmap(_frameSize.Width, _frameSize.Height, game.GetColorDepth()));
            _targetBitmap.reset(BitmapHelper::CreateBitmap(_dstRect.GetWidth(), _dstRect.GetHeight(), game.GetColorDepth()));
            _videoDDB = gfxDriver->CreateDDB(_dstRect.GetWidth(), _dstRect.GetHeight(), game.GetColorDepth(), true);
        }
    }

    _frameTime = 1000 / _frameRate;
    _loop = false;
    return true;
}

void VideoPlayer::Close()
{
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
}

void VideoPlayer::Play()
{
    switch (_playState)
    {
    case PlayStatePaused: Resume(); /* fall-through */
    case PlayStateInitial: _playState = PlayStatePlaying; break;
    default: break; // TODO: support rewind/replay from stop/finished state?
    }
}

void VideoPlayer::Pause()
{
    if (_playState != PlayStatePlaying) return;

    if (_audioOut)
        _audioOut->Pause();
    _playState = PlayStatePaused;
}

void VideoPlayer::Resume()
{
    if (_playState != PlayStatePaused) return;

    if (_audioOut)
        _audioOut->Resume();
    _playState = PlayStatePlaying;
}

int VideoPlayer::GetAudioPos()
{
    return _audioOut ? _audioOut->GetPositionMs() : 0;
}

bool VideoPlayer::Poll()
{
    if (_playState != PlayStatePlaying)
        return false;
    // Acquire next video frame
    if (!NextFrame())
    {
        _playState = PlayStateFinished;
        return false;
    }
    // Render current frame
    if (_audioFrame && !RenderAudio())
    {
        _playState = PlayStateError;
        return false;
    }
    if (_videoFrame && !RenderVideo())
    {
        _playState = PlayStateError;
        return false;
    }
    return true;
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

    // Use intermediate hi-color buffer if necessary
    if (_hicolBuf)
    {
        _hicolBuf->Blit(usebuf);
        usebuf = _hicolBuf.get();
    }

    if ((_flags & kVideo_Stretch) != 0)
    {
        if (gfxDriver->HasAcceleratedTransform())
        {
            gfxDriver->UpdateDDBFromBitmap(_videoDDB, usebuf, false);
            _videoDDB->SetStretch(_dstRect.GetWidth(), _dstRect.GetHeight(), false);
        }
        else
        {
            _targetBitmap->StretchBlt(usebuf, RectWH(_dstRect.GetSize()));
            gfxDriver->UpdateDDBFromBitmap(_videoDDB, _targetBitmap.get(), false);
        }
    }
    else
    {
        gfxDriver->UpdateDDBFromBitmap(_videoDDB, usebuf, false);
    }
    gfxDriver->DrawSprite(_dstRect.Left, _dstRect.Top, _videoDDB);
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
    bool OpenImpl(const AGS::Common::String &name, int &flags) override;
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

bool FlicPlayer::OpenImpl(const AGS::Common::String &name, int &flags)
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

    _videoFrame.reset(BitmapHelper::CreateClearBitmap(fliwidth, fliheight, 8));
    _frameDepth = 8;
    _frameSize = Size(fliwidth, fliheight);
    _frameRate = 1000 / fli_speed;
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
    bool OpenImpl(const AGS::Common::String &name, int &flags) override;
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

bool TheoraPlayer::OpenImpl(const AGS::Common::String &name, int &flags)
{
    std::unique_ptr<Stream> video_stream(AssetMgr->OpenAsset(name));
    apeg_set_stream_reader(apeg_stream_init, apeg_stream_read, apeg_stream_skip);
    apeg_set_display_depth(game.GetColorDepth());
    // we must disable length detection, otherwise it takes ages to start
    // playing if the file is large because it seeks through the whole thing
    apeg_disable_length_detection(TRUE);
    apeg_ignore_audio((flags & kVideo_EnableAudio) == 0);

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

    if (gfxDriver->UsesMemoryBackBuffer())
        gfxDriver->GetMemoryBackBuffer()->Clear();

    // Init APEG
    _dataStream = std::move(video_stream);
    _frameDepth = game.GetColorDepth();
    _frameSize = Size(video_w, video_h);
    _frameRate = _apegStream->frame_rate;
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
    _apegStream->frame_updated = -1;
    _apegStream->audio.flushed = FALSE;

    if ((_apegStream->flags & APEG_HAS_AUDIO) && _wantAudio)
    {
        unsigned char *buf = nullptr;
        int count = 0;
        if (apeg_get_audio_frame(_apegStream, &buf, &count) == APEG_ERROR)
            return false;
        _audioFrame = SoundBuffer(buf, count);
    }

    if ((_apegStream->flags & APEG_HAS_VIDEO))
    {
        if (apeg_get_video_frame(_apegStream) == APEG_ERROR)
            return false;

        // Update frame count
        ++(_apegStream->frame);

        // Update the display frame
        _apegStream->frame_updated = 0;
        apeg_display_video_frame(_apegStream);
    }

    _videoFrame->WrapAllegroBitmap(_apegStream->bitmap, true);

    return true;
}

} // namespace Engine
} // namespace AGS


//-----------------------------------------------------------------------------
// Running the single video playback
//
// TODO: Single video playback as a "game state" class?
//-----------------------------------------------------------------------------
// Checks input events, tells if the video should be skipped
static bool video_check_user_input(VideoSkipType skip)
{
    KeyInput key;
    int mbut, mwheelz;
    if (run_service_key_controls(key))
    {
        if ((key.Key == eAGSKeyCodeEscape) && (skip == VideoSkipEscape))
            return true;
        if (skip >= VideoSkipAnyKey)
            return true;  // skip on any key
    }
    if (run_service_mb_controls(mbut, mwheelz) && (mbut >= 0) && (skip == VideoSkipKeyOrMouse))
        return true; // skip on mouse click
    return false;
}

static void video_run(std::unique_ptr<VideoPlayer> video, int flags, VideoSkipType skip)
{
    gl_Video = std::move(video);

    // Clear the screen before starting playback
    // TODO: needed for FLIC, but perhaps may be done differently
    if ((flags & kVideo_ClearScreen) != 0)
    {
        if (gfxDriver->UsesMemoryBackBuffer())
        {
            Bitmap *screen_bmp = gfxDriver->GetMemoryBackBuffer();
            screen_bmp->Clear();
        }
        render_to_screen();
    }
    
    gl_Video->Play();
    const int old_fps = setTimerFps(gl_Video->GetFramerate());
    // Loop until finished or skipped by player
    while (gl_Video->GetPlayState() == PlayStatePlaying ||
           gl_Video->GetPlayState() == PlayStatePaused)
    {
        sys_evt_process_pending();
        // Check user input skipping the video
        if (video_check_user_input(skip)) break;
        gl_Video->Poll(); // update/render next frame
        UpdateGameAudioOnly(); // update the game and wait for next frame
    }
    setTimerFps(old_fps);
    gl_Video.reset();

    // Clear the screen after stopping playback
    // TODO: needed for FLIC, but perhaps may be done differently
    if (gfxDriver->UsesMemoryBackBuffer())
    {
        Bitmap *screen_bmp = gfxDriver->GetMemoryBackBuffer();
        screen_bmp->Clear();
    }
    render_to_screen();

    invalidate_screen();
    ags_clear_input_state();
}

void play_flc_video(int numb, int flags, VideoSkipType skip)
{
    std::unique_ptr<FlicPlayer> video(new FlicPlayer());
    // Try couple of various filename formats
    String flicname = String::FromFormat("flic%d.flc", numb);
    if (!video->Open(flicname, flags))
    {
        flicname.Format("flic%d.fli", numb);
        if (!video->Open(flicname, flags))
        {
            debug_script_warn("FLIC animation flic%d.flc nor flic%d.fli not found", numb, numb);
            return;
        }
    }

    video_run(std::move(video), flags, skip);
}

void play_theora_video(const char *name, int flags, VideoSkipType skip)
{
    std::unique_ptr<TheoraPlayer> video(new TheoraPlayer());
    if (!video->Open(name, flags))
    {
        debug_script_warn("Error playing theora video '%s'", name);
        return;
    }

    video_run(std::move(video), flags, skip);
}

void video_pause()
{
    if (gl_Video)
        gl_Video->Pause();
}

void video_resume()
{
    if (gl_Video)
        gl_Video->Play();
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

void play_theora_video(const char *name, int flags, AGS::Engine::VideoSkipType skip) {}
void play_flc_video(int numb, int flags, AGS::Engine::VideoSkipType skip) {}
void video_on_gfxmode_changed() {}
void video_shutdown() {}

#endif
