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
#include "core/assetmanager.h"
#include "ac/draw.h"
#include "ac/game.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_audio.h"
#include "ac/joystick.h"
#include "ac/sys_events.h"
#include "debug/debug_log.h"
#include "gfx/graphicsdriver.h"
#include "main/game_run.h"
#include "media/audio/audio.h"
#include "media/video/video_core.h"
#include "util/memory_compat.h"
#include "util/path.h"

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
    BlockingVideoPlayer(int player_id,
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
    void StopVideo();

    int _playerID = 0;
    const String _assetName; // for diagnostics
    int _videoFlags = 0;
    int _stateFlags = 0;
    IDriverDependantBitmap *_videoDDB = nullptr;
    Rect _dstRect;
    float _oldFps = 0.f;
    VideoSkipType _skip = VideoSkipNone;
    PlaybackState _playbackState = PlayStateInvalid;
};

BlockingVideoPlayer::BlockingVideoPlayer(int player_id,
    int video_flags, int state_flags, VideoSkipType skip)
    : _playerID(player_id)
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
    assert(_playerID >= 0);

    // Optionally stop the game audio
    if ((_stateFlags & kVideoState_StopGameAudio) != 0)
    {
        stop_all_sound_and_music();
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

    auto player = video_core_get_player(_playerID);

    // Optionally adjust game speed, but only if it's lower than video's FPS
    auto video_fps = player->GetFramerate();
    auto game_fps = get_game_speed();
    _oldFps = game_fps;
    if (((_stateFlags & kVideoState_SetGameFps) != 0) &&
        (game_fps < video_fps))
    {
        set_game_speed(video_fps);
    }

    // Setup video
    if ((_videoFlags & kVideo_EnableVideo) != 0)
    {
        const bool software_draw = !gfxDriver->HasAcceleratedTransform();
        Size frame_sz = player->GetFrameSize();
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
            player->SetTargetFrame(dest.GetSize());
        }

        const int dst_depth = player->GetTargetDepth();
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

    player->Play();
    _playbackState = player->GetPlayState();
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
        // TODO: the sound restoration here was based on legacy AmbientSound system
        // need to reimplement using modern one? need to investigate how it worked before
    }
}

void BlockingVideoPlayer::Pause()
{
    assert(_playerID >= 0);
    auto player = video_core_get_player(_playerID);
    player->Pause();
}

void BlockingVideoPlayer::Resume()
{
    assert(_playerID >= 0);
    auto player = video_core_get_player(_playerID);
    player->Play();
}

bool BlockingVideoPlayer::Run()
{
    assert(_playerID >= 0);
    if (_playerID < 0)
        return false;
    // Loop until finished or skipped by player
    if (IsPlaybackDone(GetPlayState()))
        return false;

    sys_evt_process_pending();
    // Check user input skipping the video
    if (video_check_user_input(_skip))
        return false;

    std::unique_ptr<Bitmap> frame;
    {
        auto player = video_core_get_player(_playerID);
        // update/render next frame
        _playbackState = player->GetPlayState();
        frame = player->GetReadyFrame();
    }

    if ((_videoFlags & kVideo_EnableVideo) != 0)
    {
        if (frame)
        {
            gfxDriver->UpdateDDBFromBitmap(_videoDDB, frame.get());
            _videoDDB->SetStretch(_dstRect.GetWidth(), _dstRect.GetHeight(), false);

            {
                auto player = video_core_get_player(_playerID);
                player->ReleaseFrame(std::move(frame));
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
    if (_playerID >= 0)
    {
        video_core_slot_stop(_playerID);
        _playerID = -1;
    }

    if (_videoDDB)
        gfxDriver->DestroyDDB(_videoDDB);
    _videoDDB = nullptr;
}

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

static HError video_single_run(const String &asset_name,
    int video_flags, int state_flags, VideoSkipType skip)
{
    std::unique_ptr<Stream> video_stream(AssetMgr->OpenAsset(asset_name));
    if (!video_stream)
    {
        return new Error(String::FromFormat("Failed to open file: %s", asset_name.GetCStr()));
    }

    VideoInitParams params;
    params.Flags = static_cast<VideoFlags>(video_flags);
    params.TargetColorDepth = game.GetColorDepth();

    video_core_init();
    auto slot = video_core_slot_init(std::move(video_stream), asset_name, Path::GetFileExtension(asset_name), params);
    if (slot < 0)
    {
        return new Error(String::FromFormat("Failed to run video: %s", asset_name.GetCStr()));
    }

    gl_Video.reset(new BlockingVideoPlayer(slot, video_flags, state_flags, skip));
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

    return video_single_run(flicname, video_flags, state_flags, skip);
}

HError play_theora_video(const char *name, int video_flags, int state_flags, VideoSkipType skip)
{
    return video_single_run(name, video_flags, state_flags, skip);
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
    video_core_shutdown();
}

#else

HError play_theora_video(const char *name, int video_flags, int state_flags, AGS::Engine::VideoSkipType skip) { return HError::None(); }
HError play_flc_video(int numb, int video_flags, int state_flags, AGS::Engine::VideoSkipType skip) { return HError::None(); }
void video_pause() {}
void video_resume() {}
void video_shutdown() {}

#endif
