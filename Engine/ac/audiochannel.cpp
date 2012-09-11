
#include "util/wgt2allg.h"
#include "ac/audiochannel.h"
#include "ac/roomstruct.h"
#include "debug/debug_log.h"
#include "ac/gamestate.h"
#include "media/audio/audio.h"

extern GameState play;
extern roomstruct thisroom;

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
