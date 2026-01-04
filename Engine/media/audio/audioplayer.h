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
// Audio playback class.
// Controls playback state. Retrieves audio data from decoder and passes into
// the audio output.
//
// TODO: a virtual Decoder and AudioOutput interfaces, to let hide current
// implementations, and also substitute default implementations
// (e.g. with plugins).
//
//=============================================================================
#ifndef __AGS_EE_MEDIA__AUDIOPLAYER_H
#define __AGS_EE_MEDIA__AUDIOPLAYER_H
#include <memory>
#include "media/audio/audiodefines.h" // PlaybackState etc
#include "media/audio/sdldecoder.h"
#include "media/audio/openalsource.h"

namespace AGS
{
namespace Engine
{

class AudioPlayer
{
public:
    AudioPlayer(int handle, std::unique_ptr<SDLDecoder> decoder);

    // Gets current playback state
    PlaybackState GetPlayState() const { return _playState; }
    // Gets current playback state, *excluding* temporary states such as Initial
    // FIXME: rework this! -- a quick hack to let external user know the future player state on init stage
    PlaybackState GetPlayStateNormal() const { return _playState == PlayStateInitial ? _onLoadPlayState : _playState; }
    // Gets frequency (sample rate)
    float GetFrequency() const { return _decoder->GetFreq(); }
    // Gets duration, in ms
    float GetDurationMs() const { return _decoder->GetDurationMs(); }
    // Gets playback position, in ms
    float GetPositionMs() const { return _source->GetPositionMs(); }

    // Sets the sound panning (-1.0f to 1.0)
    void SetPanning(float panning) { _source->SetPanning(panning); }
    // Sets the playback speed (fraction of normal);
    // NOTE: the speed is implemented through resampling
    void SetSpeed(float speed) { _source->SetSpeed(speed); }
    // Sets the playback volume (gain)
    void SetVolume(float volume) { _source->SetVolume(volume); }

    // Update state, transfer data from decoder to player if possible
    void Poll();
    // Begin playback
    void Play();
    // Pause playback
    void Pause();
    // Stop playback completely
    void Stop();
    // Seek to the given time position
    void Seek(float pos_ms);

private:
    // Opens decoder and sets up playback state
    void Init();

    const int handle_ = -1; // for diagnostic purposes only
    std::unique_ptr<SDLDecoder> _decoder;
    std::unique_ptr<OpenAlSource> _source;
    PlaybackState _playState = PlayStateInitial;
    PlaybackState _onLoadPlayState = PlayStatePaused;
    float _onLoadPositionMs = 0.0f;
    SoundBuffer _bufferPending{};
};

} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_MEDIA__AUDIOPLAYER_H
