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
    _videoFrameQueue = std::deque<std::unique_ptr<Bitmap>>();

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
    float video_pos = HasVideo() ? _framesPlayed * _frameTime : 0.f;
    float audio_pos = HasAudio() ? _audioOut->GetPositionMs() : 0.f;
    _posMs = std::max(video_pos, audio_pos);
    return frame;
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

    auto old_frametime = _targetFrameTime;
    _targetFPS = _frameRate * speed;
    _targetFrameTime = 1000.f / _targetFPS;

    // Adjust our virtual timestamps by the difference between
    // previous play duration, and new duration calculated from the new speed.
    float ft_rel = _targetFrameTime / old_frametime;
    Clock::duration virtual_play_dur =
        Clock::duration((int64_t)(play_dur.count() * ft_rel));
    _startTs = now - virtual_play_dur;

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
    if (_framesPlayed > _wantFrameIndex)
        return nullptr;
    return NextFrameFromQueue();
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

    bool res_video = HasVideo() && ProcessVideo();
    bool res_audio = HasAudio() && ProcessAudio();

    // TODO: get frame timestamps if available from decoder?
    float video_pos = HasVideo() ? _framesPlayed * _frameTime : 0.f;
    float audio_pos = HasAudio() ? _audioOut->GetPositionMs() : 0.f;
    _posMs = std::max(video_pos, audio_pos);

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
    _playbackDuration = Clock::duration();
    _wantFrameIndex = 0u;
    _posMs = 0.f;
    _framesPlayed = 0;
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
    if (_videoFrameQueue.size() >= _videoQueueMax)
        return;

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
    if (!NextVideoFrame(usebuf))
    { // failed to get frame, so move prepared target frame into the pool for now
        _videoFramePool.push(std::move(target_frame));
        return;
    }

    const auto decoded_frame_sz = usebuf->GetDataSize();

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
    _stats.VideoIn.Frames++;
    _stats.VideoIn.TotalDataSz += target_frame->GetDataSize();
    _stats.VideoIn.TotalDurMs += _frameTime;
    _stats.VideoIn.TotalTime += input_dur_ms;
    _stats.VideoIn.RawDecodedDataSz = decoded_frame_sz;
    _stats.VideoIn.RawDecodedConvDataSz = target_frame->GetDataSize();
    _stats.VideoIn.AvgTimePerFrame = _stats.VideoIn.TotalTime / _stats.VideoIn.Frames;
    _stats.VideoIn.MaxTimePerFrame = std::max(_stats.VideoIn.MaxTimePerFrame, static_cast<uint32_t>(input_dur_ms));

    // Push final frame to the queue
    _videoFrameQueue.push_back(std::move(target_frame));
}

void VideoPlayer::BufferAudio()
{
    if (_audioFrame)
        return; // still got one queued

    const auto input_start = Clock::now();

    _audioFrame = NextAudioFrame();

    const auto input_dur_ms = ToMilliseconds(Clock::now() - input_start);

    // Stats
    if (_audioFrame)
    {
        _stats.AudioIn.Frames++;
        _stats.AudioIn.TotalDataSz += _audioFrame.Size;
        _stats.AudioIn.TotalDurMs += _audioFrame.DurMs;
        _stats.AudioIn.TotalTime += input_dur_ms;
        _stats.AudioIn.AvgTimePerFrame = _stats.AudioIn.TotalTime / _stats.AudioIn.Frames;
        _stats.AudioIn.MaxTimePerFrame = std::max(_stats.AudioIn.MaxTimePerFrame, static_cast<uint32_t>(input_dur_ms));
    }
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
    _stats.MaxBufferedVideo = std::max(_stats.MaxBufferedVideo, _videoFrameQueue.size());

    if (PrintStatsEachMs > 0u)
        PrintStats(false);
}

void VideoPlayer::UpdatePlayTime()
{
    auto now = Clock::now();
    if (_resetStartTime)
    {
        _startTs = now;
        _resetStartTime = false;
    }
    _pollTs = now;
    _playbackDuration = _pollTs - _startTs;
    _wantFrameIndex = ToMilliseconds(_playbackDuration) / _targetFrameTime;
    /*Debug::Printf("VIDEO TIME: playdur %lld, target frame time %.2f, want frame = %u, played frame = %u",
        ToMilliseconds(_playbackDuration),
        _targetFrameTime,
        _wantFrameIndex,
        _framesPlayed);/**/
}

std::unique_ptr<Bitmap> VideoPlayer::NextFrameFromQueue()
{
    if (_videoFrameQueue.empty())
        return nullptr;
    auto frame = std::move(_videoFrameQueue.front());
    _videoFrameQueue.pop_front();
    _framesPlayed++;

    // Stats
    _stats.VideoOut.Frames++;
    _stats.VideoOut.TotalDataSz += frame->GetDataSize();
    _stats.VideoOut.TotalDurMs += _frameTime;
    return frame;
}

bool VideoPlayer::ProcessVideo()
{
    // Optionally drop late frames, but leave at least 1 for display
    if ((_flags & kVideo_DropFrames) != 0)
    {
        while ((_videoFrameQueue.size() > 1) &&
            (_framesPlayed /*+ 1*/ < _wantFrameIndex))
        {
            auto frame = NextFrameFromQueue();
            assert(frame);
            _videoFramePool.push(std::move(frame));
            //Debug::Printf("DROPPED LATE FRAME, queue size: %d", _videoFrameQueue.size());
            _stats.VideoOut.Dropped++;
        }
    }
    // We are good so long as there's a ready frame in queue
    return !_videoFrameQueue.empty();
}

bool VideoPlayer::ProcessAudio()
{
    if (!_audioFrame)
        return false;

    assert(_audioOut);
    if (_audioOut->PutData(_audioFrame) > 0u)
    {
        // Stats
        _stats.AudioOut.Frames++;
        _stats.AudioOut.TotalDataSz += _audioFrame.Size;
        _stats.AudioOut.TotalDurMs += _audioFrame.DurMs;

        _audioFrame = SoundBuffer(); // clear received buffer
    }
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
                  "\n\tvideo output frames: %u"
                  "\n\t            total size: %llu bytes"
                  "\n\t            total duration: %.2f ms"
                  "\n\t            frames dropped: %u"
                  "\n\taudio input frames: %u"
                  "\n\t            total size: %llu bytes"
                  "\n\t            total duration: %.2f ms"
                  "\n\t            frames dropped: %u"
                  "\n\taverage audio frame size: %u bytes"
                  "\n\tmax time per input audio frame: %u ms"
                  "\n\tavg time per input audio frame: %u ms"
                  "\n\ttotal time on input audio frames: %llu ms"
                  "\n\taudio output frames: %u"
                  "\n\t            total size: %llu bytes"
                  "\n\t            total duration: %.2f ms"
                  "\n\t            frames dropped: %u",
                  _name.GetCStr(),
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
                  _videoQueueMax,
                  _stats.VideoOut.Frames,
                  _stats.VideoOut.TotalDataSz,
                  _stats.VideoOut.TotalDurMs,
                  _stats.VideoOut.Dropped,
                  _stats.AudioIn.Frames,
                  _stats.AudioIn.TotalDataSz,
                  _stats.AudioIn.TotalDurMs,
                  _stats.AudioIn.Dropped,
                  static_cast<uint32_t>(_stats.AudioIn.TotalDataSz / _stats.AudioIn.Frames),
                  _stats.AudioIn.MaxTimePerFrame,
                  _stats.AudioIn.AvgTimePerFrame,
                  _stats.AudioIn.TotalTime,
                  _stats.AudioOut.Frames,
                  _stats.AudioOut.TotalDataSz,
                  _stats.AudioOut.TotalDurMs,
                  _stats.AudioOut.Dropped
                  );
}

} // namespace Engine
} // namespace AGS

#endif // AGS_NO_VIDEO_PLAYER
