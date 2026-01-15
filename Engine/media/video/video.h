//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// Video playback logical frontend. Serves as a control layer between game
// script and working video thread(s). Synchronizes logical and real playback
// states once during each game update: this is done to guarantee that the
// video state is fixed within a script callback.
//
// Videos play in their own FPS rate by default, which typically may be
// different from the game's, but the frame that the game is displaying
// on screen is updated in game's FPS rate. This also means that if a video
// has higher FPS than the game's, then there will be visible frame skips.
// If video's FPS is lower, then the same video frame will simply be displayed
// for more than 1 game's frame (not a bad thing).
//
// There are two kinds of playback controls below: blocking and non-blocking.
//
// Blocking video is a legacy interface left for backwards compatibility.
// It completely stops game updates for the duration of video (optionally with
// exception of game's audio track). No game elements are drawn while it plays
// (neither GUI, nor even a mouse cursor).
//
// Non-blocking video interface is a new default, which supports full playback
// control, and lets assign a video frame to any game object on screen.
// Game script is responsible for anything that happens during playback.
//
//-----------------------------------------------------------------------------
//
// TODO:
//     - in the engine support tagging sprites as "opaque" so that the renderer
//       would know to use simpler texture update method (video frames do not
//       have transparency in them... or can they?.. this may depend on info
//       retrieved from the video file too).
//
// TODO: POTENTIAL ALTERNATIVES (for consideration)
//     - current implementation of a non-blocking video provides a sprite
//       as a raw bitmap that may be updated to a texture. This lets to assign
//       video frame on any object. But a downside is the increased complexity,
//       which may make optimizing for performance somewhat more difficult.
//       An alternate implementation could instead render video frames on a
//       specialized game object, e.g. VideoOverlay, a subclass of Overlay.
//
//=============================================================================
#ifndef __AGS_EE_MEDIA__VIDEO_H
#define __AGS_EE_MEDIA__VIDEO_H

#include "gfx/bitmap.h"
#include "media/audio/audiodefines.h"
#include "util/geometry.h"
#include "util/string.h"
#include "util/error.h"

namespace AGS
{
namespace Engine
{

// Flags which define behavior of a blocking video "game state"
enum VideoStateFlags
{
    // Force-clear screen on begin and end (meant for software render)
    kVideoState_ClearScreen    = 0x0001,
    // Stretch video to the game screen
    kVideoState_Stretch        = 0x0002,
    // Stop game audio and restore after video finishes playing
    kVideoState_StopGameAudio  = 0x0004,
    // Increase game FPS to match video's
    kVideoState_SetGameFps     = 0x0008
};

// Flags which define a skipping method for the blocking video playback
enum VideoSkipType
{
    VideoSkipNone         = 0,
    VideoSkipEscape       = 1,
    VideoSkipAnyKey       = 2,
    VideoSkipKeyOrMouse   = 3
};

} // namespace Engine
} // namespace AGS


// Legacy Blocking video API
//
// Start a blocking OGV playback
AGS::Common::HError play_theora_video(const AGS::Common::String &name, int video_flags, int state_flags, AGS::Engine::VideoSkipType skip);
// Start a blocking FLIC playback
AGS::Common::HError play_flc_video(int numb, int video_flags, int state_flags, AGS::Engine::VideoSkipType skip);
// Pause the active blocking video
void video_single_pause();
// Resume the active blocking video
void video_single_resume();
// Stop current blocking video playback and dispose all video resource
void video_single_stop();

// Non-blocking video API
//
class VideoControl
{
public:
    VideoControl(int video_id, int sprite_id);
    ~VideoControl();

    // Gets if the video is valid (playing or ready to play)
    bool IsReady() const
    {
        return IsPlaybackReady(_state);
    }

    int GetVideoID() const { return _videoID; }
    int GetSpriteID() const { return _spriteID; }
    int GetScriptHandle() const { return _scriptHandle; }

    void SetScriptHandle(int sc_handle);

    int GetFrame() const { return _frameIndex; }
    int GetFrameCount() const { return _frameCount; }
    float GetFrameRate() const { return _frameRate; }
    Size GetFrameSize() const { return _frameRes; }
    float GetDurationMs() const { return _durMs; }
    float GetPositionMs() const { return _posMs; }
    bool GetLooping() const { return _looping; }
    PlaybackState GetState() const { return _state; }
    float GetSpeed() const { return _speed; }
    int GetVolume() const { return _volume; }

    void SetLooping(bool looping)
    {
        _looping = looping;
        _paramsChanged = true;
    }

    void SetSpeed(float speed)
    {
        _speed = speed;
        _paramsChanged = true;
    }

    void SetVolume(int vol)
    {
        _volume = vol;
        _paramsChanged = true;
    }

    bool Play();
    void Pause();
    bool NextFrame();
    uint32_t SeekFrame(uint32_t frame);
    float SeekMs(float pos_ms);

    // Synchronize VideoControl with the video playback subsystem;
    // - start scheduled playback;
    // - apply all accumulated sound parameters;
    // - read and save current position;
    // Returns if the clip is still playing, otherwise it's finished
    bool Update();

private:
    std::unique_ptr<AGS::Common::Bitmap> SetNewFrame(std::unique_ptr<AGS::Common::Bitmap> frame);

    const int _videoID;
    const int _spriteID;
    int       _scriptHandle = -1;
    Size      _frameRes;
    float     _frameRate = 0.f;
    float     _durMs = 0.f;
    uint32_t  _frameCount = 0;
    int       _volume = 100;
    float     _speed = 1.f;
    bool      _looping = false;
    PlaybackState _state = PlayStateInitial;
    uint32_t  _frameIndex = 0;
    float     _posMs = 0.f;
    bool      _paramsChanged = false;
};

// open_video starts video and returns a VideoControl object
// associated with it.
AGS::Common::HError open_video(const char *name, int video_flags, int &video_id);
VideoControl *get_video_control(int video_id);
void video_stop(int video_id);
// syncs logical video objects with the video core state
void sync_video_playback();
// update non-blocking videos
void update_video_system_on_game_loop();
// Stop all videos and video thread
void video_shutdown();

#endif // __AGS_EE_MEDIA__VIDEO_H
