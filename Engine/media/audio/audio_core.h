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
#include "util/string.h"

// Initializes audio core system;
// starts polling on a background thread.
void audio_core_init(/*config, soundlib*/);
// Shut downs audio core system;
// stops any associated threads.
void audio_core_shutdown();

// Audio slot controls: slots are abstract holders for a playback.
//
// Initializes playback on a free playback slot (reuses spare one or allocates new if there's none).
// Data array must contain full wave data to play.
int audio_core_slot_init(std::shared_ptr<std::vector<uint8_t>> &data, const AGS::Common::String &extension_hint, bool repeat);
// Initializes playback streaming
int audio_core_slot_init(std::unique_ptr<AGS::Common::Stream> in, const AGS::Common::String &extension_hint, bool repeat);
// Start playback on a slot
PlaybackState audio_core_slot_play(int slot_handle);
// Pause playback on a slot, resume with 'audio_core_slot_play'
PlaybackState audio_core_slot_pause(int slot_handle);
// Stop playback on a slot, disposes sound data, frees a slot
void audio_core_slot_stop(int slot_handle);
// Seek on a slot, new position in milliseconds
void audio_core_slot_seek_ms(int slot_handle, float pos_ms);

#if defined(AGS_DISABLE_THREADS)
// polls the audio core if we have no threads, polled in Engine/ac/timer.cpp
void audio_core_entry_poll();
#endif

// Audio core config
// Set new master volume, affects all slots
void audio_core_set_master_volume(float newvol);
// Sets up single playback parameters
void audio_core_slot_configure(int slot_handle, float volume, float speed, float panning);

PlaybackState audio_core_slot_get_play_state(int slot_handle);
PlaybackState audio_core_slot_get_play_state(int slot_handle, float &pos_ms);
float audio_core_slot_get_pos_ms(int slot_handle);
// Returns sound duration in milliseconds
float audio_core_slot_get_duration(int slot_handle);
int audio_core_slot_get_freq(int slot_handle);

#endif // __AGS_EE_MEDIA__AUDIOCORE_H
