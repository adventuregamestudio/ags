//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
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
    // FIXME: only use source id privately
    ALuint GetSourceID() const { return _source; }
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
    void Play() { Play(_processedMs); }
    // Starts the playback, telling the position value associated with the sound data
    void Play(float timestamp);
    // Stops the playback, clears the queued data
    void Stop();
    // Pauses the playback, keeps the queued data
    void Pause();
    // Resumes the playback from the current position
    void Resume();
    // Sets the arbitrary playback position associated with current sound data
    void SetPlayTime(float timestamp);
    // Sets the playback speed; the speed is implemented through a resampling
    void SetSpeed(float speed);

private:
    // Unqueues processed buffers
    void Unqueue();

    ALuint _source = 0u;
    SDL_AudioFormat _inputFormat = 0u;
    ALenum _alFormat = 0u;
    int _freq = 0;
    int _channels = 0;
    PlaybackState _playState = PlayStateInitial;
    float _speed = 1.f; // change in playback rate
    float _processedMs = 0.0f;
    mutable float _lastPosReport = 0.f; // to fixup reported position, in case speed changes
    unsigned _queued = 0u;

    // SDL resampler state, in case dynamic resampling in necessary
    SDLResampler _resampler;
    // Keeping record of some precalculated buffer properties
    struct BufferParams
    {
        float AlTime = 0.f; // buffer time in internal openal's rate (in seconds)
        float Speed = 0.f; // associated playback speed
        float Time = 0.f; // buffer time in the real playback rate (speed-adjusted)
        BufferParams() = default;
        BufferParams(float t, float sp) : AlTime(t), Speed(sp), Time(t * sp) {}
    };
    // playback speeds related to queued buffers
    std::deque<BufferParams> _bufferRecords;
};

} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_MEDIA__OPENALSOURCE_H
