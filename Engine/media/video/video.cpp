//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2023 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
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
#include "ac/joystick.h"

using namespace AGS::Common;
using namespace AGS::Engine;


extern GameSetupStruct game;
extern IGraphicsDriver *gfxDriver;


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

HError VideoPlayer::Open(const String &name, int flags)
{
    HError err = OpenImpl(name, flags);
    if (!err)
        return err;

    _flags = flags;
    // Start the audio stream
    if ((flags & kVideo_EnableAudio) != 0)
    {
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
        _dstRect = PlaceInRect(play.GetMainViewport(), RectWH(_frameSize),
            ((_flags & kVideo_Stretch) == 0) ? kPlaceCenter : kPlaceStretchProportional);
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
    return HError::None();
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
    if (!IsValid())
        return;

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
    if (!NextFrame() && !_audioFrame)
    { // stop is no new frames, and no buffered frames left
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
    if (_wantAudio)
        _audioFrame = SoundBuffer(); // clear received buffer
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
            gfxDriver->UpdateDDBFromBitmap(_videoDDB, usebuf);
            _videoDDB->SetStretch(_dstRect.GetWidth(), _dstRect.GetHeight(), false);
        }
        else
        {
            _targetBitmap->StretchBlt(usebuf, RectWH(_dstRect.GetSize()));
            gfxDriver->UpdateDDBFromBitmap(_videoDDB, _targetBitmap.get());
        }
    }
    else
    {
        gfxDriver->UpdateDDBFromBitmap(_videoDDB, usebuf);
    }
    gfxDriver->BeginSpriteBatch(play.GetMainViewport(), SpriteTransform());
    gfxDriver->DrawSprite(_dstRect.Left, _dstRect.Top, _videoDDB);
    gfxDriver->EndSpriteBatch();
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

    bool IsValid() override { return _pf != nullptr; }
    void Restore() override;

private:
    HError OpenImpl(const AGS::Common::String &name, int &flags) override;
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

HError FlicPlayer::OpenImpl(const AGS::Common::String &name, int& /*flags*/)
{
    std::unique_ptr<Stream> in(AssetMgr->OpenAsset(name));
    if (!in)
        return new Error(String::FromFormat("Failed to open file: %s", name.GetCStr()));
    in->Seek(8);
    const int fliwidth = in->ReadInt16();
    const int fliheight = in->ReadInt16();
    in.reset();

    PACKFILE *pf = PackfileFromAsset(AssetPath(name, "*"));
    if (open_fli_pf(pf) != FLI_OK)
    {
        pack_fclose(pf);
        return new Error("Failed to open FLI/FLC animation; could be an invalid or unsupported format");
    }
    _pf = pf;

    get_palette_range(_oldpal, 0, 255);

    _videoFrame.reset(BitmapHelper::CreateClearBitmap(fliwidth, fliheight, 8));
    _frameDepth = 8;
    _frameSize = Size(fliwidth, fliheight);
    _frameRate = 1000 / fli_speed;
    return HError::None();
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

    bool IsValid() override { return _apegStream != nullptr; }

private:
    HError OpenImpl(const AGS::Common::String &name, int &flags) override;
    void CloseImpl() override;
    bool NextFrame() override;

    std::unique_ptr<Stream> _dataStream;
    APEG_STREAM *_apegStream = nullptr;
    // Optional wrapper around original buffer frame (in case we want to extract a portion of it)
    std::unique_ptr<Bitmap> _theoraFrame;
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

HError TheoraPlayer::OpenImpl(const AGS::Common::String &name, int &flags)
{
    std::unique_ptr<Stream> video_stream(AssetMgr->OpenAsset(name));
    if (!video_stream)
    {
        return new Error(String::FromFormat("Failed to open file: %s", name.GetCStr()));
    }

    apeg_set_stream_reader(apeg_stream_init, apeg_stream_read, apeg_stream_skip);
    apeg_set_display_depth(game.GetColorDepth());
    // we must disable length detection, otherwise it takes ages to start
    // playing if the file is large because it seeks through the whole thing
    apeg_disable_length_detection(TRUE);
    apeg_ignore_audio((flags & kVideo_EnableAudio) == 0);

    APEG_STREAM* apeg_stream = apeg_open_stream_ex(video_stream.get());
    if (!apeg_stream)
    {
        return new Error(String::FromFormat("Failed to open theora video '%s'; could be an invalid or unsupported format", name.GetCStr()));
    }
    int video_w = apeg_stream->w, video_h = apeg_stream->h;
    if (video_w <= 0 || video_h <= 0)
    {
        return new Error(String::FromFormat("Failed to run theora video '%s': invalid frame dimensions (%d x %d)", name.GetCStr(), video_w, video_h));
    }

    _apegStream = apeg_stream;

    if (gfxDriver->UsesMemoryBackBuffer())
        gfxDriver->GetMemoryBackBuffer()->Clear();

    // Init APEG
    _dataStream = std::move(video_stream);
    _frameDepth = game.GetColorDepth();
    _frameRate = _apegStream->frame_rate;
    const Size video_size = Size(video_w, video_h);
    // According to the documentation:
    // encoded theora frames must be a multiple of 16 in width and height.
    // Which means that the original content may end up positioned on a larger frame.
    // In such case we store this surface in a separate wrapper for the reference,
    // while the actual video frame is assigned a sub-bitmap (a portion of the full frame).
    if (Size(_apegStream->bitmap->w, _apegStream->bitmap->h) != video_size)
    {
        _theoraFrame.reset(BitmapHelper::CreateRawBitmapWrapper(_apegStream->bitmap));
        _videoFrame.reset(BitmapHelper::CreateSubBitmap(_theoraFrame.get(), RectWH(video_size)));
    }
    else
    {
        _videoFrame.reset(BitmapHelper::CreateRawBitmapWrapper(_apegStream->bitmap));
    }
    _frameSize = _videoFrame->GetSize();

    _audioChannels = _apegStream->audio.channels;
    _audioFreq = _apegStream->audio.freq;
    _audioFormat = AUDIO_S16SYS;
    apeg_set_error(_apegStream, NULL);
    return HError::None();
}

void TheoraPlayer::CloseImpl()
{
    apeg_close_stream(_apegStream);
    _apegStream = nullptr;
}

bool TheoraPlayer::NextFrame()
{
    assert(_apegStream);
    // reset some data
    bool has_audio = false, has_video = false;
    _apegStream->frame_updated = -1;
    _apegStream->audio.flushed = FALSE;

    if ((_apegStream->flags & APEG_HAS_AUDIO) && _wantAudio)
    {
        unsigned char *buf = nullptr;
        int count = 0;
        int ret = apeg_get_audio_frame(_apegStream, &buf, &count);
        if (ret == APEG_ERROR)
            return false;
        _audioFrame = SoundBuffer(buf, count);
        has_audio = ret != APEG_EOF;
    }

    if ((_apegStream->flags & APEG_HAS_VIDEO))
    {
        int ret = apeg_get_video_frame(_apegStream);
        if (ret == APEG_ERROR)
            return false;

        // Update frame count
        ++(_apegStream->frame);

        // Update the display frame
        _apegStream->frame_updated = 0;
        ret = apeg_display_video_frame(_apegStream);
        has_video = ret != APEG_EOF;
    }

    return has_audio || has_video;
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
    for (InputType type = ags_inputevent_ready(); type != kInputNone; type = ags_inputevent_ready())
    {
        if (type == kInputKeyboard)
        {
            KeyInput ki;
            if (run_service_key_controls(ki) && !IsAGSServiceKey(ki.Key))
            {
                if ((ki.Key == eAGSKeyCodeEscape) && (skip == VideoSkipEscape))
                    return true; // skip on Escape key
                if (skip >= VideoSkipAnyKey)
                    return true;  // skip on any key
            }
        }
        else if (type == kInputMouse)
        {
            eAGSMouseButton mbut;
            if (run_service_mb_controls(mbut) && (skip == VideoSkipKeyOrMouse))
                return true; // skip on mouse click
        }
        else if (type == kInputGamepad)
        {
            GamepadInput gbut;
            if (run_service_gamepad_controls(gbut) && (skip == VideoSkipAnyKey) &&
                is_default_gamepad_skip_button_pressed(gbut.Button))
                return true; // skip on ABXY
        }
    }
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
        if (video_check_user_input(skip))
            break;
        gl_Video->Poll(); // update/render next frame
        UpdateGameAudioOnly(); // update the game and wait for next frame
    }
    setTimerFps(old_fps);
    gl_Video.reset();

    // Clear the screen after stopping playback
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

    invalidate_screen();
    ags_clear_input_state();
}

HError play_flc_video(int numb, int flags, VideoSkipType skip)
{
    std::unique_ptr<FlicPlayer> video(new FlicPlayer());
    // Try couple of various filename formats
    String flicname = String::FromFormat("flic%d.flc", numb);
    if (!AssetMgr->DoesAssetExist(flicname))
    {
        flicname.Format("flic%d.fli", numb);
        if (!AssetMgr->DoesAssetExist(flicname))
            return new Error(String::FromFormat("FLIC animation flic%d.flc nor flic%d.fli were found", numb, numb));
    }
    HError err = video->Open(flicname, flags);
    if (!err)
        return err;

    video_run(std::move(video), flags, skip);
    return HError::None();
}

HError play_theora_video(const char *name, int flags, VideoSkipType skip)
{
    std::unique_ptr<TheoraPlayer> video(new TheoraPlayer());
    HError err = video->Open(name, flags);
    if (!err)
        return err;

    video_run(std::move(video), flags, skip);
    return HError::None();
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
void video_pause() {}
void video_resume() {}
void video_on_gfxmode_changed() {}
void video_shutdown() {}

#endif
