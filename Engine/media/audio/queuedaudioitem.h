#ifndef __AC_QUEUEDAUDIOITEM_H
#define __AC_QUEUEDAUDIOITEM_H

#include "media/audio/soundclip.h"

struct QueuedAudioItem {
    short audioClipIndex;
    short priority;
    bool  repeat;
    SOUNDCLIP *cachedClip;
};

#endif // __AC_QUEUEDAUDIOITEM_H