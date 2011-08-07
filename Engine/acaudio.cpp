/*
  Adventure Game Studio source code Copyright 1999-2011 Chris Jones.
  All rights reserved.

  The AGS Editor Source Code is provided under the Artistic License 2.0
  http://www.opensource.org/licenses/artistic-license-2.0.php

  You MAY NOT compile your own builds of the engine without making it EXPLICITLY
  CLEAR that the code has been altered from the Standard Version.

*/
#define WGT2ALLEGRO_NOFUNCTIONS
#include "wgt2allg.h"
#define CROOM_NOFUNCTIONS
#include "acroom.h"
#include "acruntim.h"

extern int psp_is_old_datafile;

extern "C" {
 extern FILE*clibfopen(char*,char*);
}

ScriptAudioChannel scrAudioChannel[MAX_SOUND_CHANNELS + 1];
CCAudioChannel ccDynamicAudio;
CCAudioClip ccDynamicAudioClip;
char acaudio_buffer[256];
int reserved_channel_count = 0;

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
    ccAddExternalSymbol(game.audioClips[ee].scriptName, &game.audioClips[ee]);
  }

  calculate_reserved_channel_count();
}

int AudioChannel_GetID(ScriptAudioChannel *channel)
{
  return channel->id;
}

int AudioChannel_GetIsPlaying(ScriptAudioChannel *channel)
{
  if (play.fast_forward)
  {
    return 0;
  }

  if ((channels[channel->id] != NULL) &&
      (channels[channel->id]->done == 0))
  {
    return 1;
  }
  return 0;
}

int AudioChannel_GetPanning(ScriptAudioChannel *channel)
{
  if ((channels[channel->id] != NULL) &&
      (channels[channel->id]->done == 0))
  {
    return channels[channel->id]->panningAsPercentage;
  }
  return 0;
}

void AudioChannel_SetPanning(ScriptAudioChannel *channel, int newPanning)
{
  if ((newPanning < -100) || (newPanning > 100))
    quitprintf("!AudioChannel.Panning: panning value must be between -100 and 100 (passed=%d)", newPanning);
  
  if ((channels[channel->id] != NULL) &&
      (channels[channel->id]->done == 0))
  {
    channels[channel->id]->set_panning(((newPanning + 100) * 255) / 200);
    channels[channel->id]->panningAsPercentage = newPanning;
  }
}

ScriptAudioClip* AudioChannel_GetPlayingClip(ScriptAudioChannel *channel)
{
  if ((channels[channel->id] != NULL) &&
      (channels[channel->id]->done == 0))
  {
    return (ScriptAudioClip*)channels[channel->id]->sourceClip;
  }
  return NULL;
}

int AudioChannel_GetPosition(ScriptAudioChannel *channel)
{
  if ((channels[channel->id] != NULL) &&
      (channels[channel->id]->done == 0))
  {
    if (play.fast_forward)
      return 999999999;

    return channels[channel->id]->get_pos();
  }
  return 0;
}

int AudioChannel_GetPositionMs(ScriptAudioChannel *channel)
{
  if ((channels[channel->id] != NULL) &&
      (channels[channel->id]->done == 0))
  {
    if (play.fast_forward)
      return 999999999;

    return channels[channel->id]->get_pos_ms();
  }
  return 0;
}

int AudioChannel_GetLengthMs(ScriptAudioChannel *channel)
{
  if ((channels[channel->id] != NULL) &&
      (channels[channel->id]->done == 0))
  {
    return channels[channel->id]->get_length_ms();
  }
  return 0;
}

int AudioChannel_GetVolume(ScriptAudioChannel *channel)
{
  if ((channels[channel->id] != NULL) &&
      (channels[channel->id]->done == 0))
  {
    return channels[channel->id]->volAsPercentage;
  }
  return 0;
}

int AudioChannel_SetVolume(ScriptAudioChannel *channel, int newVolume)
{
  if ((newVolume < 0) || (newVolume > 100))
    quitprintf("!AudioChannel.Volume: new value out of range (supplied: %d, range: 0..100)", newVolume);

  if ((channels[channel->id] != NULL) &&
      (channels[channel->id]->done == 0))
  {
    channels[channel->id]->set_volume((newVolume * 255) / 100);
    channels[channel->id]->volAsPercentage = newVolume;
  }
  return 0;
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
  play.crossfade_initial_volume_out = channels[SPECIAL_CROSSFADE_CHANNEL]->volAsPercentage;
  play.crossfade_out_volume_per_step = crossfadeSpeed;

  play.crossfading_in_channel = fadeInChannel;
  if (newSound != NULL)
  {
    start_fading_in_new_track_if_applicable(fadeInChannel, newSound);
  }
}

void stop_or_fade_out_channel(int fadeOutChannel, int fadeInChannel = -1, ScriptAudioClip *newSound = NULL)
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

void AudioChannel_Stop(ScriptAudioChannel *channel)
{
  stop_or_fade_out_channel(channel->id, -1, NULL);
}

void AudioChannel_Seek(ScriptAudioChannel *channel, int newPosition)
{
  if (newPosition < 0)
    quitprintf("!AudioChannel.Seek: invalid seek position %d", newPosition);

  if ((channels[channel->id] != NULL) &&
      (channels[channel->id]->done == 0))
  {
    channels[channel->id]->seek(newPosition);
  }
}

void AudioChannel_SetRoomLocation(ScriptAudioChannel *channel, int xPos, int yPos)
{
  if ((channels[channel->id] != NULL) &&
      (channels[channel->id]->done == 0))
  {
    int maxDist = ((xPos > thisroom.width / 2) ? xPos : (thisroom.width - xPos)) - AMBIENCE_FULL_DIST;
    channels[channel->id]->xSource = (xPos > 0) ? xPos : -1;
    channels[channel->id]->ySource = yPos;
    channels[channel->id]->maximumPossibleDistanceAway = maxDist;
    if (xPos > 0)
    {
      update_directional_sound_vol();
    }
    else
    {
      channels[channel->id]->directionalVolModifier = 0;
      channels[channel->id]->set_volume(channels[channel->id]->vol);
    }
  }
}

int AudioClip_GetFileType(ScriptAudioClip *clip)
{
  return game.audioClips[clip->id].fileType;
}

int AudioClip_GetType(ScriptAudioClip *clip)
{
  return game.audioClips[clip->id].type;
}

const char* get_audio_clip_file_name(ScriptAudioClip *clip)
{
  if (game.audioClips[clip->id].bundlingType == AUCL_BUNDLE_EXE)
  {
    strcpy(acaudio_buffer, game.audioClips[clip->id].fileName);
    FILE *iii = clibfopen(acaudio_buffer, "rb");
    if (iii != NULL)
    {
      fclose(iii);
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

int AudioClip_GetIsAvailable(ScriptAudioClip *clip)
{
  if (get_audio_clip_file_name(clip) != NULL)
    return 1;

  return 0;
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

SOUNDCLIP *load_sound_clip(ScriptAudioClip *audioClip, bool repeat)
{
  const char *clipFileName = get_audio_clip_file_name(audioClip);
  if ((clipFileName == NULL) || (usetup.digicard == DIGI_NONE))
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
    soundClip->volAsPercentage = audioClip->defaultVolume;
    soundClip->originalVolAsPercentage = soundClip->volAsPercentage;
    soundClip->set_volume((audioClip->defaultVolume * 255) / 100);
    soundClip->soundType = audioClip->type;
    soundClip->sourceClip = audioClip;
  }
  return soundClip;
}

void AudioClip_Stop(ScriptAudioClip *clip)
{
  for (int i = 0; i < MAX_SOUND_CHANNELS; i++)
  {
    if ((channels[i] != NULL) && (!channels[i]->done) && (channels[i]->sourceClip == clip))
    {
      AudioChannel_Stop(&scrAudioChannel[i]);
    }
  }
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

    int newVolume = channels[play.crossfading_out_channel]->volAsPercentage - play.crossfade_out_volume_per_step;
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
    int newVolume = channels[play.crossfading_in_channel]->volAsPercentage + play.crossfade_in_volume_per_step;
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
  update_polled_stuff(false);
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
    soundfx->set_volume(0);
    soundfx->volAsPercentage = 0;
  }

  if (play.fast_forward) 
  {
    soundfx->set_volume(0);
    soundfx->volAsPercentage = 0;

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

ScriptAudioChannel* AudioClip_Play(ScriptAudioClip *clip, int priority, int repeat)
{
  return play_audio_clip(clip, priority, repeat, 0, false);
}

ScriptAudioChannel* AudioClip_PlayFrom(ScriptAudioClip *clip, int position, int priority, int repeat)
{
  return play_audio_clip(clip, priority, repeat, position, false);
}

ScriptAudioChannel* AudioClip_PlayQueued(ScriptAudioClip *clip, int priority, int repeat)
{
  return play_audio_clip(clip, priority, repeat, 0, true);
}

void play_audio_clip_by_index(int audioClipIndex)
{
  if ((audioClipIndex >= 0) && (audioClipIndex < game.audioClipCount))
    AudioClip_Play(&game.audioClips[audioClipIndex], SCR_NO_VALUE, SCR_NO_VALUE);
}

int System_GetAudioChannelCount()
{
  return MAX_SOUND_CHANNELS;
}

ScriptAudioChannel* System_GetAudioChannels(int index)
{
  if ((index < 0) || (index >= MAX_SOUND_CHANNELS))
    quit("!System.AudioChannels: invalid sound channel index");

  return &scrAudioChannel[index];
}

void Game_StopAudio(int audioType)
{
  if (((audioType < 0) || (audioType >= game.audioClipTypeCount)) && (audioType != SCR_NO_VALUE))
    quitprintf("!Game.StopAudio: invalid audio type %d", audioType);
  int aa;

  for (aa = 0; aa < MAX_SOUND_CHANNELS; aa++)
  {
    if (audioType == SCR_NO_VALUE)
    {
      stop_or_fade_out_channel(aa);
    }
    else
    {
      ScriptAudioClip *clip = AudioChannel_GetPlayingClip(&scrAudioChannel[aa]);
      if ((clip != NULL) && (clip->type == audioType))
        stop_or_fade_out_channel(aa);
    }
  }

  remove_clips_of_type_from_queue(audioType);
}

int Game_IsAudioPlaying(int audioType)
{
  if (((audioType < 0) || (audioType >= game.audioClipTypeCount)) && (audioType != SCR_NO_VALUE))
    quitprintf("!Game.IsAudioPlaying: invalid audio type %d", audioType);

  if (play.fast_forward)
    return 0;

  for (int aa = 0; aa < MAX_SOUND_CHANNELS; aa++)
  {
    ScriptAudioClip *clip = AudioChannel_GetPlayingClip(&scrAudioChannel[aa]);
    if (clip != NULL) 
    {
      if ((clip->type == audioType) || (audioType == SCR_NO_VALUE))
      {
        return 1;
      }
    }
  }
  return 0;
}

void Game_SetAudioTypeSpeechVolumeDrop(int audioType, int volumeDrop) 
{
  if ((audioType < 0) || (audioType >= game.audioClipTypeCount))
    quit("!Game.SetAudioTypeVolume: invalid audio type");

  game.audioClipTypes[audioType].volume_reduction_while_speech_playing = volumeDrop;
}

void Game_SetAudioTypeVolume(int audioType, int volume, int changeType)
{
  if ((volume < 0) || (volume > 100))
    quitprintf("!Game.SetAudioTypeVolume: volume %d is not between 0..100", volume);
  if ((audioType < 0) || (audioType >= game.audioClipTypeCount))
    quit("!Game.SetAudioTypeVolume: invalid audio type");
  int aa;

  if ((changeType == VOL_CHANGEEXISTING) ||
      (changeType == VOL_BOTH))
  {
    for (aa = 0; aa < MAX_SOUND_CHANNELS; aa++)
    {
      ScriptAudioClip *clip = AudioChannel_GetPlayingClip(&scrAudioChannel[aa]);
      if ((clip != NULL) && (clip->type == audioType))
      {
        channels[aa]->set_volume((volume * 255) / 100);
        channels[aa]->volAsPercentage = volume;
      }
    }
  }

  if ((changeType == VOL_SETFUTUREDEFAULT) ||
      (changeType == VOL_BOTH))
  {
    play.default_audio_type_volumes[audioType] = volume;
  }

}

void register_audio_script_functions()
{
  ccAddExternalSymbol("AudioChannel::Seek^1",(void *)AudioChannel_Seek);
  ccAddExternalSymbol("AudioChannel::SetRoomLocation^2",(void *)AudioChannel_SetRoomLocation);
  ccAddExternalSymbol("AudioChannel::Stop^0",(void *)AudioChannel_Stop);
  ccAddExternalSymbol("AudioChannel::get_ID",(void *)AudioChannel_GetID);
  ccAddExternalSymbol("AudioChannel::get_IsPlaying",(void *)AudioChannel_GetIsPlaying);
  ccAddExternalSymbol("AudioChannel::get_LengthMs",(void *)AudioChannel_GetLengthMs);
  ccAddExternalSymbol("AudioChannel::get_Panning",(void *)AudioChannel_GetPanning);
  ccAddExternalSymbol("AudioChannel::set_Panning",(void *)AudioChannel_SetPanning);
  ccAddExternalSymbol("AudioChannel::get_PlayingClip",(void *)AudioChannel_GetPlayingClip);
  ccAddExternalSymbol("AudioChannel::get_Position",(void *)AudioChannel_GetPosition);
  ccAddExternalSymbol("AudioChannel::get_PositionMs",(void *)AudioChannel_GetPositionMs);
  ccAddExternalSymbol("AudioChannel::get_Volume",(void *)AudioChannel_GetVolume);
  ccAddExternalSymbol("AudioChannel::set_Volume",(void *)AudioChannel_SetVolume);

  ccAddExternalSymbol("AudioClip::Play^2",(void *)AudioClip_Play);
  ccAddExternalSymbol("AudioClip::PlayFrom^3",(void *)AudioClip_PlayFrom);
  ccAddExternalSymbol("AudioClip::PlayQueued^2",(void *)AudioClip_PlayQueued);
  ccAddExternalSymbol("AudioClip::Stop^0",(void *)AudioClip_Stop);
  ccAddExternalSymbol("AudioClip::get_FileType",(void *)AudioClip_GetFileType);
  ccAddExternalSymbol("AudioClip::get_IsAvailable",(void *)AudioClip_GetIsAvailable);
  ccAddExternalSymbol("AudioClip::get_Type",(void *)AudioClip_GetType);

  ccAddExternalSymbol("Game::IsAudioPlaying^1",(void *)Game_IsAudioPlaying);
  ccAddExternalSymbol("Game::SetAudioTypeSpeechVolumeDrop^2", (void*)Game_SetAudioTypeSpeechVolumeDrop);
  ccAddExternalSymbol("Game::SetAudioTypeVolume^3", (void*)Game_SetAudioTypeVolume);
  ccAddExternalSymbol("Game::StopAudio^1",(void *)Game_StopAudio);

  ccAddExternalSymbol("System::get_AudioChannelCount", (void*)System_GetAudioChannelCount);
  ccAddExternalSymbol("System::geti_AudioChannels", (void*)System_GetAudioChannels);
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

const char *CCAudioChannel::GetType() {
  return "AudioChannel";
}

int CCAudioChannel::Serialize(const char *address, char *buffer, int bufsize) {
  ScriptAudioChannel *ach = (ScriptAudioChannel*)address;
  StartSerialize(buffer);
  SerializeInt(ach->id);
  return EndSerialize();
}

void CCAudioChannel::Unserialize(int index, const char *serializedData, int dataSize) {
  StartUnserialize(serializedData, dataSize);
  int id = UnserializeInt();
  ccRegisterUnserializedObject(index, &scrAudioChannel[id], this);
}

const char *CCAudioClip::GetType() {
  return "AudioClip";
}

int CCAudioClip::Serialize(const char *address, char *buffer, int bufsize) {
  ScriptAudioClip *ach = (ScriptAudioClip*)address;
  StartSerialize(buffer);
  SerializeInt(ach->id);
  return EndSerialize();
}

void CCAudioClip::Unserialize(int index, const char *serializedData, int dataSize) {
  StartUnserialize(serializedData, dataSize);
  int id = UnserializeInt();
  ccRegisterUnserializedObject(index, &game.audioClips[id], this);
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

SOUNDCLIP *load_sound_clip_from_old_style_number(bool isMusic, int indexNumber, bool repeat)
{
  ScriptAudioClip* audioClip = get_audio_clip_for_old_style_number(isMusic, indexNumber);

  if (audioClip != NULL)
  {
      return load_sound_clip(audioClip, repeat);
  }

  return NULL;
}
