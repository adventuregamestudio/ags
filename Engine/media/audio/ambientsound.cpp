
#include "media/audio/ambientsound.h"
#include "media/audio/audiodefines.h"
#include "media/audio/soundclip.h"

extern SOUNDCLIP *channels[MAX_SOUND_CHANNELS+1];

bool AmbientSound::IsPlaying () {
    if (channel <= 0)
        return false;
    return (channels[channel] != NULL) ? true : false;
}
