
#include "ac/audiocliptype.h"

void AudioClipType::ReadFromFile(FILE *f)
{
    id = getw(f);
    reservedChannels = getw(f);
    volume_reduction_while_speech_playing = getw(f);
    crossfadeSpeed = getw(f);
    reservedForFuture = getw(f);
}

void AudioClipType::WriteToFile(FILE *f)
{
    putw(id, f);
    putw(reservedChannels, f);
    putw(volume_reduction_while_speech_playing, f);
    putw(crossfadeSpeed, f);
    putw(reservedForFuture, f);
}
