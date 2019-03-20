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

#ifndef __AC_AUDIO_H
#define __AC_AUDIO_H

#include "media/audio/audiodefines.h"
#include "ac/dynobj/scriptaudioclip.h"
#include "ac/dynobj/scriptaudiochannel.h"
#include "media/audio/ambientsound.h"
#include "util/mutex.h"
#include "util/mutex_lock.h"
#include "util/thread.h"

struct SOUNDCLIP;

void        calculate_reserved_channel_count();
void        update_clip_default_volume(ScriptAudioClip *audioClip);
void        start_fading_in_new_track_if_applicable(int fadeInChannel, ScriptAudioClip *newSound);
void        move_track_to_crossfade_channel(int currentChannel, int crossfadeSpeed, int fadeInChannel, ScriptAudioClip *newSound);
void        stop_or_fade_out_channel(int fadeOutChannel, int fadeInChannel = -1, ScriptAudioClip *newSound = NULL);
int         find_free_audio_channel(ScriptAudioClip *clip, int priority, bool interruptEqualPriority);
SOUNDCLIP*  load_sound_clip(ScriptAudioClip *audioClip, bool repeat);
void        audio_update_polled_stuff();
void        queue_audio_clip_to_play(ScriptAudioClip *clip, int priority, int repeat);
ScriptAudioChannel* play_audio_clip_on_channel(int channel, ScriptAudioClip *clip, int priority, int repeat, int fromOffset, SOUNDCLIP *cachedClip = NULL);
void        remove_clips_of_type_from_queue(int audioType);
void        update_queued_clips_volume(int audioType, int new_vol);
// Checks if speech voice-over is currently playing, and reapply volume drop to all other active clips
void        update_volume_drop_if_voiceover();
ScriptAudioChannel* play_audio_clip(ScriptAudioClip *clip, int priority, int repeat, int fromOffset, bool queueIfNoChannel);
ScriptAudioChannel* play_audio_clip_by_index(int audioClipIndex);
void        stop_and_destroy_channel_ex(int chid, bool resetLegacyMusicSettings);
void        stop_and_destroy_channel (int chid);

// ***** BACKWARDS COMPATIBILITY WITH OLD AUDIO SYSTEM ***** //
int         get_old_style_number_for_sound(int sound_number);
SOUNDCLIP * load_sound_clip_from_old_style_number(bool isMusic, int indexNumber, bool repeat);

//=============================================================================

int         init_mod_player(int numVoices);
void        remove_mod_player();
void        force_audiostream_include();
int         get_volume_adjusted_for_distance(int volume, int sndX, int sndY, int sndMaxDist);
void        update_directional_sound_vol();
void        update_ambient_sound_vol ();
// Tells if the audio type is allowed to play with regards to current sound config
bool        is_audiotype_allowed_to_play(AudioFileType type);
// Loads sound data referenced by audio clip item, and starts playback;
// returns NULL on failure
SOUNDCLIP * load_sound_and_play(ScriptAudioClip *aclip, bool repeat);
void        stop_all_sound_and_music();
void        shutdown_sound();
int         play_sound_priority (int val1, int priority);
int         play_sound(int val1);

//=============================================================================

extern int current_music_type;

void        clear_music_cache();
void        play_next_queued();
int         calculate_max_volume();
// add/remove the volume drop to the audio channels while speech is playing
void        apply_volume_drop_modifier(bool applyModifier);
// Update the music, and advance the crossfade on a step
// (this should only be called once per game loop);
void        update_audio_system_on_game_loop ();
void        stopmusic();
void        update_music_volume();
void        post_new_music_check (int newchannel);
int         prepare_for_new_music ();
// Gets audio clip from legacy music number, which also may contain queue flag
ScriptAudioClip *get_audio_clip_for_music(int mnum);
SOUNDCLIP * load_music_from_disk(int mnum, bool doRepeat);
void        play_new_music(int mnum, SOUNDCLIP *music);
void        newmusic(int mnum);

extern AGS::Engine::Thread audioThread;
extern AGS::Engine::Mutex _audio_mutex;
extern volatile bool _audio_doing_crossfade;
extern SOUNDCLIP *channels[MAX_SOUND_CHANNELS+1]; // needed for update_mp3_thread
extern volatile int psp_audio_multithreaded;

void update_polled_mp3();
void update_mp3_thread();

extern volatile int mvolcounter;
extern int update_music_at;

// crossFading is >0 (channel number of new track), or -1 (old
// track fading out, no new track)
extern int crossFading, crossFadeVolumePerStep, crossFadeStep;
extern int crossFadeVolumeAtStart;

extern SOUNDCLIP *cachedQueuedMusic;

extern int last_sound_played[MAX_SOUND_CHANNELS + 1];
extern AmbientSound ambient[MAX_SOUND_CHANNELS + 1];  // + 1 just for safety on array iterations

#endif // __AC_AUDIO_H
