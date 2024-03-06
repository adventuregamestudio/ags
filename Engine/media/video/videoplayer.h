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
//
// Video playback interface.
// Renders video frames onto bitmap buffer; supports double-buffering,
// where active bitmap is switched each next time.
// Renders audio frames using OpenAlSource output.
//
// TODO: separate Video Decoder class, would be useful e.g. for plugins.
//
//=============================================================================
#ifndef __AGS_EE_MEDIA__VIDEOPLAYER_H
#define __AGS_EE_MEDIA__VIDEOPLAYER_H

#include <deque>
#include <memory>
#include <stack>
#include "ac/timer.h"
#include "gfx/bitmap.h"
#include "media/audio/audiodefines.h"
#include "media/audio/openalsource.h"
#include "util/error.h"
#include "util/stream.h"

namespace AGS
{
namespace Engine
{

enum VideoFlags
{
    kVideo_EnableVideo    = 0x0001,
    kVideo_EnableAudio    = 0x0002,
    kVideo_Loop           = 0x0004,
    kVideo_LegacyFrameSize= 0x0008
};

// Parent video player class, provides basic playback logic,
// queries audio and video frames from decoders, plays audio chunks,
// renders the video frames on bitmap.
// Relies on frame decoding being implemented in derived classes.
class VideoPlayer
{
public:
    virtual ~VideoPlayer();

    // Tries to init a video playback reading from the given stream
    Common::HError Open(std::unique_ptr<Common::Stream> data_stream,
        const String &name, int flags);
    Common::HError Open(std::unique_ptr<Common::Stream> data_stream,
        const String &name, int flags,
        const Size &target_sz, int target_depth = 0, float target_fps = 0.f);
    // Tells if the videoplayer object is valid and ready to render
    virtual bool IsValid() { return false; }
    bool HasVideo() const { return (_flags & kVideo_EnableVideo) != 0; }
    bool HasAudio() const { return (_flags & kVideo_EnableAudio) != 0; }
    // Assigns a wanted target bitmap size
    void SetTargetFrame(const Size &target_sz);
    // Begins or resumes playback
    void Play();
    // Pauses playback
    void Pause();
    // Stops the playback, releasing any resources
    void Stop();
    // Seek to the given time position
    void Seek(float pos_ms);

    const String &GetName() const { return _name; }
    int GetFrameDepth() const { return _frameDepth; }
    const Size &GetFrameSize() const { return _frameSize; }
    int GetTargetDepth() const { return _targetDepth; }
    const Size &GetTargetSize() const { return _targetSize; }
    // Get suggested video framerate (frames per second)
    float GetFramerate() const { return _frameRate; }
    // Tells if video playback is looping
    bool IsLooping() const { return (_flags & kVideo_Loop) != 0; }
    // Get current playback state
    PlaybackState GetPlayState() const { return _playState; }
    // Gets duration, in ms
    float GetDurationMs() const { return 0 /* TODO */; }
    // Gets playback position, in ms
    float GetPositionMs() const { return 0 /* TODO */; }

    void  SetSpeed(float speed);
    void  SetVolume(float volume);

    // Retrieve the currently prepared video frame
    std::unique_ptr<Common::Bitmap> GetReadyFrame();
    // Tell VideoPlayer that this frame is not used anymore, and may be recycled
    // TODO: redo this part later, by introducing some kind of a RAII lock wrapper.
    void ReleaseFrame(std::unique_ptr<Common::Bitmap> frame);

    // Updates the video playback, renders next frame
    bool Poll();

protected:
    // Opens the video, implementation-specific; allows to modify flags
    virtual Common::HError OpenImpl(std::unique_ptr<Common::Stream> /*data_stream*/,
        const String &/*name*/, int& /*flags*/, int /*target_depth*/)
        { return new Common::Error("Internal error: operation not implemented"); };
    // Closes the video, implementation-specific
    virtual void CloseImpl() {};
    // Retrieves next video frame, implementation-specific
    virtual bool NextVideoFrame(Common::Bitmap *dst) { return false; };
    // Retrieves next audio frame, implementation-specific
    // TODO: change return type to a proper allocated buffer
    // when we support a proper audio queue here.
    virtual SoundBuffer NextAudioFrame() { return SoundBuffer(); };

    // Audio internals
    int _audioChannels = 0;
    int _audioFreq = 0;
    SDL_AudioFormat _audioFormat = 0;

    // Video internals
    // Native video frame's format
    int _frameDepth = 0; // bits per pixel
    Size _frameSize{};
    float _frameRate = 0.f;

private:
    // Resume after pause
    void ResumeImpl();
    // Read and queue video frames
    void BufferVideo();
    // Read and queue audio frames
    void BufferAudio();
    // Process buffered video frame(s);
    // returns if should continue working
    bool ProcessVideo();
    // Process buffered audio frame(s);
    // returns if should continue working
    bool ProcessAudio();

    // Parameters
    String _name;
    int _flags = 0;
    // Output video frame's color depth and size
    Size _targetSize;
    int _targetDepth = 0;
    float _targetFPS = 0.f;
    uint32_t _videoQueueMax = 5u;
    uint32_t _audioQueueMax = 0u; // we don't have a real queue atm
    // Playback state
    PlaybackState _playState = PlayStateInitial;
    uint32_t _framesPlayed = 0u;
    // Stage timestamps, used to calculate the next frame timing;
    // note that these are "virtual time", and are adjusted whenever playback
    // is paused and resumed, or playback speed changes.
    AGS_Clock::time_point _firstFrameTime; // time when the first frame was ready
    AGS_Clock::time_point _pauseTime; // time when the playback was paused
    // Audio
    // Audio queue (single frame for now, because output buffers too)
    SoundBuffer _audioFrame{};
    // Audio output object
    std::unique_ptr<OpenAlSource> _audioOut;
    // Video
    float _frameTime = 0.f; // frame duration in ms
    // Helper buffer for retrieving video frames of different size/depth;
    // should match "native" video frame size and color depth
    std::unique_ptr<Common::Bitmap> _vframeBuf;
    // Helper buffer for copying 8-bit frames to the final frame
    std::unique_ptr<Common::Bitmap> _hicolBuf;
    // Buffered frame queue
    std::stack<std::unique_ptr<Common::Bitmap>> _videoFramePool;
    std::deque<std::unique_ptr<Common::Bitmap>> _videoFrameQueue;
    // Video frame prepared for the user
    std::unique_ptr<Common::Bitmap> _videoReadyFrame;
};

} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_MEDIA__VIDEOPLAYER_H
