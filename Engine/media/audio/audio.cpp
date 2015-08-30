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

#include <stdio.h>
#include "util/wgt2allg.h"
#include "media/audio/audio.h"
#include "ac/gamesetupstruct.h"
#include "ac/dynobj/cc_audioclip.h"
#include "ac/dynobj/cc_audiochannel.h"
#include "ac/gamestate.h"
#include "script/script_runtime.h"
#include "ac/audiochannel.h"
#include "ac/audioclip.h"
#include "ac/gamesetup.h"
#include "media/audio/sound.h"
#include "debug/debug_log.h"
#include "debug/debugger.h"
#include "ac/common.h"
#include "ac/global_audio.h"
#include "ac/roomstruct.h"
#include <math.h>
#include "util/stream.h"
#include "core/assetmanager.h"

using AGS::Common::Stream;

AGS::Engine::Mutex _audio_mutex;
volatile bool _audio_doing_crossfade;

extern GameSetupStruct game;
extern GameSetup usetup;
extern GameState play;
extern roomstruct thisroom;
extern CharacterInfo*playerchar;

extern int psp_is_old_datafile;

extern volatile int switching_away_from_game;

#if !defined(IOS_VERSION) && !defined(PSP_VERSION) && !defined(ANDROID_VERSION)
volatile int psp_audio_multithreaded = 0;
#endif

ScriptAudioChannel scrAudioChannel[MAX_SOUND_CHANNELS + 1];
CCAudioChannel ccDynamicAudio;
CCAudioClip ccDynamicAudioClip;
char acaudio_buffer[256];
int reserved_channel_count = 0;

AGS::Engine::Thread audioThread;

void calculate_reserved_channel_count()
{
    int reservedChannels = 0;
    for (int i = 0; i < game.audioClipTypeCount; i++)
    {
        reservedChannels += game.audioClipTypes[i].reservedChannels;
    }
    reserved_channel_count = reservedChannels;
}

void register_audio_script_objects()
{
    int ee;
    for (ee = 0; ee <= MAX_SOUND_CHANNELS; ee++) 
    {
        scrAudioChannel[ee].id = ee;
        ccRegisterManagedObject(&scrAudioChannel[ee], &ccDynamicAudio);
    }

    for (ee = 0; ee < game.audioClipCount; ee++)
    {
        game.audioClips[ee].id = ee;
        ccRegisterManagedObject(&game.audioClips[ee], &ccDynamicAudioClip);
        ccAddExternalDynamicObject(game.audioClips[ee].scriptName, &game.audioClips[ee], &ccDynamicAudioClip);
    }

    calculate_reserved_channel_count();
}

void update_clip_default_volume(ScriptAudioClip *audioClip)
{
    if (play.default_audio_type_volumes[audioClip->type] >= 0) 
    {
        audioClip->defaultVolume = play.default_audio_type_volumes[audioClip->type];
    }
}

void start_fading_in_new_track_if_applicable(int fadeInChannel, ScriptAudioClip *newSound)
{
    int crossfadeSpeed = game.audioClipTypes[newSound->type].crossfadeSpeed;
    if (crossfadeSpeed > 0)
    {
        update_clip_default_volume(newSound);
        play.crossfade_in_volume_per_step = crossfadeSpeed;
        play.crossfade_final_volume_in = newSound->defaultVolume;
        play.crossfading_in_channel = fadeInChannel;
    }
}

void move_track_to_crossfade_channel(int currentChannel, int crossfadeSpeed, int fadeInChannel, ScriptAudioClip *newSound)
{
    stop_and_destroy_channel(SPECIAL_CROSSFADE_CHANNEL);
    channels[SPECIAL_CROSSFADE_CHANNEL] = channels[currentChannel];
    channels[currentChannel] = NULL;

    play.crossfading_out_channel = SPECIAL_CROSSFADE_CHANNEL;
    play.crossfade_step = 0;
    play.crossfade_initial_volume_out = channels[SPECIAL_CROSSFADE_CHANNEL]->get_volume();
    play.crossfade_out_volume_per_step = crossfadeSpeed;

    play.crossfading_in_channel = fadeInChannel;
    if (newSound != NULL)
    {
        start_fading_in_new_track_if_applicable(fadeInChannel, newSound);
    }
}

void stop_or_fade_out_channel(int fadeOutChannel, int fadeInChannel, ScriptAudioClip *newSound)
{
    ScriptAudioClip *sourceClip = AudioChannel_GetPlayingClip(&scrAudioChannel[fadeOutChannel]);
    if ((sourceClip != NULL) && (game.audioClipTypes[sourceClip->type].crossfadeSpeed > 0))
    {
        move_track_to_crossfade_channel(fadeOutChannel, game.audioClipTypes[sourceClip->type].crossfadeSpeed, fadeInChannel, newSound);
    }
    else
    {
        stop_and_destroy_channel(fadeOutChannel);
    }
}

const char* get_audio_clip_file_name(ScriptAudioClip *clip)
{
    if (game.audioClips[clip->id].bundlingType == AUCL_BUNDLE_EXE)
    {
        strcpy(acaudio_buffer, game.audioClips[clip->id].fileName);
        Stream *in = Common::AssetManager::OpenAsset(acaudio_buffer);
        if (in != NULL)
        {
            // CHECKME: so, what was that? a file exists check?
            delete in;
            return &acaudio_buffer[0];
        }
    }
    else
    {
        sprintf(acaudio_buffer, (psp_is_old_datafile ? "~music.vox~%s" : "~audio.vox~%s"), game.audioClips[clip->id].fileName);
        PACKFILE *iii = pack_fopen(acaudio_buffer, "rb");
        if (iii != NULL)
        {
            pack_fclose(iii);
            return &acaudio_buffer[0];
        }
    }
    sprintf(acaudio_buffer, "AudioCache\\%s", game.audioClips[clip->id].fileName);
    if (exists(acaudio_buffer))
    {
        return &acaudio_buffer[0];
    }
    return NULL;
}


int find_free_audio_channel(ScriptAudioClip *clip, int priority, bool interruptEqualPriority)
{
    int lowestPrioritySoFar = 9999999;
    int lowestPriorityID = -1;
    int channelToUse = -1;

    if (!interruptEqualPriority)
        priority--;

    int startAtChannel = reserved_channel_count;
    int endBeforeChannel = MAX_SOUND_CHANNELS;

    if (game.audioClipTypes[clip->type].reservedChannels > 0)
    {
        startAtChannel = 0;
        for (int i = 0; i < clip->type; i++)
        {
            startAtChannel += game.audioClipTypes[i].reservedChannels;
        }
        endBeforeChannel = startAtChannel + game.audioClipTypes[clip->type].reservedChannels;
    }

    for (int i = startAtChannel; i < endBeforeChannel; i++)
    {
        if ((channels[i] == NULL) || (channels[i]->done))
        {
            channelToUse = i;
            stop_and_destroy_channel(i);
            break;
        }
        if ((channels[i]->priority < lowestPrioritySoFar) &&
            (channels[i]->soundType == clip->type))
        {
            lowestPrioritySoFar = channels[i]->priority;
            lowestPriorityID = i;
        }
    }

    if ((channelToUse < 0) && (lowestPriorityID >= 0) &&
        (lowestPrioritySoFar <= priority))
    {
        stop_or_fade_out_channel(lowestPriorityID, lowestPriorityID, clip);
        channelToUse = lowestPriorityID;
    }
    else if ((channelToUse >= 0) && (play.crossfading_in_channel < 1))
    {
        start_fading_in_new_track_if_applicable(channelToUse, clip);
    }
    return channelToUse;
}

bool is_audiotype_allowed_to_play(AudioFileType type)
{
    return type == eAudioFileMIDI && usetup.midicard != MIDI_NONE ||
           type != eAudioFileMIDI && usetup.digicard != DIGI_NONE;
}

SOUNDCLIP *load_sound_clip(ScriptAudioClip *audioClip, bool repeat)
{
    const char *clipFileName = get_audio_clip_file_name(audioClip);
    if ((clipFileName == NULL) || !is_audiotype_allowed_to_play((AudioFileType)audioClip->fileType))
    {
        return NULL;
    }

    update_clip_default_volume(audioClip);

    SOUNDCLIP *soundClip = NULL;
    switch (audioClip->fileType)
    {
    case eAudioFileOGG:
        soundClip = my_load_static_ogg(clipFileName, audioClip->defaultVolume, repeat);
        break;
    case eAudioFileMP3:
        soundClip = my_load_static_mp3(clipFileName, audioClip->defaultVolume, repeat);
        break;
    case eAudioFileWAV:
    case eAudioFileVOC:
        soundClip = my_load_wave(clipFileName, audioClip->defaultVolume, repeat);
        break;
    case eAudioFileMIDI:
        soundClip = my_load_midi(clipFileName, repeat);
        break;
    case eAudioFileMOD:
#ifndef PSP_NO_MOD_PLAYBACK
        soundClip = my_load_mod(clipFileName, repeat);
#else
        soundClip = NULL;
#endif
        break;
    default:
        quitprintf("AudioClip.Play: invalid audio file type encountered: %d", audioClip->fileType);
    }
    if (soundClip != NULL)
    {
        soundClip->set_volume_origin(audioClip->defaultVolume);
        soundClip->soundType = audioClip->type;
        soundClip->sourceClip = audioClip;
    }
    return soundClip;
}

void recache_queued_clips_after_loading_save_game()
{
    for (int i = 0; i < play.new_music_queue_size; i++)
    {
        play.new_music_queue[i].cachedClip = NULL;
    }
}

void audio_update_polled_stuff()
{
    play.crossfade_step++;

    if (play.crossfading_out_channel > 0)
    {
        if (channels[play.crossfading_out_channel] == NULL)
            quitprintf("Crossfade out channel is %d but channel has gone", play.crossfading_out_channel);

        int newVolume = channels[play.crossfading_out_channel]->get_volume() - play.crossfade_out_volume_per_step;
        if (newVolume > 0)
        {
            AudioChannel_SetVolume(&scrAudioChannel[play.crossfading_out_channel], newVolume);
        }
        else
        {
            stop_and_destroy_channel(play.crossfading_out_channel);
            play.crossfading_out_channel = 0;
        }
    }

    if (play.crossfading_in_channel > 0)
    {
        int newVolume = channels[play.crossfading_in_channel]->get_volume() + play.crossfade_in_volume_per_step;
        if (newVolume > play.crossfade_final_volume_in)
        {
            newVolume = play.crossfade_final_volume_in;
        }

        AudioChannel_SetVolume(&scrAudioChannel[play.crossfading_in_channel], newVolume);

        if (newVolume >= play.crossfade_final_volume_in)
        {
            play.crossfading_in_channel = 0;
        }
    }

    if (play.new_music_queue_size > 0)
    {
        for (int i = 0; i < play.new_music_queue_size; i++)
        {
            ScriptAudioClip *clip = &game.audioClips[play.new_music_queue[i].audioClipIndex];
            int channel = find_free_audio_channel(clip, clip->defaultPriority, false);
            if (channel >= 0)
            {
                QueuedAudioItem itemToPlay = play.new_music_queue[i];

                play.new_music_queue_size--;
                for (int j = i; j < play.new_music_queue_size; j++)
                {
                    play.new_music_queue[j] = play.new_music_queue[j + 1];
                }

                play_audio_clip_on_channel(channel, clip, itemToPlay.priority, itemToPlay.repeat, 0, itemToPlay.cachedClip);
                i--;
            }
        }
    }
}

void queue_audio_clip_to_play(ScriptAudioClip *clip, int priority, int repeat)
{
    if (play.new_music_queue_size >= MAX_QUEUED_MUSIC) {
        DEBUG_CONSOLE("Too many queued music, cannot add %s", clip->scriptName);
        return;
    }

    SOUNDCLIP *cachedClip = load_sound_clip(clip, (repeat != 0));
    if (cachedClip != NULL) 
    {
        play.new_music_queue[play.new_music_queue_size].audioClipIndex = clip->id;
        play.new_music_queue[play.new_music_queue_size].priority = priority;
        play.new_music_queue[play.new_music_queue_size].repeat = (repeat != 0);
        play.new_music_queue[play.new_music_queue_size].cachedClip = cachedClip;
        play.new_music_queue_size++;
    }
    
    if (!psp_audio_multithreaded)
      update_polled_mp3();
}

ScriptAudioChannel* play_audio_clip_on_channel(int channel, ScriptAudioClip *clip, int priority, int repeat, int fromOffset, SOUNDCLIP *soundfx)
{
    if (soundfx == NULL)
    {
        soundfx = load_sound_clip(clip, (repeat) ? true : false);
    }
    if (soundfx == NULL)
    {
        DEBUG_CONSOLE("AudioClip.Play: unable to load sound file");
        if (play.crossfading_in_channel == channel)
        {
            play.crossfading_in_channel = 0;
        }
        return NULL;
    }
    soundfx->priority = priority;

    if (play.crossfading_in_channel == channel)
    {
        soundfx->set_volume_origin(0);
    }

    // Mute the audio clip if fast-forwarding the cutscene
    if (play.fast_forward) 
    {
        soundfx->set_volume_override(0);

        // CHECKME!!
        // [IKM] According to the 3.2.1 logic the clip will restore
        // its value after cutscene, but only if originalVolAsPercentage
        // is not zeroed. Something I am not sure about: why does it
        // disable the clip under condition that there's more than one
        // channel for this audio type? It does not even check if
        // anything of this type is currently playing.
        if (game.audioClipTypes[clip->type].reservedChannels != 1)
            soundfx->originalVolAsPercentage = 0;
    }

    if (soundfx->play_from(fromOffset) == 0)
    {
        DEBUG_CONSOLE("AudioClip.Play: failed to play sound file");
        return NULL;
    }

    last_sound_played[channel] = -1;
    channels[channel] = soundfx;
    return &scrAudioChannel[channel];
}

void remove_clips_of_type_from_queue(int audioType) 
{
    int aa;
    for (aa = 0; aa < play.new_music_queue_size; aa++)
    {
        ScriptAudioClip *clip = &game.audioClips[play.new_music_queue[aa].audioClipIndex];
        if (clip->type == audioType)
        {
            play.new_music_queue_size--;
            for (int bb = aa; bb < play.new_music_queue_size; bb++)
                play.new_music_queue[bb] = play.new_music_queue[bb + 1];
            aa--;
        }
    }
}

ScriptAudioChannel* play_audio_clip(ScriptAudioClip *clip, int priority, int repeat, int fromOffset, bool queueIfNoChannel)
{
    if (!queueIfNoChannel)
        remove_clips_of_type_from_queue(clip->type);

    if (priority == SCR_NO_VALUE)
        priority = clip->defaultPriority;
    if (repeat == SCR_NO_VALUE)
        repeat = clip->defaultRepeat;

    int channel = find_free_audio_channel(clip, priority, !queueIfNoChannel);
    if (channel < 0)
    {
        if (queueIfNoChannel)
            queue_audio_clip_to_play(clip, priority, repeat);
        else
            DEBUG_CONSOLE("AudioClip.Play: no channels available to interrupt PRI:%d TYPE:%d", priority, clip->type);

        return NULL;
    }

    return play_audio_clip_on_channel(channel, clip, priority, repeat, fromOffset);
}

void play_audio_clip_by_index(int audioClipIndex)
{
    if ((audioClipIndex >= 0) && (audioClipIndex < game.audioClipCount))
        AudioClip_Play(&game.audioClips[audioClipIndex], SCR_NO_VALUE, SCR_NO_VALUE);
}

bool unserialize_audio_script_object(int index, const char *objectType, const char *serializedData, int dataSize)
{
    if (strcmp(objectType, "AudioChannel") == 0)
    {
        ccDynamicAudio.Unserialize(index, serializedData, dataSize);
    }
    else if (strcmp(objectType, "AudioClip") == 0)
    {
        ccDynamicAudioClip.Unserialize(index, serializedData, dataSize);
    }
    else
    {
        return false;
    }
    return true;
}

ScriptAudioClip* get_audio_clip_for_old_style_number(bool isMusic, int indexNumber)
{
    char audioClipName[200];
    if (isMusic)
        sprintf(audioClipName, "aMusic%d", indexNumber);
    else
        sprintf(audioClipName, "aSound%d", indexNumber);

    for (int bb = 0; bb < game.audioClipCount; bb++)
    {
        if (stricmp(game.audioClips[bb].scriptName, audioClipName) == 0)
        {
            return &game.audioClips[bb];
        }
    }

    return NULL;
}



void stop_and_destroy_channel_ex(int chid, bool resetLegacyMusicSettings) {
    if ((chid < 0) || (chid > MAX_SOUND_CHANNELS))
        quit("!StopChannel: invalid channel ID");

    if (channels[chid] != NULL) {
        channels[chid]->destroy();
        delete channels[chid];
        channels[chid] = NULL;
    }

    if (play.crossfading_in_channel == chid)
        play.crossfading_in_channel = 0;
    if (play.crossfading_out_channel == chid)
        play.crossfading_out_channel = 0;

    // destroyed an ambient sound channel
    if (ambient[chid].channel > 0)
        ambient[chid].channel = 0;

    if ((chid == SCHAN_MUSIC) && (resetLegacyMusicSettings))
    {
        play.cur_music_number = -1;
        current_music_type = 0;
    }
}

void stop_and_destroy_channel (int chid) 
{
    stop_and_destroy_channel_ex(chid, true);
}



// ***** BACKWARDS COMPATIBILITY WITH OLD AUDIO SYSTEM ***** //

int get_old_style_number_for_sound(int sound_number)
{
    int audio_clip_id = 0;

    if (psp_is_old_datafile)
    {
        // No sound assigned.
        if (sound_number < 1)
            return 0;

        // Sound number is not yet updated to audio clip id.
        if (sound_number <= 0x10000000)
            return sound_number;

        // Remove audio clip id flag.
        audio_clip_id = sound_number - 0x10000000;
    }
    else
        audio_clip_id = sound_number;

    if (audio_clip_id >= 0)
    {
        int old_style_number = 0;
        if (sscanf(game.audioClips[audio_clip_id].scriptName, "aSound%d", &old_style_number) == 1)
            return old_style_number;    
    }
    return 0;
}

SOUNDCLIP *load_sound_clip_from_old_style_number(bool isMusic, int indexNumber, bool repeat)
{
    ScriptAudioClip* audioClip = get_audio_clip_for_old_style_number(isMusic, indexNumber);

    if (audioClip != NULL)
    {
        return load_sound_clip(audioClip, repeat);
    }

    return NULL;
}

//=============================================================================

int last_sound_played[MAX_SOUND_CHANNELS + 1];

void force_audiostream_include() {
    // This should never happen, but the call is here to make it
    // link the audiostream libraries
    stop_audio_stream(NULL);
}

AmbientSound ambient[MAX_SOUND_CHANNELS + 1];  // + 1 just for safety on array iterations

int get_volume_adjusted_for_distance(int volume, int sndX, int sndY, int sndMaxDist)
{
    int distx = playerchar->x - sndX;
    int disty = playerchar->y - sndY;
    // it uses Allegro's "fix" sqrt without the ::
    int dist = (int)::sqrt((double)(distx*distx + disty*disty));

    // if they're quite close, full volume
    int wantvol = volume;

    if (dist >= AMBIENCE_FULL_DIST)
    {
        // get the relative volume
        wantvol = ((dist - AMBIENCE_FULL_DIST) * volume) / sndMaxDist;
        // closer is louder
        wantvol = volume - wantvol;
    }

    return wantvol;
}

void update_directional_sound_vol()
{
    for (int chan = 1; chan < MAX_SOUND_CHANNELS; chan++) 
    {
        if ((channels[chan] != NULL) && (channels[chan]->done == 0) &&
            (channels[chan]->xSource >= 0)) 
        {
            channels[chan]->directionalVolModifier = 
                get_volume_adjusted_for_distance(channels[chan]->vol, 
                channels[chan]->xSource,
                channels[chan]->ySource,
                channels[chan]->maximumPossibleDistanceAway) -
                channels[chan]->vol;

            channels[chan]->set_volume(channels[chan]->vol);
        }
    }
}

void update_ambient_sound_vol () {

    for (int chan = 1; chan < MAX_SOUND_CHANNELS; chan++) {

        AmbientSound *thisSound = &ambient[chan];

        if (thisSound->channel == 0)
            continue;

        int sourceVolume = thisSound->vol;

        if ((channels[SCHAN_SPEECH] != NULL) && (channels[SCHAN_SPEECH]->done == 0)) {
            // Negative value means set exactly; positive means drop that amount
            if (play.speech_music_drop < 0)
                sourceVolume = -play.speech_music_drop;
            else
                sourceVolume -= play.speech_music_drop;

            if (sourceVolume < 0)
                sourceVolume = 0;
            if (sourceVolume > 255)
                sourceVolume = 255;
        }

        // Adjust ambient volume so it maxes out at overall sound volume
        int ambientvol = (sourceVolume * play.sound_volume) / 255;

        int wantvol;

        if ((thisSound->x == 0) && (thisSound->y == 0)) {
            wantvol = ambientvol;
        }
        else {
            wantvol = get_volume_adjusted_for_distance(ambientvol, thisSound->x, thisSound->y, thisSound->maxdist);
        }

        if (channels[thisSound->channel] == NULL)
            quit("Internal error: the ambient sound channel is enabled, but it has been destroyed");

        channels[thisSound->channel]->set_volume(wantvol);
    }
}

SOUNDCLIP *load_sound_and_play(ScriptAudioClip *aclip, bool repeat)
{
    SOUNDCLIP *soundfx = load_sound_clip(aclip, repeat);

    if (soundfx != NULL) {
        if (soundfx->play() == 0)
            soundfx = NULL;
    }

    return soundfx;
}

void stop_all_sound_and_music() 
{
    int a;
    stopmusic();
    // make sure it doesn't start crossfading when it comes back
    crossFading = 0;
    // any ambient sound will be aborted
    for (a = 0; a <= MAX_SOUND_CHANNELS; a++)
        stop_and_destroy_channel(a);
}

void shutdown_sound() 
{
    stop_all_sound_and_music();

#ifndef PSP_NO_MOD_PLAYBACK
    if (opts.mod_player)
        remove_mod_player();
#endif
    remove_sound();
}

// the sound will only be played if there is a free channel or
// it has a priority >= an existing sound to override
int play_sound_priority (int val1, int priority) {
    int lowest_pri = 9999, lowest_pri_id = -1;

    // find a free channel to play it on
    for (int i = SCHAN_NORMAL; i < numSoundChannels; i++) {
        if (val1 < 0) {
            // Playing sound -1 means iterate through and stop all sound
            if ((channels[i] != NULL) && (channels[i]->done == 0))
                stop_and_destroy_channel (i);
        }
        else if ((channels[i] == NULL) || (channels[i]->done != 0)) {
            if (PlaySoundEx(val1, i) >= 0)
                channels[i]->priority = priority;
            return i;
        }
        else if (channels[i]->priority < lowest_pri) {
            lowest_pri = channels[i]->priority;
            lowest_pri_id = i;
        }

    }
    if (val1 < 0)
        return -1;

    // no free channels, see if we have a high enough priority
    // to override one
    if (priority >= lowest_pri) {
        if (PlaySoundEx(val1, lowest_pri_id) >= 0) {
            channels[lowest_pri_id]->priority = priority;
            return lowest_pri_id;
        }
    }

    return -1;
}

int play_sound(int val1) {
    return play_sound_priority(val1, 10);
}

//=============================================================================



int current_music_type = 0;
// crossFading is >0 (channel number of new track), or -1 (old
// track fading out, no new track)
int crossFading = 0, crossFadeVolumePerStep = 0, crossFadeStep = 0;
int crossFadeVolumeAtStart = 0;
SOUNDCLIP *cachedQueuedMusic = NULL;

int musicPollIterator; // long name so it doesn't interfere with anything else

volatile int mvolcounter = 0;
int update_music_at=0;



/*
#include "almp3_old.h"
ALLEGRO_MP3 *mp3ptr;
int mp3vol=128;

void amp_setvolume(int newvol) { mp3vol=newvol; }
int load_amp(char*namm,int loop) {
mp3ptr = new ALLEGRO_MP3(namm);
if (mp3ptr == NULL) return 0;
if (mp3ptr->get_error_code() != 0) {
delete mp3ptr;
return 0;
}
mp3ptr->play(mp3vol, 8192);
return 1;
}
void install_amp() { }
void unload_amp() {
mp3ptr->stop();
delete mp3ptr;
}
int amp_decode() {
mp3ptr->poll();
if (mp3ptr->is_finished()) {
if (play.music_repeat)
mp3ptr->play(mp3vol, 8192);
else return -1;
}
return 0;
}
*/
//#endif

void clear_music_cache() {

    if (cachedQueuedMusic != NULL) {
        cachedQueuedMusic->destroy();
        delete cachedQueuedMusic;
        cachedQueuedMusic = NULL;
    }

}

void play_next_queued() {
    // check if there's a queued one to play
    if (play.music_queue_size > 0) {

        int tuneToPlay = play.music_queue[0];

        if (tuneToPlay >= QUEUED_MUSIC_REPEAT) {
            // Loop it!
            play.music_repeat++;
            play_new_music(tuneToPlay - QUEUED_MUSIC_REPEAT, cachedQueuedMusic);
            play.music_repeat--;
        }
        else {
            // Don't loop it!
            int repeatWas = play.music_repeat;
            play.music_repeat = 0;
            play_new_music(tuneToPlay, cachedQueuedMusic);
            play.music_repeat = repeatWas;
        }

        // don't free the memory, as it has been transferred onto the
        // main music channel
        cachedQueuedMusic = NULL;

        play.music_queue_size--;
        for (int i = 0; i < play.music_queue_size; i++)
            play.music_queue[i] = play.music_queue[i + 1];

        if (play.music_queue_size > 0)
            cachedQueuedMusic = load_music_from_disk(play.music_queue[0], 0);
    }

}

int calculate_max_volume() {
    // quieter so that sounds can be heard better
    int newvol=play.music_master_volume + ((int)thisroom.options[ST_VOLUME]) * LegacyRoomVolumeFactor;
    if (newvol>255) newvol=255;
    if (newvol<0) newvol=0;

    if (play.fast_forward)
        newvol = 0;

    return newvol;
}

// add/remove the volume drop to the audio channels while speech is playing
void apply_volume_drop_modifier(bool applyModifier)
{
    for (int i = 0; i < MAX_SOUND_CHANNELS; i++) 
    {
        if ((channels[i] != NULL) && (channels[i]->done == 0) && (channels[i]->sourceClip != NULL))
        {
            if (applyModifier)
            {
                int audioType = ((ScriptAudioClip*)channels[i]->sourceClip)->type;
                channels[i]->apply_volume_modifier(-(game.audioClipTypes[audioType].volume_reduction_while_speech_playing * 255 / 100));
            }
            else
                channels[i]->apply_volume_modifier(0);
        }
    }
}

extern volatile char want_exit;
extern int frames_per_second;

void update_mp3_thread()
{
	while (switching_away_from_game) { }
	AGS::Engine::MutexLock _lock(_audio_mutex);
	for (musicPollIterator = 0; musicPollIterator <= MAX_SOUND_CHANNELS; ++musicPollIterator)
	{
		if ((channels[musicPollIterator] != NULL) && (channels[musicPollIterator]->done == 0))
			channels[musicPollIterator]->poll();
	}
}

void update_mp3()
{
	if (!psp_audio_multithreaded) update_mp3_thread();
}

void update_polled_mp3() {
    update_mp3();

        if (mvolcounter > update_music_at) {
            update_music_volume();
            apply_volume_drop_modifier(false);
            update_music_at = 0;
            mvolcounter = 0;
            update_ambient_sound_vol();
        }
}

// Update the music, and advance the crossfade on a step
// (this should only be called once per game loop)
void update_polled_audio_and_crossfade ()
{
	update_polled_stuff_if_runtime ();

	AGS::Engine::MutexLock _lock(_audio_mutex);

    _audio_doing_crossfade = true;

    audio_update_polled_stuff();

    if (crossFading) {
        crossFadeStep++;
        update_music_volume();
    }

    // Check if the current music has finished playing
    if ((play.cur_music_number >= 0) && (play.fast_forward == 0)) {
        if (IsMusicPlaying() == 0) {
            // The current music has finished
            play.cur_music_number = -1;
            play_next_queued();
        }
        else if ((game.options[OPT_CROSSFADEMUSIC] > 0) &&
            (play.music_queue_size > 0) && (!crossFading)) {
                // want to crossfade, and new tune in the queue
                int curpos = channels[SCHAN_MUSIC]->get_pos_ms();
                int muslen = channels[SCHAN_MUSIC]->get_length_ms();
                if ((curpos > 0) && (muslen > 0)) {
                    // we want to crossfade, and we know how far through
                    // the tune we are
                    int takesSteps = calculate_max_volume() / game.options[OPT_CROSSFADEMUSIC];
                    int takesMs = (takesSteps * 1000) / frames_per_second;
                    if (curpos >= muslen - takesMs)
                        play_next_queued();
                }
        }
    }

    _audio_doing_crossfade = false;

}


void stopmusic() {

    if (crossFading > 0) {
        // stop in the middle of a new track fading in
        // Abort the new track, and let the old one finish fading out
        stop_and_destroy_channel (crossFading);
        crossFading = -1;
    }
    else if (crossFading < 0) {
        // the music is already fading out
        if (game.options[OPT_CROSSFADEMUSIC] <= 0) {
            // If they have since disabled crossfading, stop the fadeout
            stop_and_destroy_channel(SCHAN_MUSIC);
            crossFading = 0;
            crossFadeStep = 0;
            update_music_volume();
        }
    }
    else if ((game.options[OPT_CROSSFADEMUSIC] > 0)
        && (channels[SCHAN_MUSIC] != NULL)
        && (channels[SCHAN_MUSIC]->done == 0)
        && (current_music_type != 0)
        && (current_music_type != MUS_MIDI)
        && (current_music_type != MUS_MOD)) {

            crossFading = -1;
            crossFadeStep = 0;
            crossFadeVolumePerStep = game.options[OPT_CROSSFADEMUSIC];
            crossFadeVolumeAtStart = calculate_max_volume();
    }
    else
        stop_and_destroy_channel (SCHAN_MUSIC);

    play.cur_music_number = -1;
    current_music_type = 0;
}

void update_music_volume() {

    if ((current_music_type) || (crossFading < 0)) 
    {
        // targetVol is the maximum volume we're fading in to
        // newvol is the starting volume that we faded out from
        int targetVol = calculate_max_volume();
        int newvol;
        if (crossFading)
            newvol = crossFadeVolumeAtStart;
        else
            newvol = targetVol;

        // fading out old track, target volume is silence
        if (crossFading < 0)
            targetVol = 0;

        if (crossFading) {
            int curvol = crossFadeVolumePerStep * crossFadeStep;

            if ((curvol > targetVol) && (curvol > newvol)) {
                // it has fully faded to the new track
                newvol = targetVol;
                stop_and_destroy_channel_ex(SCHAN_MUSIC, false);
                if (crossFading > 0) {
                    channels[SCHAN_MUSIC] = channels[crossFading];
                    channels[crossFading] = NULL;
                }
                crossFading = 0;
            }
            else {
                if (crossFading > 0)
                    channels[crossFading]->set_volume((curvol > targetVol) ? targetVol : curvol);

                newvol -= curvol;
                if (newvol < 0)
                    newvol = 0;
            }
        }
        if (channels[SCHAN_MUSIC])
            channels[SCHAN_MUSIC]->set_volume (newvol);
    }
}

// Ensures crossfader is stable after loading (or failing to load)
// new music
void post_new_music_check (int newchannel) {
    if ((crossFading > 0) && (channels[crossFading] == NULL)) {
        crossFading = 0;
        // Was fading out but then they played invalid music, continue
        // to fade out
        if (channels[SCHAN_MUSIC] != NULL)
            crossFading = -1;
    }

}

// Sets up the crossfading for playing the new music track,
// and returns the channel number to use
int prepare_for_new_music () {
    int useChannel = SCHAN_MUSIC;

    if ((game.options[OPT_CROSSFADEMUSIC] > 0)
        && (channels[SCHAN_MUSIC] != NULL)
        && (channels[SCHAN_MUSIC]->done == 0)
        && (current_music_type != MUS_MIDI)
        && (current_music_type != MUS_MOD)) {

            if (crossFading > 0) {
                // It's still crossfading to the previous track
                stop_and_destroy_channel_ex(SCHAN_MUSIC, false);
                channels[SCHAN_MUSIC] = channels[crossFading];
                channels[crossFading] = NULL;
                crossFading = 0;
                update_music_volume();
            }
            else if (crossFading < 0) {
                // an old track is still fading out, no new music yet
                // Do nothing, and keep the current crossfade step
            }
            else {
                // start crossfading
                crossFadeStep = 0;
                crossFadeVolumePerStep = game.options[OPT_CROSSFADEMUSIC];
                crossFadeVolumeAtStart = calculate_max_volume();
            }
            useChannel = SPECIAL_CROSSFADE_CHANNEL;
            crossFading = useChannel;
    }
    else {
        // crossfading is now turned off
        stopmusic();
        // ensure that any traces of old tunes fading are eliminated
        // (otherwise the new track will be faded out)
        crossFading = 0;
    }

    // Just make sure, because it will be overwritten in a sec
    if (channels[useChannel] != NULL)
        stop_and_destroy_channel (useChannel);

    return useChannel;
}

ScriptAudioClip *get_audio_clip_for_music(int mnum)
{
    if (mnum >= QUEUED_MUSIC_REPEAT)
        mnum -= QUEUED_MUSIC_REPEAT;
    return get_audio_clip_for_old_style_number(true, mnum);
}

SOUNDCLIP *load_music_from_disk(int mnum, bool doRepeat) {

    if (mnum >= QUEUED_MUSIC_REPEAT) {
        mnum -= QUEUED_MUSIC_REPEAT;
        doRepeat = true;
    }

    SOUNDCLIP *loaded = load_sound_clip_from_old_style_number(true, mnum, doRepeat);

    if ((loaded == NULL) && (mnum > 0)) 
    {
        debug_log("Music %d not found",mnum);
        DEBUG_CONSOLE("FAILED to load music %d", mnum);
    }

    return loaded;
}


void play_new_music(int mnum, SOUNDCLIP *music) {
    if (debug_flags & DBG_NOMUSIC)
        return;

    if ((play.cur_music_number == mnum) && (music == NULL)) {
        DEBUG_CONSOLE("PlayMusic %d but already playing", mnum);
        return;  // don't play the music if it's already playing
    }

    ScriptAudioClip *aclip = get_audio_clip_for_music(mnum);
    if (aclip && !is_audiotype_allowed_to_play((AudioFileType)aclip->fileType))
        return;

    int useChannel = SCHAN_MUSIC;
    DEBUG_CONSOLE("Playing music %d", mnum);

    if (mnum<0) {
        stopmusic();
        return;
    }

    if (play.fast_forward) {
        // while skipping cutscene, don't change the music
        play.end_cutscene_music = mnum;
        return;
    }

    useChannel = prepare_for_new_music ();

    play.cur_music_number=mnum;
    current_music_type = 0;
    channels[useChannel] = NULL;

    play.current_music_repeating = play.music_repeat;
    // now that all the previous music is unloaded, load in the new one

    if (music != NULL) {
        channels[useChannel] = music;
        music = NULL;
    }
    else {
        channels[useChannel] = load_music_from_disk(mnum, (play.music_repeat > 0));
    }

    if (channels[useChannel] != NULL) {

        if (channels[useChannel]->play() == 0)
            channels[useChannel] = NULL;
        else
            current_music_type = channels[useChannel]->get_sound_type();
    }

    post_new_music_check(useChannel);

    update_music_volume();

}

void newmusic(int mnum) {
    play_new_music(mnum, NULL);
}
