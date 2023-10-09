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
#include <math.h>
#include "core/platform.h"
#include "media/audio/audio.h"
#include "ac/audiocliptype.h"
#include "ac/gamesetupstruct.h"
#include "ac/dynobj/cc_audioclip.h"
#include "ac/dynobj/cc_audiochannel.h"
#include "ac/gamestate.h"
#include "script/script_runtime.h"
#include "ac/audiochannel.h"
#include "ac/audioclip.h"
#include "ac/game.h"
#include "ac/gamesetup.h"
#include "ac/path_helper.h"
#include "ac/view.h"
#include "media/audio/sound.h"
#include "debug/debug_log.h"
#include "debug/debugger.h"
#include "ac/common.h"
#include "ac/file.h"
#include "ac/global_audio.h"
#include <math.h>
#include "util/stream.h"
#include "core/assetmanager.h"
#include "ac/timer.h"
#include "main/game_run.h"
#include "media/audio/audio_core.h"
#include "platform/base/sys_main.h"
#include "ac/dynobj/dynobj_manager.h"

using namespace AGS::Common;
using namespace AGS::Engine;

//-----------------------
//sound channel management

static std::array<std::unique_ptr<SOUNDCLIP>, TOTAL_AUDIO_CHANNELS> _channels;

SOUNDCLIP *AudioChans::GetChannel(int index)
{
    return _channels[index].get();
}

SOUNDCLIP *AudioChans::GetChannelIfPlaying(int index)
{
    auto *ch = _channels[index].get();
    return (ch != nullptr && ch->is_ready()) ? ch : nullptr;
}

SOUNDCLIP *AudioChans::SetChannel(int index, std::unique_ptr<SOUNDCLIP> ch)
{
    if ((ch != nullptr) && (_channels[index] != nullptr))
        Debug::Printf(kDbgMsg_Warn, "WARNING: channel %d - clip overwritten", index);
    _channels[index] = std::move(ch);
    return _channels[index].get();
}

SOUNDCLIP *AudioChans::MoveChannel(int to, int from)
{
    return SetChannel(to, std::move(_channels[from]));
}

void AudioChans::DeleteClipOnChannel(int index)
{
    _channels[index].reset();
}

extern GameSetupStruct game;
extern GameState play;
extern RoomStruct thisroom;
extern CharacterInfo*playerchar;
extern CCAudioChannel ccDynamicAudio;

extern volatile int switching_away_from_game;
extern int frames_per_second; // for queue "hack"

ScriptAudioChannel scrAudioChannel[MAX_GAME_CHANNELS];
int reserved_channel_count = 0;

void calculate_reserved_channel_count()
{
    int reservedChannels = 0;
    for (size_t i = 0; i < game.audioClipTypes.size(); i++)
    {
        reservedChannels += game.audioClipTypes[i].reservedChannels;
    }
    reserved_channel_count = reservedChannels;
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

static void move_track_to_crossfade_channel(int currentChannel, int crossfadeSpeed, int fadeInChannel, ScriptAudioClip *newSound)
{
    stop_and_destroy_channel(SPECIAL_CROSSFADE_CHANNEL);
    auto *cfade_clip = AudioChans::MoveChannel(SPECIAL_CROSSFADE_CHANNEL, currentChannel);
    if (!cfade_clip)
        return;

    play.crossfading_out_channel = SPECIAL_CROSSFADE_CHANNEL;
    play.crossfade_step = 0;
    play.crossfade_initial_volume_out = cfade_clip->get_volume100();
    play.crossfade_out_volume_per_step = crossfadeSpeed;

    play.crossfading_in_channel = fadeInChannel;
    if (newSound != nullptr)
    {
        start_fading_in_new_track_if_applicable(fadeInChannel, newSound);
    }
}

// NOTE: this function assumes one of the user channels
void stop_or_fade_out_channel(int fadeOutChannel, int fadeInChannel, ScriptAudioClip *newSound)
{
    ScriptAudioClip *sourceClip = AudioChannel_GetPlayingClip(&scrAudioChannel[fadeOutChannel]);
    if ((play.fast_forward == 0) && // don't crossfade if skipping a cutscene
        (sourceClip != nullptr) && (game.audioClipTypes[sourceClip->type].crossfadeSpeed > 0))
    {
        move_track_to_crossfade_channel(fadeOutChannel, game.audioClipTypes[sourceClip->type].crossfadeSpeed, fadeInChannel, newSound);
    }
    else
    {
        stop_and_destroy_channel(fadeOutChannel);
    }
}

static int find_free_audio_channel(ScriptAudioClip *clip, int priority, bool interruptEqualPriority,
    bool for_queue = true)
{
    int lowestPrioritySoFar = 9999999;
    int lowestPriorityID = -1;
    int channelToUse = -1;

    if (!interruptEqualPriority)
        priority--;

    int startAtChannel = reserved_channel_count;
    int endBeforeChannel = game.numGameChannels;

    if (game.audioClipTypes[clip->type].reservedChannels > 0)
    {
        startAtChannel = 0;
        for (int i = 0; i < clip->type; i++)
        {
            startAtChannel += game.audioClipTypes[i].reservedChannels;
        }
        // NOTE: we allow to place sound on a crossfade channel for backward compatibility,
        // but ONLY for the case of audio type with reserved channels (weird quirk).
        endBeforeChannel = std::min(game.numCompatGameChannels,
            startAtChannel + game.audioClipTypes[clip->type].reservedChannels);
    }

    for (int i = startAtChannel; i < endBeforeChannel; i++)
    {
        auto* ch = AudioChans::GetChannelIfPlaying(i);
        if (ch == nullptr)
        {
            channelToUse = i;
            stop_and_destroy_channel(i);
            break;
        }
        if ((ch->priority < lowestPrioritySoFar) &&
            (ch->sourceClipType == clip->type))
        {
            lowestPrioritySoFar = ch->priority;
            lowestPriorityID = i;
        }
        // NOTE: This is a "hack" for starting queued clips;
        // since having a new audio system (3.6.0 onwards), the audio timing
        // changed a little, and queued sounds have to start bit earlier
        // if we want them to sound seamless with the previous clips.
        // TODO: investigate better solutions? may require reimplementation of the sound queue.
        if (for_queue && (ch->sourceClipType == clip->type))
        { // try to start queued sounds 1 frame earlier
            const float trigger_pos = (1000.f / frames_per_second) * 1.f;
            if (ch->get_pos_ms() >= (ch->get_length_ms() - trigger_pos))
            {
                lowestPrioritySoFar = priority;
                lowestPriorityID = i;
            }
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

bool is_audiotype_allowed_to_play(AudioFileType /*type*/)
{ // TODO: this is a remnant of an old audio logic, think this function over
    return usetup.audio_enabled;
}

SOUNDCLIP *load_sound_clip(ScriptAudioClip *audioClip, bool repeat)
{
    if (!is_audiotype_allowed_to_play((AudioFileType)audioClip->fileType))
    {
        return nullptr;
    }

    update_clip_default_volume(audioClip);

    SOUNDCLIP *soundClip = nullptr;
    AssetPath asset_name = get_audio_clip_assetpath(audioClip->bundlingType, audioClip->fileName);
    const char *ext = "";
    switch (audioClip->fileType)
    {
    case eAudioFileOGG:
        ext = "ogg"; break;
    case eAudioFileMP3:
        ext = "mp3"; break;
    case eAudioFileWAV:
    case eAudioFileVOC:
        ext = "wav"; break;
    case eAudioFileMIDI:
        ext = "mid"; break;
    case eAudioFileMOD:
        ext = "mod"; break;
    case eAudioFileFLAC:
        ext = "flac"; break;
        break;
    default:
        quitprintf("AudioClip.Play: invalid audio file type encountered: %d", audioClip->fileType);
    }

    soundClip = load_sound_clip(asset_name, ext, repeat);
    if (soundClip != nullptr)
    {
        soundClip->set_volume100(audioClip->defaultVolume);
        soundClip->sourceClipID = audioClip->id;
        soundClip->sourceClipType = audioClip->type;
    }
    return soundClip;
}

static void audio_update_polled_stuff()
{
    ///////////////////////////////////////////////////////////////////////////
    // Do crossfade
    play.crossfade_step++;

    if (play.crossfading_out_channel > 0 && !AudioChans::GetChannelIfPlaying(play.crossfading_out_channel))
        play.crossfading_out_channel = 0;

    if (play.crossfading_out_channel > 0)
    {
        SOUNDCLIP* ch = AudioChans::GetChannel(play.crossfading_out_channel);
        int newVolume = ch ? ch->get_volume100() - play.crossfade_out_volume_per_step : 0;
        if (newVolume > 0)
        {
            ch->set_volume100(newVolume);
        }
        else
        {
            stop_and_destroy_channel(play.crossfading_out_channel);
            play.crossfading_out_channel = 0;
        }
    }

    if (play.crossfading_in_channel > 0 && !AudioChans::GetChannelIfPlaying(play.crossfading_in_channel))
        play.crossfading_in_channel = 0;

    if (play.crossfading_in_channel > 0)
    {
        SOUNDCLIP* ch = AudioChans::GetChannel(play.crossfading_in_channel);
        int newVolume = ch ? ch->get_volume100() + play.crossfade_in_volume_per_step : 0;
        if (newVolume > play.crossfade_final_volume_in)
        {
            newVolume = play.crossfade_final_volume_in;
        }

        ch->set_volume100(newVolume);

        if (newVolume >= play.crossfade_final_volume_in)
        {
            play.crossfading_in_channel = 0;
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    // Do audio queue
    if (play.new_music_queue_size > 0)
    {
        for (int i = 0; i < play.new_music_queue_size; i++)
        {
            ScriptAudioClip *clip = &game.audioClips[play.new_music_queue[i].audioClipIndex];
            int channel = find_free_audio_channel(clip, clip->defaultPriority, false, true);
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

    ///////////////////////////////////////////////////////////////////////////
    // Do non-blocking voice speech
    // NOTE: there's only one speech channel, therefore it's either blocking
    // or non-blocking at any given time. If it's changed, we'd need to keep
    // record of every channel, or keep a count of active channels.
    if (play.IsNonBlockingVoiceSpeech())
    {
        if (!AudioChans::ChannelIsPlaying(SCHAN_SPEECH))
        {
            stop_voice_nonblocking();
        }
    }
}

// Applies a volume drop modifier to the clip, in accordance to its audio type
static void apply_volume_drop_to_clip(SOUNDCLIP *clip)
{
    int audiotype = clip->sourceClipType;
    clip->apply_volume_modifier(-(game.audioClipTypes[audiotype].volume_reduction_while_speech_playing * 255 / 100));
}

static void queue_audio_clip_to_play(ScriptAudioClip *clip, int priority, int repeat)
{
    if (play.new_music_queue_size >= MAX_QUEUED_MUSIC) {
        debug_script_log("Too many queued music, cannot add %s", clip->scriptName.GetCStr());
        return;
    }

    SOUNDCLIP *cachedClip = load_sound_clip(clip, (repeat != 0));
    if (cachedClip != nullptr) 
    {
        play.new_music_queue[play.new_music_queue_size].audioClipIndex = clip->id;
        play.new_music_queue[play.new_music_queue_size].priority = priority;
        play.new_music_queue[play.new_music_queue_size].repeat = (repeat != 0);
        play.new_music_queue[play.new_music_queue_size].cachedClip = cachedClip;
        play.new_music_queue_size++;
    }
}

ScriptAudioChannel* play_audio_clip_on_channel(int channel, ScriptAudioClip *clip, int priority, int repeat, int fromOffset, SOUNDCLIP *soundfx)
{
    if (soundfx == nullptr)
    {
        soundfx = load_sound_clip(clip, (repeat) ? true : false);
    }
    if (soundfx == nullptr)
    {
        debug_script_log("AudioClip.Play: unable to load sound file");
        if (play.crossfading_in_channel == channel)
        {
            play.crossfading_in_channel = 0;
        }
        return nullptr;
    }
    soundfx->priority = priority;

    if (play.crossfading_in_channel == channel)
    {
        soundfx->set_volume100(0);
    }

    // Mute the audio clip if fast-forwarding the cutscene
    if (play.fast_forward) 
    {
        soundfx->set_mute(true);

        // CHECKME!!
        // [IKM] According to the 3.2.1 logic the clip will restore
        // its value after cutscene, but only if originalVolAsPercentage
        // is not zeroed. Something I am not sure about: why does it
        // disable the clip under condition that there's more than one
        // channel for this audio type? It does not even check if
        // anything of this type is currently playing.
        if (game.audioClipTypes[clip->type].reservedChannels != 1)
            soundfx->set_volume100(0);
    }

    if (soundfx->play_from(fromOffset) == 0)
    {
        delete soundfx;
        debug_script_log("AudioClip.Play: failed to play sound file");
        return nullptr;
    }

    // Apply volume drop if any speech voice-over is currently playing
    // NOTE: there is a confusing logic in sound clip classes, that they do not use
    // any modifiers when begin playing, therefore we must apply this only after
    // playback was started.
    if (!play.fast_forward && play.speech_has_voice)
        apply_volume_drop_to_clip(soundfx);

    AudioChans::SetChannel(channel, std::unique_ptr<SOUNDCLIP>(soundfx));
    return &scrAudioChannel[channel];
}

void remove_clips_of_type_from_queue(int audioType) 
{
    int aa;
    for (aa = 0; aa < play.new_music_queue_size; aa++)
    {
        ScriptAudioClip *clip = &game.audioClips[play.new_music_queue[aa].audioClipIndex];
        if ((audioType == SCR_NO_VALUE) || (clip->type == audioType))
        {
            play.new_music_queue_size--;
            for (int bb = aa; bb < play.new_music_queue_size; bb++)
                play.new_music_queue[bb] = play.new_music_queue[bb + 1];
            aa--;
        }
    }
}

void update_queued_clips_volume(int audioType, int new_vol)
{
    for (int i = 0; i < play.new_music_queue_size; ++i)
    {
        // NOTE: if clip is uncached, the volume will be set from defaults when it is loaded
        SOUNDCLIP *sndclip = play.new_music_queue[i].cachedClip;
        if (sndclip)
        {
            ScriptAudioClip *clip = &game.audioClips[play.new_music_queue[i].audioClipIndex];
            if (clip->type == audioType)
                sndclip->set_volume100(new_vol);
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

    int channel = find_free_audio_channel(clip, priority, !queueIfNoChannel, queueIfNoChannel);
    if (channel < 0)
    {
        if (queueIfNoChannel)
            queue_audio_clip_to_play(clip, priority, repeat);
        else
            debug_script_log("AudioClip.Play: no channels available to interrupt PRI:%d TYPE:%d", priority, clip->type);

        return nullptr;
    }

    return play_audio_clip_on_channel(channel, clip, priority, repeat, fromOffset);
}

ScriptAudioChannel* play_audio_clip_by_index(int audioClipIndex)
{
    if ((audioClipIndex >= 0) && ((size_t)audioClipIndex < game.audioClips.size()))
        return AudioClip_Play(&game.audioClips[audioClipIndex], SCR_NO_VALUE, SCR_NO_VALUE);
    else 
        return nullptr;
}

void stop_and_destroy_channel(int chid)
{
    if ((chid < 0) || (chid >= TOTAL_AUDIO_CHANNELS))
        quit("!StopChannel: invalid channel ID");

    AudioChans::DeleteClipOnChannel(chid);

    if (play.crossfading_in_channel == chid)
        play.crossfading_in_channel = 0;
    if (play.crossfading_out_channel == chid)
        play.crossfading_out_channel = 0;
    // don't update 'crossFading' here as it is updated in all the cross-fading functions.
}

void export_missing_audiochans()
{
    for (int i = 0; i < game.numCompatGameChannels; ++i)
    {
        int h = ccGetObjectHandleFromAddress(&scrAudioChannel[i]);
        if (h <= 0)
            ccRegisterPersistentObject(&scrAudioChannel[i], &ccDynamicAudio); // add internal ref
    }
}

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
    for (int chnum = NUM_SPEECH_CHANS; chnum < game.numGameChannels; chnum++)
    {
        auto* ch = AudioChans::GetChannelIfPlaying(chnum);
        if ((ch != nullptr) && (ch->xSource >= 0)) 
        {
            ch->apply_directional_modifier(
                get_volume_adjusted_for_distance(ch->get_volume255(), 
                    ch->xSource,
                    ch->ySource,
                    ch->maximumPossibleDistanceAway) -
                ch->get_volume255());
        }
    }
}

SOUNDCLIP *load_sound_and_play(ScriptAudioClip *aclip, bool repeat)
{
    SOUNDCLIP *soundfx = load_sound_clip(aclip, repeat);
    if (!soundfx) { return nullptr; }

    if (soundfx->play() == 0) {
        delete soundfx;
        return nullptr;
    }

    return soundfx;
}

void stop_all_sound_and_music() 
{
    stop_voice_nonblocking();
    // any ambient sound will be aborted
    for (int i = 0; i < TOTAL_AUDIO_CHANNELS; ++i)
        stop_and_destroy_channel(i);
}

void shutdown_sound() 
{
    stop_all_sound_and_music(); // game logic
    audio_core_shutdown(); // audio core system
    soundcache_clear(); // clear cached data
    sys_audio_shutdown(); // backend; NOTE: sys_main will know if it's required
    usetup.audio_enabled = false;
}


//=============================================================================
// Music update is scheduled when the voice speech stops;
// we do a small delay before reverting any volume adjustments
static bool music_update_scheduled = false;
static auto music_update_at = AGS_Clock::now();

void cancel_scheduled_music_update() {
    music_update_scheduled = false;
}

void schedule_music_update_at(AGS_Clock::time_point at) {
    music_update_scheduled = true;
    music_update_at = at;
}

void postpone_scheduled_music_update_by(std::chrono::milliseconds duration) {
    if (!music_update_scheduled) { return; }
    music_update_at += duration;
}

void process_scheduled_music_update() {
    if (!music_update_scheduled) { return; }
    if (music_update_at > AGS_Clock::now()) { return; }
    cancel_scheduled_music_update();
    apply_volume_drop_modifier(false);
}
// end scheduled music update functions
//=============================================================================

// add/remove the volume drop to the audio channels while speech is playing
void apply_volume_drop_modifier(bool applyModifier)
{
    for (int i = NUM_SPEECH_CHANS; i < game.numGameChannels; i++)
    {
        auto* ch = AudioChans::GetChannelIfPlaying(i);
        if (ch && ch->sourceClipID >= 0)
        {
            if (applyModifier)
                apply_volume_drop_to_clip(ch);
            else
                ch->apply_volume_modifier(0); // reset modifier
        }
    }
}

// Checks if speech voice-over is currently playing, and reapply volume drop to all other active clips
void update_volume_drop_if_voiceover()
{
    apply_volume_drop_modifier(play.speech_has_voice);
}

// Sync logical game channels with the audio backend:
// startup new assigned clips, apply changed parameters.
void sync_audio_playback()
{
    for (int i = 0; i < TOTAL_AUDIO_CHANNELS; ++i)
    { // update the playing channels, and dispose the finished / invalid ones
        auto *ch = AudioChans::GetChannelIfPlaying(i);
        if (ch && !ch->update())
        {
            AudioChans::DeleteClipOnChannel(i);
        }
    }
}

// Update the music, and advance the crossfade on a step
// (this should only be called once per game loop)
void update_audio_system_on_game_loop ()
{
    update_polled_stuff();

    // Sync logical game channels with the audio backend
    // NOTE: we update twice, first time here - because we need to know
    // which clips are still playing before updating the sound transitions
    // and queues, then second time later - because we need to apply any
    // changes to channels / parameters.
    // TODO: investigate options for optimizing this.
    sync_audio_playback();

    process_scheduled_music_update();

    audio_update_polled_stuff();

    if (loopcounter % 5 == 0) // TODO: investigate why we do this each 5 frames?
    {
        update_directional_sound_vol();
    }

    // Sync logical game channels with the audio backend again
    sync_audio_playback();
}
