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
//
// Video playback interface.
//
// TODO: good future changes:
//  - do not render to the screen right inside the VideoPlayer class,
//    instead write the frame into the bitmap or texture, and expose
//    current frame in the interface to let the engine decide what to do
//    with it.
//
//=============================================================================
#ifndef __AGS_EE_MEDIA__VIDEO_H
#define __AGS_EE_MEDIA__VIDEO_H
#include <atomic>
#include "media/audio/openalsource.h"
#include "util/geometry.h"
#include "util/string.h"
#include "util/error.h"

namespace AGS
{

namespace Common { class Bitmap; }

namespace Engine
{

enum VideoFlags
{
    kVideo_EnableVideo    = 0x0001,
    kVideo_Stretch        = 0x0002,
    kVideo_ClearScreen    = 0x0004,
    kVideo_EnableAudio    = 0x0010,
    kVideo_KeepGameAudio  = 0x0020
};

enum VideoSkipType
{
    VideoSkipNone         = 0,
    VideoSkipEscape       = 1,
    VideoSkipAnyKey       = 2,
    VideoSkipKeyOrMouse   = 3
};

class IDriverDependantBitmap;
using AGS::Common::Bitmap;
using AGS::Common::String;

// Parent video player class, provides basic playback logic,
// queries audio and video frames from decoders, plays audio chunks,
// renders the video frames on screen, according to the stretch flags.
// Relies on frame decoding being implemented in derived classes.
class VideoPlayer
{
public:
    virtual ~VideoPlayer();
    // Tries to open a video file of a given name
    Common::HError Open(const Common::String &name, int flags);
    virtual bool IsValid() { return false; }
    // Stops the playback, releasing any resources
    void Close();
    // Begins or resumes playback
    void Play();
    // Pauses playback
    void Pause();
    // Restores the video after display switch
    virtual void Restore() {};

    // Get suggested video framerate (frames per second)
    uint32_t GetFramerate() const { return _frameRate; }
    // Tells if video playback is looping
    bool IsLooping() const { return _loop; }
    // Get current playback state
    PlaybackState GetPlayState() const { return _playState; }

    // Updates the video playback, renders next frame
    bool Poll();

protected:
    // Opens the video, implementation-specific; allows to modify flags
    virtual Common::HError OpenImpl(const String& /*name*/, int& /*flags*/)
        { return new Common::Error("Internal error: operation not implemented"); };
    // Closes the video, implementation-specific
    virtual void CloseImpl() {};
    // Retrieves next video frame, implementation-specific
    virtual bool NextFrame() { return false; };

    int GetAudioPos(); // in ms

    int _audioChannels = 0;
    int _audioFreq = 0;
    SDL_AudioFormat _audioFormat = 0;
    SoundBuffer _audioFrame{};
    bool _wantAudio = false;

    std::unique_ptr<Bitmap> _videoFrame;
    int _frameDepth = 0; // bits per pixel
    Size _frameSize{};
    uint32_t _frameRate = 0u;

private:
    // Renders the current audio data
    bool RenderAudio();
    // Renders the current video frame
    bool RenderVideo();
    // Resumes after pausing
    void Resume();

    // Parameters
    bool _loop = false;
    int _flags = 0;
    // Playback state
    uint32_t _frameTime = 0u; // frame duration in ms
    PlaybackState _playState = PlayStateInitial;
    // Audio
    std::unique_ptr<OpenAlSource> _audioOut;
    // Video
    Rect _dstRect{};
    std::unique_ptr<Bitmap> _hicolBuf;
    std::unique_ptr<Bitmap> _targetBitmap;
    IDriverDependantBitmap *_videoDDB = nullptr;
};

} // namespace Engine
} // namespace AGS


AGS::Common::HError play_theora_video(const char *name, int flags, AGS::Engine::VideoSkipType skip);
AGS::Common::HError play_flc_video(int numb, int flags, AGS::Engine::VideoSkipType skip);

// Pause the active video
void video_pause();
// Resume the active video
void video_resume();
// Update video playback if the display mode has changed
void video_on_gfxmode_changed();
// Stop current playback and dispose all video resource
void video_shutdown();

#endif // __AGS_EE_MEDIA__VIDEO_H
