#ifndef __AC_QUEUEDAUDIOITEM_H
#define __AC_QUEUEDAUDIOITEM_H

#include "media/audio/soundclip.h"
#include "util/file.h"

namespace AGS { namespace Common { class CDataStream; } }
using namespace AGS; // FIXME later

struct QueuedAudioItem {
    short audioClipIndex;
    short priority;
    bool  repeat;
    SOUNDCLIP *cachedClip;

    void ReadFromFile(Common::CDataStream *in);
    void WriteToFile(Common::CDataStream *out);
};

#endif // __AC_QUEUEDAUDIOITEM_H