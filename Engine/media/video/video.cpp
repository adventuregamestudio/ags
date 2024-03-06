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
#include "ac/sys_events.h"
#include "debug/debug_log.h"
#include "gfx/graphicsdriver.h"
#include "main/game_run.h"
#include "media/video/videoplayer.h"
#include "media/video/flic_player.h"
#include "media/video/theora_player.h"
#include "util/memory_compat.h"

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

// Blocking video playback state
class BlockingVideoPlayer
{
public:
    HError Open(std::unique_ptr<VideoPlayer> player, const String &asset_name,
        int video_flags, int state_flags);
    void Close();

    const VideoPlayer *GetVideoPlayer() const { return _player.get(); }
    PlaybackState GetPlayState() const { return _player->GetPlayState(); }

    void Play();
    void Pause();
    // Updates the video playback, renders next frame
    bool Poll();

private:
    int _videoFlags = 0;
    int _stateFlags = 0;
    std::unique_ptr<VideoPlayer> _player;
    IDriverDependantBitmap *_videoDDB = nullptr;
    Rect _dstRect;
};

HError BlockingVideoPlayer::Open(std::unique_ptr<VideoPlayer> player,
    const String &asset_name, int video_flags, int state_flags)
{
    std::unique_ptr<Stream> video_stream(AssetMgr->OpenAsset(asset_name));
    if (!video_stream)
    {
        return new Error(String::FromFormat("Failed to open file: %s", asset_name.GetCStr()));
    }

    const int dst_depth = game.GetColorDepth();
    HError err = player->Open(std::move(video_stream), asset_name, video_flags, Size(), dst_depth);
    if (!err)
    {
        return err;
    }

    _player = std::move(player);
    _videoFlags = video_flags;
    _stateFlags = state_flags;
    // Setup video
    if ((video_flags & kVideo_EnableVideo) != 0)
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
    return HError::None();
}

void BlockingVideoPlayer::Close()
{
    if (_player)
        _player->Stop();

    if (_videoDDB)
        gfxDriver->DestroyDDB(_videoDDB);
    _videoDDB = nullptr;
}

void BlockingVideoPlayer::Play()
{
    assert(_player);
    _player->Play();
}

void BlockingVideoPlayer::Pause()
{
    assert(_player);
    _player->Pause();
}

bool BlockingVideoPlayer::Poll()
{
    assert(_player);
    if (!_player->Poll())
        return false;

    if ((_videoFlags & kVideo_EnableVideo) == 0)
        return true;

    const Bitmap *frame = _player->GetVideoFrame();
    gfxDriver->UpdateDDBFromBitmap(_videoDDB, frame, false);
    _videoDDB->SetStretch(_dstRect.GetWidth(), _dstRect.GetHeight(), false);

    gfxDriver->BeginSpriteBatch(play.GetMainViewport(), SpriteTransform());
    gfxDriver->DrawSprite(_dstRect.Left, _dstRect.Top, _videoDDB);
    gfxDriver->EndSpriteBatch();
    render_to_screen();
    return true;
}

std::unique_ptr<BlockingVideoPlayer> gl_Video;

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
    }
    return false;
}

static void video_run(std::unique_ptr<VideoPlayer> video, const String &asset_name,
    int video_flags, int state_flags, VideoSkipType skip)
{
    assert(video);
    if (!video)
        return;

    gl_Video.reset(new BlockingVideoPlayer());
    HError err = gl_Video->Open(std::move(video), asset_name, video_flags, state_flags);
    if (!err)
    {
        debug_script_warn("Failed to run video %s: %s", asset_name.GetCStr(), err->FullMessage().GetCStr());
    }

    // Clear the screen before starting playback
    // TODO: needed for FLIC, but perhaps may be done differently
    if ((state_flags & kVideoState_ClearScreen) != 0)
    {
        if (gfxDriver->UsesMemoryBackBuffer())
        {
            gfxDriver->GetMemoryBackBuffer()->Clear();
        }
        render_to_screen();
    }
    
    gl_Video->Play();
    const int old_fps = get_game_fps();
    set_game_speed(gl_Video->GetVideoPlayer()->GetFramerate());
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
    set_game_speed(old_fps);
    gl_Video.reset();

    // Clear the screen after stopping playback
    // TODO: needed for FLIC, but perhaps may be done differently
    if ((state_flags & kVideoState_ClearScreen) != 0)
    {
        if (gfxDriver->UsesMemoryBackBuffer())
        {
            gfxDriver->GetMemoryBackBuffer()->Clear();
        }
        render_to_screen();
    }

    invalidate_screen();
    ags_clear_input_state();
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

    video_run(std::make_unique<FlicPlayer>(), flicname, video_flags, state_flags, skip);
    return HError::None();
}

HError play_theora_video(const char *name, int video_flags, int state_flags, VideoSkipType skip)
{
    video_run(std::make_unique<TheoraPlayer>(), name, video_flags, state_flags, skip);
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

void video_shutdown()
{
    gl_Video.reset();
}

#else

void play_theora_video(const char *name, int video_flags, int state_flags, AGS::Engine::VideoSkipType skip) {}
void play_flc_video(int numb, int video_flags, int state_flags, AGS::Engine::VideoSkipType skip) {}
void video_pause() {}
void video_resume() {}
void video_shutdown() {}

#endif
