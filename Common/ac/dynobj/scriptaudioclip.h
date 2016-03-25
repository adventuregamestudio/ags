//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_CN_DYNOBJ__SCRIPTAUDIOCLIP_H
#define __AGS_CN_DYNOBJ__SCRIPTAUDIOCLIP_H

namespace AGS { namespace Common { class Stream; } }
using namespace AGS; // FIXME later

enum AudioFileType {
    eAudioFileOGG = 1,
    eAudioFileMP3 = 2,
    eAudioFileWAV = 3,
    eAudioFileVOC = 4,
    eAudioFileMIDI = 5,
    eAudioFileMOD = 6
};

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

    void ReadFromFile(Common::Stream *in);
};

#endif // __AGS_CN_DYNOBJ__SCRIPTAUDIOCLIP_H
