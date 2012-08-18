
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_CN_DYNOBJ__SCRIPTAUDIOCLIP_H
#define __AGS_CN_DYNOBJ__SCRIPTAUDIOCLIP_H

#include "util/file.h"

#define SCRIPTAUDIOCLIP_SCRIPTNAMELENGTH    30
#define SCRIPTAUDIOCLIP_FILENAMELENGTH      15
struct ScriptAudioClip {
    int id;  // not used by editor, set in engine only
    char scriptName[SCRIPTAUDIOCLIP_SCRIPTNAMELENGTH];
    char fileName[SCRIPTAUDIOCLIP_FILENAMELENGTH];
    char bundlingType;
    char type;
    char fileType;
    char defaultRepeat;
    short defaultPriority;
    short defaultVolume;
    int  reserved;

    void ReadFromFile(FILE *iii);
};

#endif // __AGS_CN_DYNOBJ__SCRIPTAUDIOCLIP_H
