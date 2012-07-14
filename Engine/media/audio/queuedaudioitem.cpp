
#include "media/audio/queuedaudioitem.h"
#include "ac/common_defines.h"

// [IKM] 2012-07-02: these functions are used during load/save game,
// and read/written as-is, hence cachedClip pointer should be serialized
// simply like pointer (although that probably does not mean much sense?)
void QueuedAudioItem::ReadFromFile(FILE *f)
{
    char padding[3];
    audioClipIndex = getshort(f);
    priority = getshort(f);
    repeat = fgetc(f);
    fread(&padding, sizeof(char), 3, f); // <-- padding
    cachedClip = (SOUNDCLIP*)getw(f);
}

void QueuedAudioItem::WriteToFile(FILE *f)
{
    char padding[3];
    putshort(audioClipIndex, f);
    putshort(priority, f);
    fputc(repeat, f);
    fwrite(&padding, sizeof(char), 3, f); // <-- padding
    putw((int32)cachedClip, f);
}
