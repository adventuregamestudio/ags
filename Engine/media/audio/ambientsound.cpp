
#include "media/audio/ambientsound.h"
#include "media/audio/audiodefines.h"
#include "media/audio/soundclip.h"
#include "util/datastream.h"

using AGS::Common::CDataStream;

extern SOUNDCLIP *channels[MAX_SOUND_CHANNELS+1];

bool AmbientSound::IsPlaying () {
    if (channel <= 0)
        return false;
    return (channels[channel] != NULL) ? true : false;
}

void AmbientSound::ReadFromFile(CDataStream *in)
{
    channel = in->ReadInt32();
    x = in->ReadInt32();
    y = in->ReadInt32();
    vol = in->ReadInt32();
    num = in->ReadInt32();
    maxdist = in->ReadInt32();
}

void AmbientSound::WriteToFile(CDataStream *out)
{
    out->WriteInt32(channel);
    out->WriteInt32(x);
    out->WriteInt32(y);
    out->WriteInt32(vol);
    out->WriteInt32(num);
    out->WriteInt32(maxdist);
}
