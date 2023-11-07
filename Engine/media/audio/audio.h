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
#ifndef __AGS_EE_MEDIA__AUDIO_H__
#define __AGS_EE_MEDIA__AUDIO_H__

#include <array>
#include <memory>
#include "media/audio/audiodefines.h"
#include "ac/dynobj/scriptaudioclip.h"
#include "ac/dynobj/scriptaudiochannel.h"
#include "ac/timer.h"

class SOUNDCLIP;

class AudioChans
{
public:
    // Gets a clip from the channel
    static SOUNDCLIP *GetChannel(int index);
    // Gets a clip from the channel but only if it's in playback state
    static SOUNDCLIP *GetChannelIfPlaying(int index);
    // Assign new clip to the channel
    static SOUNDCLIP *SetChannel(int index, std::unique_ptr<SOUNDCLIP> clip);
    // Move clip from one channel to another, clearing the first channel
    static SOUNDCLIP *MoveChannel(int to, int from);
    // Deletes any clip and frees the channel
    static void       DeleteClipOnChannel(int index);

    // Tells if channel has got a clip; does not care about its state
    static inline bool ChannelHasClip(int index) { return GetChannel(index) != nullptr; }
    // Tells if channel has got a clip and clip is in playback state
    static inline bool ChannelIsPlaying(int index) { return GetChannelIfPlaying(index) != nullptr; }

private:
    AudioChans() = delete;
    ~AudioChans() = delete;
};


void        calculate_reserved_channel_count();
void        update_clip_default_volume(ScriptAudioClip *audioClip);
void        start_fading_in_new_track_if_applicable(int fadeInChannel, ScriptAudioClip *newSound);
void        stop_or_fade_out_channel(int fadeOutChannel, int fadeInChannel = -1, ScriptAudioClip *newSound = nullptr);
SOUNDCLIP*  load_sound_clip(ScriptAudioClip *audioClip, bool repeat);
ScriptAudioChannel* play_audio_clip_on_channel(int channel, ScriptAudioClip *clip, int priority, int repeat, int fromOffset, SOUNDCLIP *cachedClip = nullptr);
void        remove_clips_of_type_from_queue(int audioType);
void        update_queued_clips_volume(int audioType, int new_vol);
// Checks if speech voice-over is currently playing, and reapply volume drop to all other active clips
void        update_volume_drop_if_voiceover();
ScriptAudioChannel* play_audio_clip(ScriptAudioClip *clip, int priority, int repeat, int fromOffset, bool queueIfNoChannel);
ScriptAudioChannel* play_audio_clip_by_index(int audioClipIndex);
void        stop_and_destroy_channel (int chid);
// Exports missing AudioChannel objects to script (for importing older saves)
void        export_missing_audiochans();

//=============================================================================

int         get_volume_adjusted_for_distance(int volume, int sndX, int sndY, int sndMaxDist);
void        update_directional_sound_vol();
// Tells if the audio type is allowed to play with regards to current sound config
bool        is_audiotype_allowed_to_play(AudioFileType type);
// Loads sound data referenced by audio clip item, and starts playback;
// returns NULL on failure
SOUNDCLIP * load_sound_and_play(ScriptAudioClip *aclip, bool repeat);
void        stop_all_sound_and_music();
void        shutdown_sound();

//=============================================================================

// add/remove the volume drop to the audio channels while speech is playing
void        apply_volume_drop_modifier(bool applyModifier);
// syncs logical audio channels with the audio backend state
void        sync_audio_playback();
void        update_audio_system_on_game_loop ();

extern void cancel_scheduled_music_update();
extern void schedule_music_update_at(AGS_Clock::time_point);
extern void postpone_scheduled_music_update_by(std::chrono::milliseconds);

#endif // __AGS_EE_MEDIA__AUDIO_H__
