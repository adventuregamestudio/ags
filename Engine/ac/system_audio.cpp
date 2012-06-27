
#include "ac/ac_common.h"
#include "ac/system_audio.h"
#include "wgt2allg.h"
#include "ac/audiodefines.h"
#include "ac/gamestate.h"

extern GameState play;
extern SOUNDCLIP *channels[MAX_SOUND_CHANNELS+1];
extern ScriptAudioChannel scrAudioChannel[MAX_SOUND_CHANNELS + 1];

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

int System_GetVolume() 
{
    return play.digital_master_volume;
}

void System_SetVolume(int newvol) 
{
    if ((newvol < 0) || (newvol > 100))
        quit("!System.Volume: invalid volume - must be from 0-100");

    if (newvol == play.digital_master_volume)
        return;

    play.digital_master_volume = newvol;
    set_volume((newvol * 255) / 100, (newvol * 255) / 100);

    // allegro's set_volume can lose the volumes of all the channels
    // if it was previously set low; so restore them
    for (int i = 0; i <= MAX_SOUND_CHANNELS; i++) 
    {
        if ((channels[i] != NULL) && (channels[i]->done == 0)) 
        {
            channels[i]->set_volume(channels[i]->vol);
        }
    }
}
