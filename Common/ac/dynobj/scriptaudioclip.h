
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_CN_DYNOBJ__SCRIPTAUDIOCLIP_H
#define __AGS_CN_DYNOBJ__SCRIPTAUDIOCLIP_H

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

#endif // __AGS_CN_DYNOBJ__SCRIPTAUDIOCLIP_H
