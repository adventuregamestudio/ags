/*
Adventure Game Studio source code Copyright 1999-2011 Chris Jones.
All rights reserved.

The AGS Editor Source Code is provided under the Artistic License 2.0
http://www.opensource.org/licenses/artistic-license-2.0.php

You MAY NOT compile your own builds of the engine without making it EXPLICITLY
CLEAR that the code has been altered from the Standard Version.

*/
#ifndef __AC_AUDIO_H
#define __AC_AUDIO_H

#define VOL_CHANGEEXISTING   1678
#define VOL_SETFUTUREDEFAULT 1679
#define VOL_BOTH             1680

#include "acaudio/ac_audiochannel.h"
#include "acaudio/ac_soundclip.h"

void calculate_reserved_channel_count();
void register_audio_script_objects();
void update_clip_default_volume(ScriptAudioClip *audioClip);
void start_fading_in_new_track_if_applicable(int fadeInChannel, ScriptAudioClip *newSound);
void move_track_to_crossfade_channel(int currentChannel, int crossfadeSpeed, int fadeInChannel, ScriptAudioClip *newSound);
void stop_or_fade_out_channel(int fadeOutChannel, int fadeInChannel = -1, ScriptAudioClip *newSound = NULL);
const char* get_audio_clip_file_name(ScriptAudioClip *clip);
int find_free_audio_channel(ScriptAudioClip *clip, int priority, bool interruptEqualPriority);
SOUNDCLIP *load_sound_clip(ScriptAudioClip *audioClip, bool repeat);
void recache_queued_clips_after_loading_save_game();
void audio_update_polled_stuff();
void queue_audio_clip_to_play(ScriptAudioClip *clip, int priority, int repeat);
ScriptAudioChannel* play_audio_clip_on_channel(int channel, ScriptAudioClip *clip, int priority, int repeat, int fromOffset, SOUNDCLIP *cachedClip = NULL);
void remove_clips_of_type_from_queue(int audioType);
ScriptAudioChannel* play_audio_clip(ScriptAudioClip *clip, int priority, int repeat, int fromOffset, bool queueIfNoChannel);
void play_audio_clip_by_index(int audioClipIndex);
int System_GetAudioChannelCount();
ScriptAudioChannel* System_GetAudioChannels(int index);
void Game_StopAudio(int audioType);
int Game_IsAudioPlaying(int audioType);
void Game_SetAudioTypeSpeechVolumeDrop(int audioType, int volumeDrop);
void Game_SetAudioTypeVolume(int audioType, int volume, int changeType);
void register_audio_script_functions();
bool unserialize_audio_script_object(int index, const char *objectType, const char *serializedData, int dataSize);
ScriptAudioClip* get_audio_clip_for_old_style_number(bool isMusic, int indexNumber);
// ***** BACKWARDS COMPATIBILITY WITH OLD AUDIO SYSTEM ***** //
int get_old_style_number_for_sound(int sound_number);
SOUNDCLIP *load_sound_clip_from_old_style_number(bool isMusic, int indexNumber, bool repeat);

extern SOUNDCLIP *channels[MAX_SOUND_CHANNELS+1];

#if !defined(IOS_VERSION) && !defined(PSP_VERSION) && !defined(ANDROID_VERSION)
extern volatile int psp_audio_multithreaded;
#endif

#endif // __AC_AUDIO_H