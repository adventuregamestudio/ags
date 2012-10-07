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
#include "media/audio/soundclip.h"
#include "util/thread.h"

void        calculate_reserved_channel_count();
void        register_audio_script_objects();
void        update_clip_default_volume(ScriptAudioClip *audioClip);
void        start_fading_in_new_track_if_applicable(int fadeInChannel, ScriptAudioClip *newSound);
void        move_track_to_crossfade_channel(int currentChannel, int crossfadeSpeed, int fadeInChannel, ScriptAudioClip *newSound);
void        stop_or_fade_out_channel(int fadeOutChannel, int fadeInChannel = -1, ScriptAudioClip *newSound = NULL);
const char *get_audio_clip_file_name(ScriptAudioClip *clip);
int         find_free_audio_channel(ScriptAudioClip *clip, int priority, bool interruptEqualPriority);
SOUNDCLIP*  load_sound_clip(ScriptAudioClip *audioClip, bool repeat);
void        recache_queued_clips_after_loading_save_game();
void        audio_update_polled_stuff();
void        queue_audio_clip_to_play(ScriptAudioClip *clip, int priority, int repeat);
ScriptAudioChannel* play_audio_clip_on_channel(int channel, ScriptAudioClip *clip, int priority, int repeat, int fromOffset, SOUNDCLIP *cachedClip = NULL);
void        remove_clips_of_type_from_queue(int audioType);
ScriptAudioChannel* play_audio_clip(ScriptAudioClip *clip, int priority, int repeat, int fromOffset, bool queueIfNoChannel);
void        play_audio_clip_by_index(int audioClipIndex);
void        register_audio_script_functions();
bool        unserialize_audio_script_object(int index, const char *objectType, const char *serializedData, int dataSize);
ScriptAudioClip* get_audio_clip_for_old_style_number(bool isMusic, int indexNumber);
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
SOUNDCLIP *load_sound_from_path(int soundNumber, int volume, bool repeat);
void        stop_all_sound_and_music();
void        shutdown_sound();
int         play_sound_priority (int val1, int priority);
int         play_sound(int val1);

//=============================================================================

extern int current_music_type;

void        clear_music_cache();
void        play_next_queued();
int         calculate_max_volume();
void        update_polled_stuff_if_runtime();
// add/remove the volume drop to the audio channels while speech is playing
void        apply_volume_drop_modifier(bool applyModifier);
void        update_polled_stuff(bool checkForDebugMessages);
// Update the music, and advance the crossfade on a step
// (this should only be called once per game loop);
void        update_polled_stuff_and_crossfade ();
void        stopmusic();
void        update_music_volume();
void        post_new_music_check (int newchannel);
int         prepare_for_new_music ();
SOUNDCLIP * load_music_from_disk(int mnum, bool doRepeat);
void        play_new_music(int mnum, SOUNDCLIP *music);
void        newmusic(int mnum);

extern AGS::Engine::Thread audioThread;


extern volatile int psp_audio_multithreaded;    // needed for UPDATE_MP3 macro

#define UPDATE_MP3 \
    if (!psp_audio_multithreaded) \
{ UPDATE_MP3_THREAD }

//#define UPDATE_MP3 update_polled_stuff_if_runtime();

// PSP: Update in thread if wanted.
extern int musicPollIterator; // long name so it doesn't interfere with anything else
extern SOUNDCLIP *channels[MAX_SOUND_CHANNELS+1]; // needed for UPDATE_MP3_THREAD macro
extern volatile int switching_away_from_game;
#define UPDATE_MP3_THREAD \
    while (switching_away_from_game) { } \
    for (musicPollIterator = 0; musicPollIterator <= MAX_SOUND_CHANNELS; musicPollIterator++) { \
    if ((channels[musicPollIterator] != NULL) && (channels[musicPollIterator]->done == 0)) \
    channels[musicPollIterator]->poll(); \
    }

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
