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
#include "media/video/videoplayer.h"

#ifndef AGS_NO_VIDEO_PLAYER

//-----------------------------------------------------------------------------
// VideoPlayer
//-----------------------------------------------------------------------------
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
    const Size &target_sz, int target_depth)
{
    // We request a target depth from decoder, but it may ignore our request,
    // so we have to check actual "native" frame's depth after
    HError err = OpenImpl(std::move(data_stream), name, flags, target_depth);
    if (!err)
        return err;

    _name = name;
    _flags = flags;
    // Start the audio stream
    if ((flags & kVideo_EnableAudio) != 0)
    {
        if ((_audioFormat > 0) && (_audioChannels > 0) && (_audioFreq > 0))
        {
            _audioOut.reset(new OpenAlSource(_audioFormat, _audioChannels, _audioFreq));
            _audioOut->Play();
            _wantAudio = true;
        }
    }
    // Setup video
    if ((flags & kVideo_EnableVideo) != 0)
    {
        _targetDepth = target_depth > 0 ? target_depth : _frameDepth;
        SetTargetFrame(target_sz);
    }

    _frameTime = 1000 / _frameRate;
    return HError::None();
}

void VideoPlayer::SetTargetFrame(const Size &target_sz)
{
    _targetSize = target_sz.IsNull() ? _frameSize : target_sz;

    // Create helper bitmaps in case of stretching or color depth conversion
    if ((_targetSize != _frameSize) || (_targetDepth != _frameDepth))
    {
        _targetBitmap.reset(BitmapHelper::CreateBitmap(_targetSize.Width, _targetSize.Height, _targetDepth));
        _readyBitmap = _targetBitmap.get();
    }
    else
    {
        _targetBitmap.reset();
        _readyBitmap = _videoFrame.get();
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
}

void VideoPlayer::Stop()
{
    if (IsPlaybackReady(_playState)) // keep any error state
        _playState = PlayStateStopped;

    // Shutdown openal source
    _audioOut.reset();
    // Close video decoder and free resources
    CloseImpl();

    _videoFrame.reset();
    _hicolBuf.reset();
    _targetBitmap.reset();
    _readyBitmap = nullptr;
}

void VideoPlayer::Play()
{
    if (!IsValid())
        return;

    switch (_playState)
    {
    case PlayStatePaused: Resume(); /* fall-through */
    case PlayStateInitial: _playState = PlayStatePlaying; break;
    default: break; // TODO: support rewind/replay from stop/finished state?
    }
}

void VideoPlayer::Pause()
{
    if (_playState != PlayStatePlaying) return;

    if (_audioOut)
        _audioOut->Pause();
    _playState = PlayStatePaused;
}

void VideoPlayer::Resume()
{
    if (_playState != PlayStatePaused) return;

    if (_audioOut)
        _audioOut->Resume();
    _playState = PlayStatePlaying;
}

void VideoPlayer::Seek(float pos_ms)
{
    // TODO
}

bool VideoPlayer::Poll()
{
    if (_playState != PlayStatePlaying)
        return false;
    // Acquire next video frame
    if (!NextFrame() && !_audioFrame)
    { // stop is no new frames, and no buffered frames left
        _playState = PlayStateFinished;
        return false;
    }
    // Render current frame
    if (_audioFrame && !RenderAudio())
    {
        _playState = PlayStateError;
        return false;
    }
    if (_videoFrame && !RenderVideo())
    {
        _playState = PlayStateError;
        return false;
    }
    return true;
}

bool VideoPlayer::RenderAudio()
{
    assert(_audioFrame);
    assert(_audioOut != nullptr);
    _wantAudio = _audioOut->PutData(_audioFrame) > 0u;
    _audioOut->Poll();
    if (_wantAudio)
        _audioFrame = SoundBuffer(); // clear received buffer
    return true;
}

bool VideoPlayer::RenderVideo()
{
    assert(_videoFrame);
    Bitmap *usebuf = _videoFrame.get();

    // Use intermediate hi-color buffer if necessary
    if (_hicolBuf)
    {
        _hicolBuf->Blit(usebuf);
        usebuf = _hicolBuf.get();
    }

    if (_targetBitmap)
    {
        if (_targetSize == _frameSize)
            _targetBitmap->Blit(usebuf);
        else
            _targetBitmap->StretchBlt(usebuf, RectWH(_targetSize));
    }

    return true;
}

} // namespace Engine
} // namespace AGS

#else // AGS_NO_VIDEO_PLAYER



#endif // !AGS_NO_VIDEO_PLAYER
