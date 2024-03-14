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
#include "ac/dynamicsprite.h"
#include "ac/game.h"
#include "ac/game.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_audio.h"
#include "ac/joystick.h"
#include "ac/spritecache.h"
#include "ac/sys_events.h"
#include "ac/dynobj/dynobj_manager.h"
#include "ac/dynobj/scriptvideoplayer.h"
#include "debug/debug_log.h"
#include "gfx/graphicsdriver.h"
#include "main/game_run.h"
#include "media/video/video_core.h"
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
// Legacy Blocking video API
//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
// Non-blocking video API
//-----------------------------------------------------------------------------

VideoControl::VideoControl(int video_id, int sprite_id)
    : _videoID(video_id)
    , _spriteID(sprite_id)
{
    auto player = video_core_get_player(video_id);
    _frameRate = player->GetFramerate();
    _durMs = player->GetDurationMs();
    _frameCount = _durMs * 1000.0 / _frameRate;
}

VideoControl::~VideoControl()
{
    if (_scriptHandle >= 0)
    {
        ScriptVideoPlayer *sc_video = (ScriptVideoPlayer*)ccGetObjectAddressFromHandle(_scriptHandle);
        if (sc_video)
        {
            sc_video->Invalidate();
            // FIXME: need to fix ManagedPool to avoid recursive Dispose of disposing object!!
            //ccAttemptDisposeObject(_scriptHandle);
        }
    }
}

void VideoControl::SetScriptHandle(int sc_handle)
{
    _scriptHandle = sc_handle;
    // TODO: do we need to check & invalidate previous handle, if there was any?
}

bool VideoControl::Play()
{
    if (!IsReady())
        return false;
    _state = PlaybackState::PlayStatePlaying;
    return true;
}

void VideoControl::Pause()
{
    if (!IsReady())
        return;
    auto player = video_core_get_player(_videoID);
    player->Pause();
    _state = player->GetPlayState();
}

bool VideoControl::NextFrame()
{
    if (!IsReady())
        return false;

    std::unique_ptr<Bitmap> new_frame;
    // Lock video player, sync play state and retrieve a new frame
    {
        auto player = video_core_get_player(_videoID);
        player->Pause();
        _state = player->GetPlayState();
        _posMs = player->GetPositionMs();
        _frameIndex = player->GetFrameIndex();
        new_frame = player->NextFrame();
    }

    if (!new_frame)
        return false;

    // Apply a new frame, return old frame to the player
    auto old_frame = SetNewFrame(std::move(new_frame));
    if (old_frame)
    {
        auto player = video_core_get_player(_videoID);
        player->ReleaseFrame(std::move(old_frame));
    }
    return true;
}

uint32_t VideoControl::SeekFrame(uint32_t frame)
{
    if (!IsReady())
        return -1;
    auto player = video_core_get_player(_videoID);
    player->Pause();
    uint32_t new_pos = player->SeekFrame(frame);
    _state = player->GetPlayState();
    _posMs = player->GetPositionMs();
    _frameIndex = player->GetFrameIndex();
    return new_pos;
}

float VideoControl::SeekMs(float pos_ms)
{
    if (!IsReady())
        return -1.f;
    auto player = video_core_get_player(_videoID);
    player->Pause();
    float new_pos = player->Seek(pos_ms);
    _state = player->GetPlayState();
    _posMs = player->GetPositionMs();
    _frameIndex = player->GetFrameIndex();
    return new_pos;
}

std::unique_ptr<Bitmap> VideoControl::SetNewFrame(std::unique_ptr<Bitmap> new_frame)
{
    // FIXME: this is ugly to use different levels of sprite interface here,
    // expand dynamic_sprite group of functions instead!
    auto old_sprite = spriteset.RemoveSprite(_spriteID);
    spriteset.SetSprite(_spriteID, std::move(new_frame), SPF_DYNAMICALLOC | SPF_OBJECTOWNED);
    game_sprite_updated(_spriteID, false);
    return old_sprite;
}

bool VideoControl::Update()
{
    if (!IsReady())
        return false;

    std::unique_ptr<Bitmap> new_frame;
    // Lock video player, sync play state and retrieve a new frame
    {
        auto player = video_core_get_player(_videoID);
        // Get current video state
        PlaybackState core_state = player->GetPlayState();
        _posMs = player->GetPositionMs();
        _frameIndex = player->GetFrameIndex();

        // If video playback already stopped on its own, then exit update early
        if (IsPlaybackDone(core_state))
        {
            _state = core_state;
            return false;
        }

        // Apply new parameters, do this early in case we start playback
        if (_paramsChanged)
        {
            auto vol_f = static_cast<float>(_volume) / 255.0f;
            if (vol_f < 0.0f) { vol_f = 0.0f; }
            if (vol_f > 1.0f) { vol_f = 1.0f; }

            auto speed_f = _speed;
            if (speed_f <= 0.0) { speed_f = 1.0f; }

            player->SetVolume(vol_f);
            player->SetSpeed(speed_f);
            _paramsChanged = false;
        }

        // Apply new playback state
        if (_state != core_state)
        {
            switch (_state)
            {
            case PlaybackState::PlayStatePlaying:
                player->Play();
                _state = player->GetPlayState();
                break;
            default: /* do nothing */
                break;
            }
        }

        if (_state != PlaybackState::PlayStatePlaying)
            return false;

        // Try get new video frame
        new_frame = player->GetReadyFrame();
    }

    // Apply a new frame, return old frame to the player
    if (new_frame)
    {
        auto old_frame = SetNewFrame(std::move(new_frame));
        if (old_frame)
        {
            auto player = video_core_get_player(_videoID);
            player->ReleaseFrame(std::move(old_frame));
        }
    }
    return true;
}


// map video ID (matching video core handle) to VideoControl object
std::unordered_map<int, std::unique_ptr<VideoControl>> gl_VideoObjects;

HError open_video(const char *name, int video_flags, int &video_id)
{
    std::unique_ptr<Stream> video_stream(AssetMgr->OpenAsset(name));
    if (!video_stream)
        return new Error(String::FromFormat("Failed to open file: %s", name));

    if (gl_VideoObjects.empty())
        video_core_init(); // start the thread

    VideoInitParams params;
    params.Flags = static_cast<VideoFlags>(video_flags);
    params.TargetColorDepth = game.GetColorDepth();
    int video_slot = video_core_slot_init(std::move(video_stream), name, Path::GetFileExtension(name), params);
    if (video_slot < 0) // FIXME: return errors from slot_init?
        return new Error(String::FromFormat("Failed to initialize video player for %s", name));

    // Allocate a sprite slot for this video player's frames;
    // note that the very first frame could be a dummy frame created as a placeholder
    std::unique_ptr<Bitmap> first_frame;
    {
        auto player = video_core_get_player(video_slot);
        first_frame = player->GetEmptyFrame();
    }
    int sprite_slot = add_dynamic_sprite(std::move(first_frame));
    if (sprite_slot <= 0)
        return new Error("No free sprite slot to render video to");

    auto video_ctrl = std::make_unique<VideoControl>(video_slot, sprite_slot);
    gl_VideoObjects[video_slot] = std::move(video_ctrl);

    // NOTE: we do not start playback right away,
    // but do so during the synchronization step (see sync_video_playback).

    video_id = video_slot;
    return HError::None();
}

VideoControl *get_video_control(int video_id)
{
    auto it = gl_VideoObjects.find(video_id);
    if (it == gl_VideoObjects.end())
        return nullptr;
    return it->second.get();
}

void video_stop(int video_id)
{
    auto it = gl_VideoObjects.find(video_id);
    if (it == gl_VideoObjects.end())
        return; // wrong index

    int video_slot = it->first;
    video_core_slot_stop(video_slot);
    free_dynamic_sprite(it->second->GetSpriteID());
    gl_VideoObjects.erase(video_slot);

    if (gl_VideoObjects.empty())
        video_core_shutdown(); // stop the thread to avoid redundant processing
}

void sync_video_playback()
{
    for (auto &obj : gl_VideoObjects)
    {
        VideoControl *video_ctrl = obj.second.get();
        video_ctrl->Update();
    }
}

void update_video_system_on_game_loop()
{
    sync_video_playback();
}

//-----------------------------------------------------------------------------

void video_shutdown()
{
    video_single_stop();
    video_core_shutdown();
    gl_VideoObjects.clear();
}

#else

//-----------------------------------------------------------------------------
// Stubs for the video support
//-----------------------------------------------------------------------------

using namespace AGS::Common;

HError play_theora_video(const char *, int, int, AGS::Engine::VideoSkipType)
{
    return new Error("Video playback is not supported in this engine build.");
}
HError play_flc_video(int, int, int, AGS::Engine::VideoSkipType)
{
    return new Error("Video playback is not supported in this engine build.");
}
void video_single_pause() {}
void video_single_resume() {}
void video_single_stop() {}

HError open_video(const char *, int, int &)
{
    return new Error("Video playback is not supported in this engine build.");
}
VideoControl *get_video_control(int) { return nullptr; }
void video_stop(int) { }
void sync_video_playback() { }
void update_video_system_on_game_loop() { }
void video_shutdown() { }

void VideoControl::SetScriptHandle(int) {}
bool VideoControl::Play() { return false; }
void VideoControl::Pause() {}
bool VideoControl::NextFrame() { return false; }
uint32_t VideoControl::SeekFrame(uint32_t) { return -1; }
float VideoControl::SeekMs(float) { return -1.f; }

#endif
