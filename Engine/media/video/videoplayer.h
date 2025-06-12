//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
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
// TODO:
//     - allow skip frames if late (add to settings);
//     - a video-audio sync mechanism; perhaps rely on audio,
//       because it's more time-sensitive in human perception;
//       drop video frames if video is lagging, but this also has to
//       be done in decoder to avoid converting vframe to a bitmap.
//     - other options: slow down playback speed until video-audio
//       relation stabilizes.
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
#include "util/time_util.h"

namespace AGS
{
namespace Engine
{

enum VideoFlags
{
    kVideo_EnableVideo    = 0x0001,
    kVideo_EnableAudio    = 0x0002,
    kVideo_Loop           = 0x0004,
    kVideo_LegacyFrameSize= 0x0008,
    // Allow to drop late video frames in autoplay mode
    kVideo_DropFrames     = 0x0010,
    // Try sync audio and video in case any of these have advanced
    // or fell back too much (this may halt or fastforward video
    // frames, but audio is never touched, as gaps in audio are
    // far more noticeable to human perception compared to video.
    kVideo_SyncAudioVideo = 0x0020,
    // Must accumulate decoded frames, when format's frames
    // do not have a full image, but diff from the previous frame
    kVideo_AccumFrame     = 0x0040,
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
    // Seek to the given time position; returns new pos or -1 on error
    float Seek(float pos_ms);
    // Seek to the given frame; returns new pos or -1 (UINT32_MAX) on error
    uint32_t SeekFrame(uint32_t frame);
    // Steps one frame forward, returns a prepared video frame on success
    std::unique_ptr<Common::Bitmap> NextFrame();

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
    float GetDurationMs() const { return _durationMs; }
    // Gets playback position, in ms
    float GetPositionMs() const { return _posMs; }
    // Gets current video frame index
    uint32_t GetFrameIndex() const { return _frameIndex; }

    void  SetSpeed(float speed);
    void  SetVolume(float volume);
    void  SetLooping(bool loop) { _flags = (_flags & ~kVideo_Loop) | (kVideo_Loop * loop); }

    // Retrieve the currently prepared video frame
    std::unique_ptr<Common::Bitmap> GetReadyFrame();
    // Tell VideoPlayer that this frame is not used anymore, and may be recycled
    // TODO: redo this part later, by introducing some kind of a RAII lock wrapper.
    void ReleaseFrame(std::unique_ptr<Common::Bitmap> frame);

    // Updates the video playback, prepares next video & audio frames for the render
    bool Poll();

protected:
    // Opens the video, implementation-specific; allows to modify flags
    virtual Common::HError OpenImpl(std::unique_ptr<Common::Stream> /*data_stream*/,
        const String &/*name*/, int& /*flags*/, int /*target_depth*/)
        { return new Common::Error("Internal error: operation not implemented"); };
    // Closes the video, implementation-specific
    virtual void CloseImpl() {};
    // Rewind to the start
    virtual bool RewindImpl() { return false; }
    // Retrieves next video frame, implementation-specific
    virtual bool NextVideoFrame(Common::Bitmap *dst, float &ts) { return false; };
    // Retrieves next audio frame, implementation-specific
    // TODO: change return type to a proper allocated buffer
    // when we support a proper audio queue here.
    virtual bool NextAudioFrame(SoundBuffer &abuf) { return false; };

    // Audio internals
    int _audioChannels = 0;
    int _audioFreq = 0;
    SDL_AudioFormat _audioFormat = 0;

    // Video internals
    // Native video frame's format
    int _frameDepth = 0; // bits per pixel
    Size _frameSize = {};
    float _frameRate = 0.f;
    float _frameTime = 0.f;
    uint32_t _frameCount = 0; // total number of frames in video (if available)
    float _durationMs = 0.f;

private:
    struct VideoFrame
    {
        VideoFrame() = default;
        VideoFrame(std::unique_ptr<Common::Bitmap> &&bmp, float ts = -1.f)
            : _bmp(std::move(bmp)), _ts(ts) {}

        const Common::Bitmap *Bitmap() const { return _bmp.get(); }
        float Timestamp() const { return _ts; }
        void SetTimestamp(float ts) { _ts = ts; }

        std::unique_ptr<Common::Bitmap> Retrieve()
        {
            return std::move(_bmp);
        }

    private:
        std::unique_ptr<Common::Bitmap> _bmp;
        float _ts = -1.f; // negative means undefined
    };

    // Rewind the stream to start and reset playback pos
    bool Rewind();
    // Resume after pause
    void ResumeImpl();
    // Read and queue video frames
    void BufferVideo();
    // Read and queue audio frames
    void BufferAudio();
    // Update statistic records
    void UpdateStats();
    // Update playback timing
    void UpdatePlayTime();
    // Tries to synchronize video and audio outputs
    void SyncVideoAudio();
    // Retrieve first available frame from queue,
    // advance output frame counter
    std::unique_ptr<VideoFrame> NextFrameFromQueue();
    // Process buffered video frame(s);
    // returns if should continue working
    bool ProcessVideo();
    // Process buffered audio frame(s);
    // returns if should continue working
    bool ProcessAudio();
    // Updates the video playback, prepares next video & audio frames for the render
    bool PollImpl();
    // Prints accumulated statistics into the log
    void PrintStats(bool close);

    // Parameters
    String _name;
    int _flags = 0;
    // Output video frame's color depth and size
    Size _targetSize;
    int _targetDepth = 0;
    float _targetFPS = 0.f;
    float _targetFrameTime = 0.f; // frame duration in ms for "target fps"
    uint32_t _queueMax = 5u; // measured in video frame times!
    float _queueTimeMax = 0.f;

    // Playback state
    PlaybackState _playState = PlayStateInitial;
    // Actual playback position; depends on both video and audio playback counters
    float _posMs = 0.f;
    // Video playback position (using 1 frame precision), increments with playback, resets on rewind or seek
    float _videoPosMs = 0.f;
    // Current video frame index (sequential since the video beginning);
    // UINT32_MAX means undefined frame
    uint32_t _frameIndex = UINT32_MAX;
    // Audio playback position, increments with playback, resets on rewind or seek
    float _audioPosMs = 0.f;
    // Stage timestamps, used to calculate the next frame timing;
    // note that these are "virtual time", and are adjusted whenever playback
    // is paused and resumed, or playback speed changes.
    // The start timestamp is a checkpoint from which the optimal playback position
    // is calculated. But it's a "virtual" time, not a real time (it may or not
    // match the real time). It's reset when the video is paused and resumed, and
    // also may be adjusted when the playback speed changes or for synchronization
    // purposes.
    Clock::time_point _startTs;
    Clock::time_point _pollTs; // timestamp of the last Poll in autoplay mode
    bool _resetStartTime = false; // an instruction to reset start ts on the next poll
    Clock::time_point _pauseTs; // time when the playback was paused
    uint32_t _inputFrameCount = 0u; // how many frames received on input
    float _inputAudioDurMs = 0.f; // how much audio received on input
    //float _nextVideoTs = 0.f; // expected next video timestamp for output
    //float _nextAudioTs = 0.f; // expected next audio timestamp for output
    Clock::duration _playbackDuration; // playback time passed since start timestamp
    float _playbackDurationMs = 0.f;
    // Audio
    // Buffered audio queue and pool
    std::stack<std::unique_ptr<SoundBuffer>> _audioFramePool;
    std::deque<std::unique_ptr<SoundBuffer>> _audioFrameQueue;
    float _audioQueueDurMs = 0.f; // accumulated duration of audio queue
    // Audio output object
    std::unique_ptr<OpenAlSource> _audioOut;
    // Video
    // Helper buffer for retrieving video frames of different size/depth;
    // should match "native" video frame size and color depth
    std::unique_ptr<Common::Bitmap> _vframeBuf;
    // Helper buffer for copying 8-bit frames to the final frame
    std::unique_ptr<Common::Bitmap> _hicolBuf;
    // Buffered frame queue and pool
    std::stack<std::unique_ptr<Common::Bitmap>> _videoFramePool;
    std::deque<std::unique_ptr<VideoFrame>> _videoFrameQueue;

    // Statistics
    struct Statistics
    {
        struct ProcStat
        {
            uint32_t Frames = 0u; // number of frames processed
            uint64_t TotalDataSz = 0u; // amount of data in frames in bytes
            float    TotalDurMs = 0.f;  // amount of media data in milliseconds
            uint32_t Dropped = 0u; // number of discarded frames

            uint32_t RawDecodedDataSz = 0u; // size of the raw frame (decoded)
            uint32_t RawDecodedConvDataSz = 0u; // size of the raw frame (decoded + converted)
            uint32_t MaxTimePerFrame = 0u; // max time spent on a frame
            uint32_t AvgTimePerFrame = 0u; // average time spent on a frame
            uint64_t TotalTime = 0u; // total time spent on input frames
        };

        Clock::time_point LastWorkTs = {}; // last time when the work time was updated
        Clock::time_point LastPlayTs = {}; // last time when the play time was updated
        Clock::duration WorkTime = {}; // real time this player was working (all time, including delays between frames)
        Clock::duration PlayTime = {}; // real time this player was in a playback state (not paused)
        ProcStat VideoIn; // amount of video data received on input
        ProcStat VideoOut; // amount of video data passed on output (to render)
        ProcStat AudioIn; // amount of audio data received on input
        ProcStat AudioOut; // amount of audio data passed on output (to render)
        uint32_t MaxBufferedVideo = 0u; // number of frames
        float MaxBufferedAudioMs = 0.f; // duration
        uint32_t BufferedVideoAccum = 0u;
        float BufferedAudioAcum = 0.f;
        float VideoTimingDiffAccum = 0.f;
        std::pair<float, float> VideoTimingDiffs;
        float AudioTimingDiffAccum = 0.f;
        std::pair<float, float> AudioTimingDiffs;
        uint32_t LastSyncRecordFrame = UINT32_MAX; // which video frame we recorded avg sync times
        std::pair<float, float> SyncTimingDiffs;
        float SyncTimingDiffAccum = 0.f;
        float SyncMaxFw = 0.f;
        float SyncMaxBw = 0.f;
    } _stats;

    static const uint32_t PrintStatsEachMs = 0u;
    bool _statsReady = false;
    Clock::time_point _statsPrintTs = {};
};

} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_MEDIA__VIDEOPLAYER_H
