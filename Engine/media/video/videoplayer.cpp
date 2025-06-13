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
#ifndef AGS_NO_VIDEO_PLAYER
#include "media/video/videoplayer.h"
#include "debug/out.h"
#include "util/memory_compat.h"

#define VIDEO_DEBUG_VERBOSE     (0)
#define VIDEO_TEST_DESYNC       (0)

namespace AGS
{
namespace Engine
{

using namespace Common;

VideoPlayer::~VideoPlayer()
{
    Stop();
}

HError VideoPlayer::Open(std::unique_ptr<Common::Stream> data_stream,
    const String &name, int flags)
{
    return Open(std::move(data_stream), name, flags, Size(), 0);
}

HError VideoPlayer::Open(std::unique_ptr<Common::Stream> data_stream,
    const String &name, int flags,
    const Size &target_sz, int target_depth, float target_fps)
{
    // We request a target depth from decoder, but it may ignore our request,
    // so we have to check actual "native" frame's depth after
    HError err = OpenImpl(std::move(data_stream), name, flags, target_depth);
    if (!err)
        return err;

    _name = name;
    _flags = flags;
    _targetFPS = target_fps > 0.f ? target_fps : _frameRate;
    // Start the audio stream
    if (HasAudio())
    {
        if ((_audioFormat > 0) && (_audioChannels > 0) && (_audioFreq > 0))
        {
            _audioOut.reset(new OpenAlSource(_audioFormat, _audioChannels, _audioFreq));
        }
    }
    // Setup video
    if (HasVideo())
    {
        _targetDepth = target_depth > 0 ? target_depth : _frameDepth;
        SetTargetFrame(target_sz);
    }

    // TODO: actually support dynamic FPS, need to adjust audio speed
    _targetFrameTime = 1000.f / _targetFPS;
    _queueTimeMax = _queueMax * _targetFrameTime;
    _resetStartTime = true;
    return HError::None();
}

void VideoPlayer::SetTargetFrame(const Size &target_sz)
{
    _targetSize = target_sz.IsNull() ? _frameSize : target_sz;

    // Create helper bitmaps in case of stretching or color depth conversion
    if ((_targetSize != _frameSize) || (_targetDepth != _frameDepth)
        || ((_flags & kVideo_AccumFrame) != 0))
    {
        _vframeBuf.reset(new Bitmap(_frameSize.Width, _frameSize.Height, _frameDepth));
    }
    else
    {
        _vframeBuf.reset();
    }

    // If we are decoding a 8-bit frame in a hi-color game, and stretching,
    // then create a hi-color buffer, as bitmap lib cannot stretch with depth change
    if ((_targetSize != _frameSize) && (_frameDepth == 8) && (_targetDepth > 8))
    {
        _hicolBuf.reset(BitmapHelper::CreateBitmap(_frameSize.Width, _frameSize.Height, _targetDepth));
    }
    else
    {
        _hicolBuf.reset();
    }

    // TODO: resize the buffered queue?

}

void VideoPlayer::Stop()
{
    if (IsPlaybackReady(_playState)) // keep any error state
    {
        Debug::Printf("VideoPlayer: Stop");
        _playState = PlayStateStopped;
    }

    // Shutdown openal source
    _audioOut.reset();
    // Close video decoder and free resources
    CloseImpl();

    _vframeBuf = nullptr;
    _hicolBuf = nullptr;
    _videoFramePool = std::stack<std::unique_ptr<Bitmap>>();
    _videoFrameQueue = std::deque<std::unique_ptr<VideoFrame>>();

    PrintStats(true);
    _statsReady = false;
}

void VideoPlayer::Play()
{
    if (!IsValid())
        return;

    switch (_playState)
    {
    case PlayStatePaused:
        Debug::Printf("VideoPlayer: Resume");
        ResumeImpl();
        /* fallthrough */
    case PlayStateInitial:
        Debug::Printf("VideoPlayer: Play");
        if (_audioOut)
            _audioOut->Play();
        _stats.LastPlayTs = Clock::now();
        if (_playState == PlayStateInitial)
        {
            _stats.LastWorkTs = _stats.LastPlayTs;
            _statsPrintTs = _stats.LastWorkTs;
        }
        _playState = PlayStatePlaying;
        _statsReady = true;
        break;
    default:
        break; // TODO: support rewind/replay from stop/finished state?
    }
}

void VideoPlayer::Pause()
{
    if (_playState != PlayStatePlaying)
        return;

    Debug::Printf("VideoPlayer: Pause");
    if (_audioOut)
        _audioOut->Pause();
    _playState = PlayStatePaused;
    _pauseTs = Clock::now();
}

float VideoPlayer::Seek(float pos_ms)
{
    Debug::Printf("VideoPlayer: Seek (%.2f)", pos_ms);
    if ((pos_ms == 0.f) && Rewind())
    {
        return 0.f;
    }
    return -1.f; // TODO
}

uint32_t VideoPlayer::SeekFrame(uint32_t frame)
{
    Debug::Printf("VideoPlayer: Seek Frame (%u)", frame);
    if ((frame == 0) && Rewind())
    {
        return 0u;
    }
    return UINT32_MAX; // TODO
}

std::unique_ptr<Bitmap> VideoPlayer::NextFrame()
{
    if (!IsPlaybackReady(_playState) || !HasVideo())
        return nullptr;
    if (_playState != PlaybackState::PlayStatePaused)
        Pause();

    BufferVideo();

    auto frame = NextFrameFromQueue();
    if (!frame)
    {
        // TODO: rewind should be done on reading from decoder, not when playing!
        // see how AudioPlayer does this
        if (IsLooping() && Rewind())
        {
            frame = NextFrameFromQueue();
        }
        else
        {
            _playState = PlayStateFinished;
            return nullptr;
        }
    }
    _posMs = std::max(_videoPosMs, _audioPosMs);
    return frame ? frame->Retrieve() : nullptr;
}

void VideoPlayer::SetSpeed(float speed)
{
    // Update our virtual "start time" to keep the proper frame timing
    Clock::time_point now{};
    Clock::duration play_dur{};
    switch (_playState)
    {
    case PlaybackState::PlayStatePlaying:
        now = Clock::now();
        play_dur = now - _startTs;
        break;
    case PlaybackState::PlayStatePaused:
        now = _pauseTs;
        play_dur = now - _startTs;
        break;
    default:
        break;
    }

    const auto old_frametime = _targetFrameTime;
    _targetFPS = _frameRate * speed;
    _targetFrameTime = 1000.f / _targetFPS;

    // Adjust our virtual timestamps by the difference between
    // previous play duration, and new duration calculated from the new speed.
    float ft_rel = _targetFrameTime / old_frametime;
    Clock::duration virtual_play_dur =
        Clock::duration((int64_t)(play_dur.count() * ft_rel));
    _startTs = now - virtual_play_dur;
    // Adjust timestamps in video and audio queue
    for (auto &f : _videoFrameQueue)
        f->SetTimestamp(f->Timestamp() * ft_rel);
    for (auto &f : _audioFrameQueue)
        f->SetTimestamp(f->Timestamp() * ft_rel);

    // Adjust the audio speed separately
    if (_audioOut)
        _audioOut->SetSpeed(speed);
}

void VideoPlayer::SetVolume(float volume)
{
    if (_audioOut)
        _audioOut->SetVolume(volume);
}

std::unique_ptr<Common::Bitmap> VideoPlayer::GetReadyFrame()
{
#if (VIDEO_DEBUG_VERBOSE)
    Debug::Printf("VIDEO READY FRAME: playdur = %.2f, queue: %u, head frame timestamp: %.2f",
                  _playbackDurationMs, _videoFrameQueue.size(), _videoFrameQueue.empty() ? -1.f : _videoFrameQueue.front()->Timestamp());
#endif

    if (_videoFrameQueue.empty())
        return nullptr; // no frames available

    if (_videoFrameQueue.front()->Timestamp() > _playbackDurationMs)
        return nullptr; // not the time yet

#if (VIDEO_TEST_DESYNC)
    // FIXME: this is for testing audio desync with video -- remove later
    static int skip_video_instance = 0;
    static int skip_video_counter = 0;
    if (skip_video_instance % 10 == 0)
    {
        if (++skip_video_counter < 33)
            return nullptr;
        skip_video_counter = 0;
    }
    skip_video_instance++;
#endif // VIDEO_TEST_DESYNC

    auto frame = NextFrameFromQueue();
    return frame ? frame->Retrieve() : nullptr;
}

void VideoPlayer::ReleaseFrame(std::unique_ptr<Common::Bitmap> frame)
{
    _videoFramePool.push(std::move(frame));
}

bool VideoPlayer::Poll()
{
    if (!IsPlaybackReady(_playState))
        return false;

    const bool res = PollImpl();
    // Update stats after polling once
    UpdateStats();
    return res;
}

bool VideoPlayer::PollImpl()
{
    // Buffer always when ready, even if we are paused
    if (HasVideo())
        BufferVideo();
    if (HasAudio())
        BufferAudio();

    if (_playState != PlayStatePlaying)
        return false;

    UpdatePlayTime();

    if (HasVideo() && HasAudio() && ((_flags & kVideo_SyncAudioVideo) != 0))
        SyncVideoAudio();

    bool res_video = HasVideo() && ProcessVideo();
    bool res_audio = HasAudio() && ProcessAudio();

    _posMs = std::max(_videoPosMs, _audioPosMs);

    // Stop if nothing is left to process, or if there was error
    if (_playState == PlayStateError)
        return false;
    if (!res_video && !res_audio)
    {
        // TODO: rewind should be done on reading from decoder, not when playing!
        // see how AudioPlayer does this
        if (IsLooping() && Rewind())
        {
            return true;
        }
        else
        {
            _playState = PlayStateFinished;
            return false;
        }
    }
    return true;
}

bool VideoPlayer::Rewind()
{
    if (!RewindImpl())
        return false;

    // TODO: this cannot be done on Rewind itself if we rewind not after
    // everything is played, but after everything is buffered!
    // See how this is implemented in the AudioPlayer!
    _resetStartTime = true;
    _startTs = Clock::now();
    _pauseTs = _startTs;
    _inputFrameCount = 0u;
    _inputAudioDurMs = 0.f;
    _playbackDuration = Clock::duration();
    _playbackDurationMs = 0.f;
    _posMs = 0.f;
    _videoPosMs = 0.f;
    _frameIndex = UINT32_MAX;
    _audioPosMs = 0.f;
    // Stats
    _stats.LastSyncRecordFrame = UINT32_MAX;
    return true;
}

void VideoPlayer::ResumeImpl()
{
    // Update our virtual "start time" to keep the proper frame timing
    auto pause_dur = Clock::now() - _pauseTs;
    _startTs += pause_dur;

    // TODO: Separate case of resuming after NextFrame,
    // must Seek to frame, or audio will fall behind
}

void VideoPlayer::BufferVideo()
{
    if (_videoFrameQueue.size() >= _queueMax)
        return; // queue limit reached

    // Optionally drop late frames, but have at least 1 for display
    if (((_flags & kVideo_DropFrames) != 0) && ((_flags & kVideo_DropFramesUndecoded) != 0)
        && _videoFrameQueue.size() > 0)
    {
        const float drop_time = _playbackDurationMs - _targetFrameTime;
        float frame_ts = PeekVideoFrame();

        if ((frame_ts >= 0.f && frame_ts < drop_time))
        {
            DropVideoFrame();
#if (VIDEO_DEBUG_VERBOSE)
            Debug::Printf("DROPPED LATE FRAME (UNDECODED), ts: %.2f, drop time: %.2f, queue size now: %u",
                          frame_ts, drop_time, _videoFrameQueue.size());
#endif
            _stats.VideoOut.Dropped++;
        }
    }

    // Get one frame from the pool, if present, otherwise allocate a new one
    std::unique_ptr<Bitmap> target_frame;
    if (_videoFramePool.empty())
    {
        target_frame.reset(new Bitmap(_targetSize.Width, _targetSize.Height, _targetDepth));
    }
    else
    {
        target_frame = std::move(_videoFramePool.top());
        _videoFramePool.pop();
    }

    const auto input_start = Clock::now();

    // Try to retrieve one video frame from decoder
    const bool must_conv = (_targetSize != _frameSize || _targetDepth != _frameDepth
        || ((_flags & kVideo_AccumFrame) != 0));
    Bitmap *usebuf = must_conv ? _vframeBuf.get() : target_frame.get();
    float frame_ts = -1.f;
    if (!NextVideoFrame(usebuf, frame_ts))
    {
        // failed to get frame, so move prepared target frame into the pool for now
        _videoFramePool.push(std::move(target_frame));
        return;
    }

    const auto decoded_frame_sz = usebuf->GetDataSize();
    // Convert frame_ts to our playback speed
    frame_ts = frame_ts * _targetFrameTime / _frameTime;
#if (VIDEO_DEBUG_VERBOSE)
    const float expect_frame_ts = _inputFrameCount * _targetFrameTime;
    Debug::Printf("INPUT VIDEO FRAME: expect ts = %.2f, given ts = %.2f, diff = %.2f", expect_frame_ts, frame_ts, expect_frame_ts - frame_ts);
#endif
    _inputFrameCount++;

    // Convert frame if necessary
    if (must_conv)
    {
        // Use intermediate hi-color buffer if necessary
        if (_hicolBuf)
        {
            _hicolBuf->Blit(usebuf);
            usebuf = _hicolBuf.get();
        }
        
        if (_targetSize == _frameSize)
            target_frame->Blit(usebuf);
        else
            target_frame->StretchBlt(usebuf, RectWH(_targetSize));
    }

    const auto input_dur_ms = ToMilliseconds(Clock::now() - input_start);

    // Stats
    assert(target_frame);
    _stats.VideoIn.Frames++;
    _stats.VideoIn.TotalDataSz += target_frame->GetDataSize();
    _stats.VideoIn.TotalDurMs += _frameTime;
    _stats.VideoIn.TotalTime += input_dur_ms;
    _stats.VideoIn.RawDecodedDataSz = decoded_frame_sz;
    _stats.VideoIn.RawDecodedConvDataSz = target_frame->GetDataSize();
    _stats.VideoIn.AvgTimePerFrame = _stats.VideoIn.TotalTime / _stats.VideoIn.Frames;
    _stats.VideoIn.MaxTimePerFrame = std::max(_stats.VideoIn.MaxTimePerFrame, static_cast<uint32_t>(input_dur_ms));
    _stats.MaxBufferedVideo = std::max<uint32_t>(_stats.MaxBufferedVideo, _videoFrameQueue.size());
    // TODO: maybe record this every 10 - 100 frames?
    _stats.BufferedVideoAccum += _videoFrameQueue.size();

    // Push final frame to the queue
    _videoFrameQueue.push_back(std::make_unique<VideoFrame>(std::move(target_frame), frame_ts));
}

void VideoPlayer::BufferAudio()
{
    if (_audioQueueDurMs >= _queueMax * _targetFrameTime)
        return; // queue limit reached

    // Get one frame from the pool, if present, otherwise allocate a new one
    std::unique_ptr<SoundBuffer> aframe;
    if (_audioFramePool.empty())
    {
        aframe.reset(new SoundBuffer());
    }
    else
    {
        aframe = std::move(_audioFramePool.top());
        _audioFramePool.pop();
    }

    const auto input_start = Clock::now();

    if (!NextAudioFrame(*aframe))
    {
        // failed to get frame, so move prepared frame into the pool for now
        _audioFramePool.push(std::move(aframe));
        return;
    }

    const auto input_dur_ms = ToMilliseconds(Clock::now() - input_start);

    // Stats
    assert(aframe);
    _stats.AudioIn.Frames++;
    _stats.AudioIn.TotalDataSz += aframe->Size();
    _stats.AudioIn.TotalDurMs += aframe->DurationMs();
    _stats.AudioIn.TotalTime += input_dur_ms;
    _stats.AudioIn.AvgTimePerFrame = _stats.AudioIn.TotalTime / _stats.AudioIn.Frames;
    _stats.AudioIn.MaxTimePerFrame = std::max(_stats.AudioIn.MaxTimePerFrame, static_cast<uint32_t>(input_dur_ms));
    _stats.MaxBufferedAudioMs = std::max(_stats.MaxBufferedAudioMs, _audioQueueDurMs);
    // TODO: maybe record this every 10 - 100 frames?
    _stats.BufferedAudioAcum += _audioQueueDurMs;

    // Push final frame to the queue
    _audioQueueDurMs += aframe->DurationMs();
    _audioFrameQueue.push_back(std::move(aframe));
}

void VideoPlayer::UpdateStats()
{
    const auto now = Clock::now();
    _stats.WorkTime += (now - _stats.LastWorkTs);
    _stats.LastWorkTs = now;
    if (_playState == PlayStatePlaying)
    {
        _stats.PlayTime += (now - _stats.LastPlayTs);
        _stats.LastPlayTs = now;
    }

    if (PrintStatsEachMs > 0u)
        PrintStats(false);
}

void VideoPlayer::UpdatePlayTime()
{
    const auto now = Clock::now();
    if (_resetStartTime)
    {
        _startTs = now;
        _resetStartTime = false;
    }
    _pollTs = now;
    _playbackDuration = _pollTs - _startTs;
    _playbackDurationMs = ToMillisecondsF(_playbackDuration);
    if (_audioOut)
    {
        _audioPosMs = _audioOut->GetPositionMs();
    }

#if (VIDEO_TEST_DESYNC)
    Debug::Printf("VIDEO TIME: playdur %.2f ms, target frametime %.2f ms, video_pos = %.2f ms (%+.2f), audio pos = %.2f ms (%+.2f), video buf = %u, audio buf = %.2f ms",
        _playbackDurationMs,
        _targetFrameTime,
        _videoPosMs,
        _videoPosMs - _playbackDurationMs,
        _audioPosMs,
        _audioPosMs - _playbackDurationMs,
        _videoFrameQueue.size(),
        _audioQueueDurMs
    );
#endif // VIDEO_TEST_DESYNC
}

void VideoPlayer::SyncVideoAudio()
{
    if (_videoFrameQueue.empty())
        return; // can happen e.g. if the video stream ended earlier than audio

    // Check if video and audio playback differ for more than a allowed limit
    const float av_diff = _videoPosMs - _audioPosMs;
    const float leeway = _targetFrameTime * 2;
    if (std::fabs(av_diff) > leeway)
    {
        // If video and audio differentiate in more than a suggested "leeway",
        // then adjust the virtual "start time":
        // if it is advanced, then the current video frame will halt for a while,
        // if it is rewinded, then some of the video frames may be dropped.
        const float adjust_playdur_ms = -std::trunc(av_diff / _targetFrameTime) * _targetFrameTime;
        // Clamp the play duration adjustment to the current video play pos,
        // because we do not want to cause an impression that playback was "rewinded".
        const Clock::duration min_playdur = std::min(_playbackDuration, // - compare vs playbackdur in case of rounding mistakes
                Clock::duration(std::chrono::microseconds(static_cast<int64_t>(_videoPosMs * 1000.f))));
        const Clock::duration adjusted_playdur = _playbackDuration + std::chrono::microseconds(static_cast<int64_t>(adjust_playdur_ms * 1000.f));
        const Clock::duration new_playdur = std::max(min_playdur, adjusted_playdur);
        const auto new_playdur_diff = ToMillisecondsF(new_playdur - _playbackDuration);
        // Recalculate start time and dependent variables
        _startTs = _pollTs - new_playdur;
        _playbackDuration = new_playdur;
        _playbackDurationMs = ToMillisecondsF(_playbackDuration);

        // Stats
        _stats.SyncMaxFw = std::max(_stats.SyncMaxFw, new_playdur_diff);
        _stats.SyncMaxBw = std::min(_stats.SyncMaxBw, new_playdur_diff);
#if (VIDEO_DEBUG_VERBOSE)
        Debug::Printf("VideoPlayer: sync at frame %u: v-a diff: %+.2f ms, leeway: %.2f ms, adjust play dur by %+.2f ms, video playdur now: %.2f (expect frame: %u)",
            _frameIndex == UINT32_MAX ? 0u : _frameIndex, av_diff, leeway, new_playdur_diff, _playbackDurationMs, static_cast<uint32_t>(_playbackDurationMs / _targetFrameTime));
#endif
    }

    // Stats
    if (_stats.LastSyncRecordFrame != _frameIndex)
    {
        _stats.SyncTimingDiffAccum += av_diff;
        _stats.LastSyncRecordFrame = _frameIndex;
    }
    _stats.SyncTimingDiffs.first = std::min(_stats.SyncTimingDiffs.first, av_diff);
    _stats.SyncTimingDiffs.second = std::max(_stats.SyncTimingDiffs.second, av_diff);
}

std::unique_ptr<VideoPlayer::VideoFrame> VideoPlayer::NextFrameFromQueue()
{
    if (_videoFrameQueue.empty())
        return nullptr;

    // For stats: remember a pos difference before getting new frame
    const int32_t video_diff = _videoPosMs - _playbackDurationMs;

    auto frame = std::move(_videoFrameQueue.front());
    _videoFrameQueue.pop_front();
    _frameIndex++;
    _videoPosMs = frame->Timestamp() + _frameTime;

    // Stats
    _stats.VideoOut.Frames++;
    _stats.VideoOut.TotalDataSz += frame->Bitmap()->GetDataSize();
    _stats.VideoOut.TotalDurMs += _frameTime;
    _stats.VideoTimingDiffAccum += video_diff;
    _stats.VideoTimingDiffs.first = std::min<int32_t>(_stats.VideoTimingDiffs.first, video_diff);
    _stats.VideoTimingDiffs.second = std::max<int32_t>(_stats.VideoTimingDiffs.second, video_diff);

    return frame;
}

bool VideoPlayer::ProcessVideo()
{
    // Optionally drop late frames, but leave at least 1 for display
    if ((_flags & kVideo_DropFrames) != 0)
    {
        const float drop_time = _playbackDurationMs - _targetFrameTime;
        while ((_videoFrameQueue.size() > 1) &&
            (_videoFrameQueue.front()->Timestamp() < drop_time))
        {
            auto frame = NextFrameFromQueue();
            assert(frame);
#if (VIDEO_DEBUG_VERBOSE)
            Debug::Printf("DROPPED LATE FRAME, ts: %.2f, drop time: %.2f, queue size now: %u",
                          frame->Timestamp(), drop_time, _videoFrameQueue.size());
#endif
            _videoFramePool.push(std::move(frame->Retrieve()));
            _stats.VideoOut.Dropped++;
        }
    }
    // We are good so long as there's a ready frame in queue
    return !_videoFrameQueue.empty();
}

bool VideoPlayer::ProcessAudio()
{
    assert(_audioOut);
    // If we have no audio in queue, then exit, but result depends on whether
    // there's still something being buffered by the audio output
    if (_audioFrameQueue.empty())
    {
        _audioOut->Poll();
        return !_audioOut->IsEmpty();
    }

#if (VIDEO_TEST_DESYNC)
    // FIXME: this is for testing audio desync with video -- remove later
    static int skip_audio_instance = 0;
    static int skip_audio_counter = 0;
    if (skip_audio_instance % 100 == 0)
    {
        if (++skip_audio_counter < 66)
            return true;
        skip_audio_counter = 0;
    }
    skip_audio_instance++;
#endif // VIDEO_TEST_DESYNC

    // Push as many frames as audio output can take at once
    do
    {
        auto aframe = std::move(_audioFrameQueue.front());
        _audioFrameQueue.pop_front();
        assert(aframe);

        if (_audioOut->PutData(*aframe) > 0u)
        {
            // Stats
            _stats.AudioOut.Frames++;
            _stats.AudioOut.TotalDataSz += aframe->Size();
            _stats.AudioOut.TotalDurMs += aframe->DurationMs();
            const float play_dur = ToMillisecondsF(_playbackDuration);
            const float audio_pos = _audioPosMs;
            const float audio_diff = audio_pos - play_dur;
            _stats.AudioTimingDiffAccum += audio_diff;
            _stats.AudioTimingDiffs.first = std::min<float>(_stats.AudioTimingDiffs.first, audio_diff);
            _stats.AudioTimingDiffs.second = std::max<float>(_stats.AudioTimingDiffs.second, audio_diff);

            // Push used frame back to the pool
            _audioQueueDurMs -= aframe->DurationMs();
            assert(_audioQueueDurMs >= 0.f);
            _audioFramePool.push(std::move(aframe));
        }
        else
        {
            // Cannot play more right now, put back to the queue (in front!)
            _audioFrameQueue.push_front(std::move(aframe));
            break;
        }
    } while (!_audioFrameQueue.empty());

    _audioOut->Poll();
    return true;
}

void VideoPlayer::PrintStats(bool close)
{
    if (!_statsReady)
        return;

    auto now = Clock::now();
    if (!close && (ToMilliseconds(now - _statsPrintTs) < PrintStatsEachMs))
        return;

    _statsPrintTs = now;
    Debug::Printf("VideoPlayer stats: \"%s\""
                  "\n\tresolution: %dx%d, fps: %.2f, frametime: %.2f ms"
                  "\n\taudio: %d Hz, chans: %d"
                  "\n\ttotal time working: %lld ms"
                  "\n\ttotal time playing: %lld ms"
                  "\n\tplayback position: %.2f ms"
                  "\n\tvideo input frames: %u"
                  "\n\t            total size: %llu bytes"
                  "\n\t            total duration: %.2f ms"
                  "\n\t            frames dropped: %u"
                  "\n\tvideo frame size (raw, decoded): %u bytes"
                  "\n\tvideo frame size (raw, final): %u bytes"
                  "\n\tmax time per input video frame: %u ms"
                  "\n\tavg time per input video frame: %u ms"
                  "\n\ttotal time on input video frames: %llu ms"
                  "\n\tmax buffered video frames: %u / %u"
                  "\n\tavg buffered video frames: %u"
                  "\n\tvideo output frames: %u"
                  "\n\t            total size: %llu bytes"
                  "\n\t            total duration: %.2f ms"
                  "\n\t            frames dropped: %u"
                  "\n\tmost video timing diff: %+.2f, %+.2f ms"
                  "\n\tavg video timing diff: %+.2f ms"
                  "\n\taudio input frames: %u"
                  "\n\t            total size: %llu bytes"
                  "\n\t            total duration: %.2f ms"
                  "\n\t            frames dropped: %u"
                  "\n\taverage audio frame: %u bytes, %.2f ms"
                  "\n\tmax time per input audio frame: %u ms"
                  "\n\tavg time per input audio frame: %u ms"
                  "\n\ttotal time on input audio frames: %llu ms"
                  "\n\tmax buffered audio duration: %.2f / %.2f ms"
                  "\n\tavg buffered audio duration: %.2f ms"
                  "\n\taudio output frames: %u"
                  "\n\t            total size: %llu bytes"
                  "\n\t            total duration: %.2f ms"
                  "\n\t            frames dropped: %u"
                  "\n\tmost audio timing diff: %+.2f, %+.2f"
                  "\n\tavg audio timing diff: %+.2f ms"
                  "\n\tvideo-audio sync:"
                  "\n\t            most desync towards audio: %+.2f ms"
                  "\n\t            most desync towards video: %+.2f ms"
                  "\n\t            avg desync: %+.2f ms"
                  "\n\t            max adjust forward: %+.2f ms"
                  "\n\t            max adjust backward: %+.2f ms",
                  _name.GetCStr(),
                  _frameSize.Width, _frameSize.Height, _frameRate, _frameTime,
                  _audioFreq, _audioChannels,
                  ToMilliseconds(_stats.WorkTime),
                  ToMilliseconds(_stats.PlayTime),
                  _posMs,
                  _stats.VideoIn.Frames,
                  _stats.VideoIn.TotalDataSz,
                  _stats.VideoIn.TotalDurMs,
                  _stats.VideoIn.Dropped,
                  _stats.VideoIn.RawDecodedDataSz,
                  _stats.VideoIn.RawDecodedConvDataSz,
                  _stats.VideoIn.MaxTimePerFrame,
                  _stats.VideoIn.AvgTimePerFrame,
                  _stats.VideoIn.TotalTime,
                  _stats.MaxBufferedVideo,
                  _queueMax,
                  _stats.BufferedVideoAccum / _stats.VideoIn.Frames,
                  _stats.VideoOut.Frames,
                  _stats.VideoOut.TotalDataSz,
                  _stats.VideoOut.TotalDurMs,
                  _stats.VideoOut.Dropped,
                  _stats.VideoTimingDiffs.first,
                  _stats.VideoTimingDiffs.second,
                  _stats.VideoTimingDiffAccum / _stats.VideoOut.Frames,
                  _stats.AudioIn.Frames,
                  _stats.AudioIn.TotalDataSz,
                  _stats.AudioIn.TotalDurMs,
                  _stats.AudioIn.Dropped,
                  static_cast<uint32_t>(_stats.AudioIn.TotalDataSz / _stats.AudioIn.Frames),
                  _stats.AudioIn.TotalDurMs / _stats.AudioIn.Frames,
                  _stats.AudioIn.MaxTimePerFrame,
                  _stats.AudioIn.AvgTimePerFrame,
                  _stats.AudioIn.TotalTime,
                  _stats.MaxBufferedAudioMs,
                  _queueTimeMax,
                  _stats.BufferedAudioAcum / _stats.AudioIn.Frames,
                  _stats.AudioOut.Frames,
                  _stats.AudioOut.TotalDataSz,
                  _stats.AudioOut.TotalDurMs,
                  _stats.AudioOut.Dropped,
                  _stats.AudioTimingDiffs.first,
                  _stats.AudioTimingDiffs.second,
                  _stats.AudioTimingDiffAccum / _stats.AudioOut.Frames,
                  _stats.SyncTimingDiffs.first, _stats.SyncTimingDiffs.second,
                  _stats.SyncTimingDiffAccum / _stats.VideoIn.Frames,
                  _stats.SyncMaxFw, _stats.SyncMaxBw
                  );
}

} // namespace Engine
} // namespace AGS

#endif // AGS_NO_VIDEO_PLAYER
