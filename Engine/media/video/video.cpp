//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
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
#include <chrono>
#include <mutex>
#include <thread>
#include "core/assetmanager.h"
#include "ac/draw.h"
#include "ac/game.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_audio.h"
#include "ac/sys_events.h"
#include "debug/debug_log.h"
#include "gfx/graphicsdriver.h"
#include "main/game_run.h"
#include "media/audio/audio.h"
#include "media/video/videoplayer.h"
#include "media/video/flic_player.h"
#include "media/video/theora_player.h"
#include "util/memory_compat.h"

using namespace AGS::Common;
using namespace AGS::Engine;


extern GameSetupStruct game;
extern IGraphicsDriver *gfxDriver;

static bool video_check_user_input(VideoSkipType skip);

//-----------------------------------------------------------------------------
// BlockingVideoPlayer game state
//-----------------------------------------------------------------------------
namespace AGS
{
namespace Engine
{

// Blocking video playback state
class BlockingVideoPlayer : public GameState
{
public:
    BlockingVideoPlayer(std::unique_ptr<VideoPlayer> player,
        int video_flags, int state_flags, VideoSkipType skip);
    ~BlockingVideoPlayer();

    PlaybackState GetPlayState() const { return _playbackState; }

    void Begin() override;
    void End() override;
    void Draw() override;
    bool Run() override;

    void Pause() override;
    void Resume() override;

private:
#if !defined(AGS_DISABLE_THREADS)
    static void PollVideo(BlockingVideoPlayer *self);
#endif
    void StopVideo();

    std::unique_ptr<VideoPlayer> _player;
    const String _assetName; // for diagnostics
    int _videoFlags = 0;
    int _stateFlags = 0;
    IDriverDependantBitmap *_videoDDB = nullptr;
    Rect _dstRect;
    float _oldFps = 0.f;
    VideoSkipType _skip = VideoSkipNone;
    std::thread _videoThread;
    std::mutex _videoMutex;
    PlaybackState _playbackState = PlayStateInvalid;

    // For saving and restoring game sounds
    int _wasMusPlaying = -1;
    int _wasAmbient[MAX_GAME_CHANNELS]{0};
};

BlockingVideoPlayer::BlockingVideoPlayer(std::unique_ptr<VideoPlayer> player,
    int video_flags, int state_flags, VideoSkipType skip)
    : _player(std::move(player))
    , _videoFlags(video_flags)
    , _stateFlags(state_flags)
    , _skip(skip)
{
}

BlockingVideoPlayer::~BlockingVideoPlayer()
{
    StopVideo();
}

void BlockingVideoPlayer::Begin()
{
    assert(_player);

    // Optionally stop the game audio
    if ((_stateFlags & kVideoState_StopGameAudio) != 0)
    {
        // Save the game audio parameters, in case we stop these
        // TODO: implement a global function that does this?
        _wasMusPlaying = play.cur_music_number;
        for (int i = NUM_SPEECH_CHANS; i < game.numGameChannels; ++i)
            _wasAmbient[i] = ambient[i].channel;
        stop_all_sound_and_music();
    }

    // Setup video
    if ((_videoFlags & kVideo_EnableVideo) != 0)
    {
        const bool software_draw = gfxDriver->HasAcceleratedTransform();
        Size frame_sz = _player->GetFrameSize();
        Rect dest = PlaceInRect(play.GetMainViewport(), RectWH(frame_sz),
            ((_stateFlags & kVideoState_Stretch) == 0) ? kPlaceCenter : kPlaceStretchProportional);
        // override the stretch option if necessary
        if (frame_sz == dest.GetSize())
            _stateFlags &= ~kVideoState_Stretch;
        else
            _stateFlags |= kVideoState_Stretch;

        // We only need to resize target bitmap for software renderer,
        // because texture-based ones can scale the texture themselves.
        if (software_draw && (frame_sz != dest.GetSize()))
        {
            _player->SetTargetFrame(dest.GetSize());
        }

        const int dst_depth = _player->GetTargetDepth();
        if (!software_draw || ((_stateFlags & kVideoState_Stretch) == 0))
        {
            _videoDDB = gfxDriver->CreateDDB(frame_sz.Width, frame_sz.Height, dst_depth, true);
        }
        else
        {
            _videoDDB = gfxDriver->CreateDDB(dest.GetWidth(), dest.GetHeight(), dst_depth, true);
        }
        _dstRect = dest;
    }

    // Clear the screen before starting playback
    // TODO: needed for FLIC, but perhaps may be done differently
    if ((_stateFlags & kVideoState_ClearScreen) != 0)
    {
        if (gfxDriver->UsesMemoryBackBuffer())
        {
            gfxDriver->GetMemoryBackBuffer()->Clear();
        }
        render_to_screen();
    }

    auto video_fps = _player->GetFramerate();
    auto game_fps = get_game_speed();
    _oldFps = game_fps;
    if (((_stateFlags & kVideoState_SetGameFps) != 0) &&
        (game_fps < video_fps))
    {
        set_game_speed(video_fps);
    }

    _player->Play();
    _playbackState = _player->GetPlayState();

#if !defined(AGS_DISABLE_THREADS)
    _videoThread = std::thread(BlockingVideoPlayer::PollVideo, this);
#endif
}

void BlockingVideoPlayer::End()
{
    StopVideo();

    set_game_speed(_oldFps);

    // Clear the screen after stopping playback
    // TODO: needed for FLIC, but perhaps may be done differently
    if ((_stateFlags & kVideoState_ClearScreen) != 0)
    {
        if (gfxDriver->UsesMemoryBackBuffer())
        {
            gfxDriver->GetMemoryBackBuffer()->Clear();
        }
        render_to_screen();
    }

    invalidate_screen();
    ags_clear_input_state();

    // Restore the game audio if we stopped them before the video playback
    if ((_stateFlags & kVideoState_StopGameAudio) != 0)
    {
        // TODO: implement a global function that does this?
        update_music_volume();
        if (_wasMusPlaying >= 0)
            newmusic(_wasMusPlaying);
        for (int i = NUM_SPEECH_CHANS; i < game.numGameChannels; ++i)
        {
            if (_wasAmbient[i] > 0)
                PlayAmbientSound(_wasAmbient[i], ambient[i].num, ambient[i].vol, ambient[i].x, ambient[i].y);
        }
    }
}

void BlockingVideoPlayer::Pause()
{
    assert(_player);
    std::lock_guard<std::mutex> lk(_videoMutex);
    _player->Pause();
}

void BlockingVideoPlayer::Resume()
{
    assert(_player);
    std::lock_guard<std::mutex> lk(_videoMutex);
    _player->Play();
}

bool BlockingVideoPlayer::Run()
{
    assert(_player);
    if (!_player)
        return false;
    // Loop until finished or skipped by player
    if (IsPlaybackDone(GetPlayState()))
        return false;

    sys_evt_process_pending();
    // Check user input skipping the video
    if (video_check_user_input(_skip))
        return false;

#if defined(AGS_DISABLE_THREADS)
    if (!_player->Poll())
        return false;
#endif

    // update/render next frame
    std::unique_ptr<Bitmap> frame;
    {
        std::lock_guard<std::mutex> lk(_videoMutex);
        _playbackState = _player->GetPlayState();
        frame = _player->GetReadyFrame();
    }

    if ((_videoFlags & kVideo_EnableVideo) != 0)
    {
        if (frame)
        {
            gfxDriver->UpdateDDBFromBitmap(_videoDDB, frame.get(), false);
            _videoDDB->SetStretch(_dstRect.GetWidth(), _dstRect.GetHeight(), false);

            {
                std::lock_guard<std::mutex> lk(_videoMutex);
                _player->ReleaseFrame(std::move(frame));
            }
        }
    }

    Draw();

    // update the game and wait for next frame
    UpdateGameAudioOnly();
    return IsPlaybackReady(GetPlayState());
}

void BlockingVideoPlayer::Draw()
{
    if (_videoDDB)
    {
        gfxDriver->BeginSpriteBatch(play.GetMainViewport(), SpriteTransform());
        gfxDriver->DrawSprite(_dstRect.Left, _dstRect.Top, _videoDDB);
        gfxDriver->EndSpriteBatch();
    }
    render_to_screen();
}

void BlockingVideoPlayer::StopVideo()
{
    // Stop player and wait for the thread to stop
    if (_player)
    {
        std::lock_guard<std::mutex> lk(_videoMutex);
        _player->Stop();
    }
#if !defined(AGS_DISABLE_THREADS)
    if (_videoThread.joinable())
        _videoThread.join();
#endif

    if (_videoDDB)
        gfxDriver->DestroyDDB(_videoDDB);
    _videoDDB = nullptr;
}

#if !defined(AGS_DISABLE_THREADS)
/* static */ void BlockingVideoPlayer::PollVideo(BlockingVideoPlayer *self)
{
    assert(self && self->_player.get());
    if (!self || !self->_player.get())
        return;

    bool do_run = true;
    while (do_run)
    {
        {
            std::lock_guard<std::mutex> lk(self->_videoMutex);
            self->_player->Poll();
            do_run = IsPlaybackReady(self->_player->GetPlayState());
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(8));
    }
}
#endif

std::unique_ptr<BlockingVideoPlayer> gl_Video;

} // namespace Engine
} // namespace AGS


//-----------------------------------------------------------------------------
// Blocking video API
// Running the single video playback
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
    }
    return false;
}

static HError video_single_run(std::unique_ptr<VideoPlayer> video, const String &asset_name,
    int video_flags, int state_flags, VideoSkipType skip)
{
    assert(video);
    if (!video)
        return HError::None();

    auto video_stream = AssetMgr->OpenAsset(asset_name);
    if (!video_stream)
    {
        return new Error(String::FromFormat("Failed to open file: %s", asset_name.GetCStr()));
    }

    const int dst_depth = game.GetColorDepth();
    HError err = video->Open(std::move(video_stream), asset_name, video_flags, Size(), dst_depth);
    if (!err)
    {
        return new Error(String::FromFormat("Failed to run video %s", asset_name.GetCStr()), err);
    }

    gl_Video.reset(new BlockingVideoPlayer(std::move(video), video_flags, state_flags, skip));
    gl_Video->Begin();
    while (gl_Video->Run());
    gl_Video->End();
    gl_Video.reset();
    
    return HError::None();
}

HError play_flc_video(int numb, int video_flags, int state_flags, VideoSkipType skip)
{
    // Try couple of various filename formats
    String flicname = String::FromFormat("flic%d.flc", numb);
    if (!AssetMgr->DoesAssetExist(flicname))
    {
        flicname.Format("flic%d.fli", numb);
        if (!AssetMgr->DoesAssetExist(flicname))
            return new Error(String::FromFormat("FLIC animation flic%d.flc nor flic%d.fli were found", numb, numb));
    }

    return video_single_run(std::make_unique<FlicPlayer>(), flicname, video_flags, state_flags, skip);
}

HError play_theora_video(const char *name, int video_flags, int state_flags, VideoSkipType skip)
{
    return video_single_run(std::make_unique<TheoraPlayer>(), name, video_flags, state_flags, skip);
}

void video_single_pause()
{
    if (gl_Video)
        gl_Video->Pause();
}

void video_single_resume()
{
    if (gl_Video)
        gl_Video->Resume();
}

void video_single_stop()
{
    gl_Video.reset();
}

void video_shutdown()
{
    video_single_stop();
}

#else

HError play_theora_video(const char *name, int video_flags, int state_flags, AGS::Engine::VideoSkipType skip) { return HError::None(); }
HError play_flc_video(int numb, int video_flags, int state_flags, AGS::Engine::VideoSkipType skip) { return HError::None(); }
void video_pause() {}
void video_resume() {}
void video_shutdown() {}

#endif
