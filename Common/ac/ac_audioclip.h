#ifndef __AC_AUDIOCLIP_H
#define __AC_AUDIOCLIP_H

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
struct ScriptAudioClip {
    int id;  // not used by editor, set in engine only
    char scriptName[30];
    char fileName[15];
    char bundlingType;
    char type;
    char fileType;
    char defaultRepeat;
    short defaultPriority;
    short defaultVolume;
    int  reserved;
};
#define AUDIO_CLIP_TYPE_SOUND 1
struct AudioClipType {
    int id;
    int reservedChannels;
    int volume_reduction_while_speech_playing;
    int crossfadeSpeed;
    int reservedForFuture;
};

#endif // __AC_AUDIOCLIP_H