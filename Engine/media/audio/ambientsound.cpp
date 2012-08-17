
#include "media/audio/ambientsound.h"
#include "media/audio/audiodefines.h"
#include "media/audio/soundclip.h"

extern SOUNDCLIP *channels[MAX_SOUND_CHANNELS+1];

bool AmbientSound::IsPlaying () {
    if (channel <= 0)
        return false;
    return (channels[channel] != NULL) ? true : false;
}

void AmbientSound::ReadFromFile(FILE *f)
{
    channel = getw(f);
    x = getw(f);
    y = getw(f);
    vol = getw(f);
    num = getw(f);
    maxdist = getw(f);
}

void AmbientSound::WriteToFile(FILE *f)
{
    putw(channel, f);
    putw(x, f);
    putw(y, f);
    putw(vol, f);
    putw(num, f);
    putw(maxdist, f);
}
