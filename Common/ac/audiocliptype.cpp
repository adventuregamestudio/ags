
#include "ac/audiocliptype.h"
#include "util/datastream.h"

using AGS::Common::CDataStream;

void AudioClipType::ReadFromFile(CDataStream *in)
{
    id = in->ReadInt32();
    reservedChannels = in->ReadInt32();
    volume_reduction_while_speech_playing = in->ReadInt32();
    crossfadeSpeed = in->ReadInt32();
    reservedForFuture = in->ReadInt32();
}

void AudioClipType::WriteToFile(CDataStream *out)
{
    out->WriteInt32(id);
    out->WriteInt32(reservedChannels);
    out->WriteInt32(volume_reduction_while_speech_playing);
    out->WriteInt32(crossfadeSpeed);
    out->WriteInt32(reservedForFuture);
}
