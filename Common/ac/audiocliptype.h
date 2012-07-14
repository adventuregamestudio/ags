#ifndef __AC_AUDIOCLIPTYPE_H
#define __AC_AUDIOCLIPTYPE_H

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
};

#endif // __AC_AUDIOCLIPTYPE_H