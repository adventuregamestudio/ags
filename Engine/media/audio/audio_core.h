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
// Audio Core: an audio backend interface.
//
//=============================================================================
#ifndef __AGS_EE_MEDIA__AUDIOCORE_H
#define __AGS_EE_MEDIA__AUDIOCORE_H
#include <condition_variable>
#include <memory>
#include <mutex>
#include <vector>
#include "media/audio/audiodefines.h"
#include "media/audio/audioplayer.h"
#include "util/string.h"


namespace AGS
{
namespace Engine
{

// AudioPlayerLock wraps a AudioPlayer pointer guarded by a mutex lock.
// Unlocks the mutex on destruction (e.g. when going out of scope).
class AudioPlayerLock
{
public:
    AudioPlayerLock(AudioPlayer *player, std::unique_lock<std::mutex> &&ulk, std::condition_variable *cv)
        : _player(player)
        , _ulock(std::move(ulk))
        , _cv(cv)
    {
    }

    AudioPlayerLock(AudioPlayerLock &&lock)
        : _player(lock._player)
        , _ulock(std::move(lock._ulock))
        , _cv(lock._cv)
    {
        lock._cv = nullptr;
    }

    ~AudioPlayerLock()
    {
        if (_cv)
            _cv->notify_all();
    }

    const AudioPlayer *operator ->() const { return _player; }
    AudioPlayer *operator ->() { return _player; }

private:
    AudioPlayer *_player = nullptr;
    std::unique_lock<std::mutex> _ulock;
    std::condition_variable *_cv = nullptr;
};

} // namespace Engine
} // namespace AGS

// Initializes audio core system;
// starts polling on a background thread.
void audio_core_init(/*config, soundlib*/);
// Shut downs audio core system;
// stops any associated threads.
void audio_core_shutdown();

// Audio core config
// Set new master volume, affects all slots
void audio_core_set_master_volume(float newvol);

// Audio slot controls: slots are abstract holders for a playback.
//
// Initializes playback on a free playback slot (reuses spare one or allocates new if there's none).
// Data array must contain full wave data to play.
int audio_core_slot_init(std::shared_ptr<std::vector<uint8_t>> &data, const AGS::Common::String &extension_hint, bool repeat);
// Initializes playback streaming
int audio_core_slot_init(std::unique_ptr<AGS::Common::Stream> in, const AGS::Common::String &extension_hint, bool repeat);
// Returns a AudioPlayer from the given slot, wrapped in a auto-locking struct.
AGS::Engine::AudioPlayerLock audio_core_get_player(int slot_handle);
// Stop and release the audio player at the given slot
void audio_core_slot_stop(int slot_handle);

#if defined(AGS_DISABLE_THREADS)
// polls the audio core if we have no threads, polled in Engine/ac/timer.cpp
void audio_core_entry_poll();
#endif

#endif // __AGS_EE_MEDIA__AUDIOCORE_H
