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
//
// Audio Core: an audio backend interface.
//
//=============================================================================
#ifndef __AGS_EE_MEDIA__AUDIOCORE_H
#define __AGS_EE_MEDIA__AUDIOCORE_H
#include <memory>
#include <vector>
#include "media/audio/audiodefines.h"
#include "media/audio/audioplayer.h"
#include "util/string.h"
#include "util/threading.h"


// AudioPlayerLock wraps a AudioPlayer pointer guarded by a mutex lock.
// Unlocks the mutex on destruction (e.g. when going out of scope).
typedef AGS::Engine::LockedObjectPtr<AGS::Engine::AudioPlayer>
    AudioPlayerLock;

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
// Initializes playback on a free playback slot.
// Data array must contain full wave data to play.
int audio_core_slot_init(std::shared_ptr<std::vector<uint8_t>> &data, const AGS::Common::String &ext_hint, bool repeat);
// Initializes playback streaming
int audio_core_slot_init(std::unique_ptr<AGS::Common::Stream> in, const AGS::Common::String &ext_hint, bool repeat);
// Returns a AudioPlayer from the given slot, wrapped in a auto-locking struct.
AudioPlayerLock audio_core_get_player(int slot_handle);
// Stop and release the audio player at the given slot
void audio_core_slot_stop(int slot_handle);

#if defined(AGS_DISABLE_THREADS)
// Polls the audio core if we have no threads, polled in WaitForNextFrame()
void audio_core_entry_poll();
#endif

#endif // __AGS_EE_MEDIA__AUDIOCORE_H
