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
        _playState = PlayStateStopped;

    // Shutdown openal source
    _audioOut.reset();
    // Close video decoder and free resources
    CloseImpl();

    _vframeBuf = nullptr;
    _hicolBuf = nullptr;
    _videoFramePool = std::stack<std::unique_ptr<Bitmap>>();
    _videoFrameQueue = std::deque<std::unique_ptr<Bitmap>>();
}

void VideoPlayer::Play()
{
    if (!IsValid())
        return;

    switch (_playState)
    {
    case PlayStatePaused:
        ResumeImpl();
        /* fallthrough */
    case PlayStateInitial:
        if (_audioOut)
            _audioOut->Play();
        _playState = PlayStatePlaying;
        break;
    default:
        break; // TODO: support rewind/replay from stop/finished state?
    }
}

void VideoPlayer::Pause()
{
    if (_playState != PlayStatePlaying)
        return;

    if (_audioOut)
        _audioOut->Pause();
    _playState = PlayStatePaused;
    _pauseTs = AGS_Clock::now();
}

float VideoPlayer::Seek(float pos_ms)
{
    if ((pos_ms == 0.f) && Rewind())
    {
        return 0.f;
    }
    return -1.f; // TODO
}

uint32_t VideoPlayer::SeekFrame(uint32_t frame)
{
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
    AGS_Clock::time_point now{};
    AGS_Clock::duration play_dur{};
    switch (_playState)
    {
    case PlaybackState::PlayStatePlaying:
        now = AGS_Clock::now();
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
    AGS_Clock::duration virtual_play_dur =
        AGS_Clock::duration((int64_t)(play_dur.count() * ft_rel));
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

    // Buffer always when ready, even if we are paused
    if (HasVideo())
        BufferVideo();
    if (HasAudio())
        BufferAudio();

    if (_playState != PlayStatePlaying)
        return false;

    UpdateTime();

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
    _startTs = AGS_Clock::now();
    _pauseTs = _startTs;
    _playbackDuration = AGS_Clock::duration();
    _wantFrameIndex = 0u;
    _posMs = 0.f;
    _framesPlayed = 0;
    return true;
}

void VideoPlayer::ResumeImpl()
{
    // Update our virtual "start time" to keep the proper frame timing
    auto pause_dur = AGS_Clock::now() - _pauseTs;
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

    // Try to retrieve one video frame from decoder
    const bool must_conv = (_targetSize != _frameSize || _targetDepth != _frameDepth
        || ((_flags & kVideo_AccumFrame) != 0));
    Bitmap *usebuf = must_conv ? _vframeBuf.get() : target_frame.get();
    if (!NextVideoFrame(usebuf))
    { // failed to get frame, so move prepared target frame into the pool for now
        _videoFramePool.push(std::move(target_frame));
        return;
    }

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

    // Push final frame to the queue
    _videoFrameQueue.push_back(std::move(target_frame));
}

void VideoPlayer::BufferAudio()
{
    if (_audioFrame)
        return; // still got one queued

    _audioFrame = NextAudioFrame();
}

void VideoPlayer::UpdateTime()
{
    auto now = AGS_Clock::now();
    if (_resetStartTime)
    {
        _startTs = now;
        _resetStartTime = false;
    }
    _pollTs = AGS_Clock::now();
    _playbackDuration = _pollTs - _startTs;
    _wantFrameIndex = std::chrono::duration_cast<std::chrono::milliseconds>(_playbackDuration).count()
        / _targetFrameTime;
    /*Debug::Printf("VIDEO TIME: playdur %lld, target frame time %.2f, want frame = %u, played frame = %u",
        std::chrono::duration_cast<std::chrono::milliseconds>(_playbackDuration).count(),
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
        _audioFrame = SoundBuffer(); // clear received buffer
    }
    _audioOut->Poll();
    return true;
}

} // namespace Engine
} // namespace AGS

#endif // AGS_NO_VIDEO_PLAYER
