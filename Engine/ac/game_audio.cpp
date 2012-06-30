
#include "wgt2allg.h"
#include "ac/game_audio.h"
#include "ac/ac_common.h"
#include "ac/audiochannel.h"
#include "ac/gamesetupstruct.h"
#include "debug/debug.h"
#include "ac/rundefines.h"
#include "ac/gamestate.h"
#include "media/audio/audio.h"

extern GameSetupStruct game;
extern GameState play;
extern ScriptAudioChannel scrAudioChannel[MAX_SOUND_CHANNELS + 1];

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

int Game_GetMODPattern() {
    if (current_music_type == MUS_MOD) {
        return channels[SCHAN_MUSIC]->get_pos();
    }
    return -1;
}

