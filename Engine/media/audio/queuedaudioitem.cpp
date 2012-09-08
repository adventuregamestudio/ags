
#include "media/audio/queuedaudioitem.h"
#include "ac/common_defines.h"
#include "util/datastream.h"

using AGS::Common::CDataStream;

// [IKM] 2012-07-02: these functions are used during load/save game,
// and read/written as-is, hence cachedClip pointer should be serialized
// simply like pointer (although that probably does not mean much sense?)
void QueuedAudioItem::ReadFromFile(CDataStream *in)
{
    char padding[3] = {0,0,0};
    audioClipIndex = in->ReadInt16();
    priority = in->ReadInt16();
    repeat = in->ReadBool();
    in->ReadArray(&padding, sizeof(char), 3); // <-- padding
    cachedClip = (SOUNDCLIP*)in->ReadInt32();
}

void QueuedAudioItem::WriteToFile(CDataStream *out)
{
    char padding[3] = {0,0,0};
    out->WriteInt16(audioClipIndex);
    out->WriteInt16(priority);
    out->WriteBool(repeat);
    out->WriteArray(&padding, sizeof(char), 3); // <-- padding
    out->WriteInt32((int32)cachedClip);
}
