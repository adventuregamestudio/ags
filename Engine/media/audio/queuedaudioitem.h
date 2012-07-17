#ifndef __AC_QUEUEDAUDIOITEM_H
#define __AC_QUEUEDAUDIOITEM_H

#include "media/audio/soundclip.h"
#include "util/file.h"

struct QueuedAudioItem {
    short audioClipIndex;
    short priority;
    bool  repeat;
    SOUNDCLIP *cachedClip;

    void ReadFromFile(FILE *f);
    void WriteToFile(FILE *f);
};

#endif // __AC_QUEUEDAUDIOITEM_H