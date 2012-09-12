#ifndef __AC_AUDIOCLIPTYPE_H
#define __AC_AUDIOCLIPTYPE_H

#include "util/file.h"

// Forward declaration
namespace AGS { namespace Common { class DataStream; } }
using namespace AGS; // FIXME later

#define AUCL_BUNDLE_EXE 1
#define AUCL_BUNDLE_VOX 2
enum AudioFileType {
    eAudioFileOGG = 1,
    eAudioFileMP3 = 2,
    eAudioFileWAV = 3,
    eAudioFileVOC = 4,
    eAudioFileMIDI = 5,
    eAudioFileMOD = 6
};

#define AUDIO_CLIP_TYPE_SOUND 1
struct AudioClipType {
    int id;
    int reservedChannels;
    int volume_reduction_while_speech_playing;
    int crossfadeSpeed;
    int reservedForFuture;

    void ReadFromFile(Common::DataStream *in);
    void WriteToFile(Common::DataStream *out);
};

#endif // __AC_AUDIOCLIPTYPE_H