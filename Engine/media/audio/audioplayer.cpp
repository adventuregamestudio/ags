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
#include "media/audio/audioplayer.h"
#include "util/memory_compat.h"

namespace AGS
{
namespace Engine
{

AudioPlayer::AudioPlayer(int handle, std::unique_ptr<SDLDecoder> decoder)
    : handle_(handle), _decoder(std::move(decoder))
{
    _source = std::make_unique<OpenAlSource>(
        _decoder->GetFormat(), _decoder->GetChannels(), _decoder->GetFreq());
}

void AudioPlayer::Init()
{
    bool success;
    if (_decoder->IsValid()) // if already opened, then just seek to start
        success = _decoder->Seek(_onLoadPositionMs) == _onLoadPositionMs;
    else
        success = _decoder->Open(_onLoadPositionMs);
    _source->SetPlaybackPosMs(_decoder->GetPositionMs());
    _playState = success ? _onLoadPlayState : PlayStateError;
    if (_playState == PlayStatePlaying)
        _source->Play();
}

void AudioPlayer::Poll()
{
    if (_playState == PlaybackState::PlayStateInitial)
        Init();
    if (_playState != PlayStatePlaying)
        return;

    // Read data from Decoder and pass into the Al Source
    if (!_bufferPending.Data() && !_decoder->EOS())
    { // if no buffer saved, and still something to decode, then read a buffer
        _bufferPending = _decoder->GetData();
        assert(_bufferPending.Data() || (_bufferPending.Size() == 0));
    }
    if (_bufferPending.Data() && (_bufferPending.Size() > 0))
    { // if having a buffer already, then try to put into source
        if (_source->PutData(_bufferPending) > 0)
            _bufferPending = SoundBufferPtr(); // clear buffer on success
    }
    _source->Poll();
    // If both finished decoding and playing, we done here.
    if (_decoder->EOS() && _source->IsEmpty())
    {
        _playState = PlayStateFinished;
    }
}

void AudioPlayer::Play()
{
    switch (_playState)
    {
    case PlayStateInitial:
        _onLoadPlayState = PlayStatePlaying;
        break;
    case PlayStateStopped:
        _decoder->Seek(0.0f);
        /* fall-through */
    case PlayStatePaused:
        _playState = PlayStatePlaying;
        _source->Play();
        break;
    default:
        break;
    }
}

void AudioPlayer::Pause()
{
    switch (_playState)
    {
    case PlayStateInitial:
        _onLoadPlayState = PlayStatePaused;
        break;
    case PlayStatePlaying:
        _playState = PlayStatePaused;
        _source->Pause();
        break;
    default:
        break;
    }
}

void AudioPlayer::Stop()
{
    switch (_playState)
    {
    case PlayStateInitial:
        _onLoadPlayState = PlayStateStopped;
        break;
    case PlayStatePlaying:
    case PlayStatePaused:
        _playState = PlayStateStopped;
        _source->Stop();
        _bufferPending = SoundBufferPtr(); // clear
        break;
    default:
        break;
    }
}

void AudioPlayer::Seek(float pos_ms)
{
    switch (_playState)
    {
    case PlayStateInitial:
        _onLoadPositionMs = pos_ms;
        Init(); // make sure the decoder opens and seeks, update posMs
        break;
    case PlayStatePlaying:
    case PlayStatePaused:
    case PlayStateStopped:
        {
            _source->Stop();
            _bufferPending = SoundBufferPtr(); // clear
            float new_pos = _decoder->Seek(pos_ms);
            _source->SetPlaybackPosMs(new_pos);
        }
        break;
    default:
        break;
    }
}

} // namespace Engine
} // namespace AGS
