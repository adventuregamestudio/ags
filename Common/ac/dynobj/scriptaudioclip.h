//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_CN_DYNOBJ__SCRIPTAUDIOCLIP_H
#define __AGS_CN_DYNOBJ__SCRIPTAUDIOCLIP_H

#include "util/string.h"

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

// Bundling type tells where to look for the audio asset
enum AudioClipBundlingType
{
    kAudioBundle_Undefined = 0,
    kAudioBundle_GamePak = 1,
    kAudioBundle_AudioVox = 2,
    kAudioBundle_SpeechVox = 3, // for voice-overs
};

#define LEGACY_AUDIOCLIP_SCRIPTNAMELENGTH    30
#define LEGACY_AUDIOCLIP_FILENAMELENGTH      15

struct ScriptAudioClip
{
    int id = -1;
    Common::String scriptName;
    Common::String fileName;
    AudioClipBundlingType bundlingType = kAudioBundle_GamePak;
    uint8_t type = 0;
    AudioFileType fileType = eAudioFileOGG;
    char defaultRepeat = 0;
    short defaultPriority = 50;
    short defaultVolume = 100;

    ScriptAudioClip() = default;
    ScriptAudioClip(uint8_t type_, const Common::String &script_name, const Common::String &file_name, AudioClipBundlingType bundle_type)
        : type(type_), scriptName(script_name), fileName(file_name), bundlingType(bundle_type)
    {}

    void ReadFromFile(Common::Stream *in);
};

#endif // __AGS_CN_DYNOBJ__SCRIPTAUDIOCLIP_H
