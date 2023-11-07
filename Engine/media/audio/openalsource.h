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
// OpenAlSource wraps and manages a single OpenAL's source object.
// Works as a sound player, transferring input data into OpenAL and querying
// current state.
// Supports on the go audio resampling, which is activated when necessary.
//
//=============================================================================
#ifndef __AGS_EE_MEDIA__OPENALSOURCE_H
#define __AGS_EE_MEDIA__OPENALSOURCE_H
#include <deque>
#include "media/audio/audiodefines.h"
#include "media/audio/openal.h"
#include "media/audio/sdldecoder.h"

namespace AGS
{
namespace Engine
{

class OpenAlSource
{
public:
    // Max sound buffers to queue before/during processing
    static const ALuint MaxQueue = 2;

    // Initializes Al source for the given format; if there's no direct format equivalent
    // found, setups a resampler.
    OpenAlSource(SDL_AudioFormat format, int channels, int freq);
    OpenAlSource(OpenAlSource&& src);
    ~OpenAlSource();

    // Tells if the al source is valid and usable
    bool IsValid() const { return _source > 0; }
    // Gets current playback state
    PlaybackState GetPlayState() const { return _playState; }
    // Tells if the data queue is empty
    bool IsEmpty() const { return _queued == 0; }
    // Gets current playback position, in ms
    float GetPositionMs() const;

    // Try putting data into the queue; returns amount of data copied,
    // or 0 if data cannot be accepted at the moment.
    size_t PutData(const SoundBuffer data);
    // Updates the state, processes the sound queue
    ALuint Poll();

    // Starts the playback, the sound will be played as soon as there's any data is in queue
    void Play();
    // Stops the playback, clears the queued data
    void Stop();
    // Pauses the playback, keeps the queued data
    void Pause();
    // Resumes the playback from the current position
    void Resume();

    // Sets the reference position in ms; this may be necessary because player
    // receives position hint only with the data timestamps
    void SetPlaybackPosMs(float pos_ms);
    // Sets the sound panning (-1.0f to 1.0)
    void SetPanning(float panning);
    // Sets the playback speed (fraction of normal); NOTE: the speed is implemented through resampling
    void SetSpeed(float speed);
    // Sets the playback volume (gain)
    void SetVolume(float volume);

private:
    // Unqueues processed buffers
    void Unqueue();

    ALuint _source = 0u;
    Sound_AudioInfo _inputFmt; // actual input format
    Sound_AudioInfo _recvFmt; // corrected format (if necessary)
    ALenum _alFormat = 0u; // matching OpenAl format
    PlaybackState _playState = PlayStateInitial;
    float _speed = 1.f; // change in playback rate
    float _predictTs = 0.f; // next timestamp prediction
    unsigned _queued = 0u;

    // SDL resampler state, in case dynamic resampling in necessary
    SDLResampler _resampler;
    // Keeping record of some precalculated buffer properties
    struct BufferRecord
    {
        float Timestamp = 0.f;
        float Duration = 0.f;
        float Speed = 0.f; // associated playback speed
        BufferRecord() = default;
        BufferRecord(float ts, float dur, float sp) : Timestamp(ts), Duration(dur), Speed(sp) {}
    };
    // Playback parameters related to the queued buffers
    std::deque<BufferRecord> _bufferRecords;
};

} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_MEDIA__OPENALSOURCE_H
