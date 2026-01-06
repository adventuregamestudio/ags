//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================

#include "ac/dynobj/scriptaudioclip.h"
#include "util/stream.h"
#include "util/string_compat.h"

using namespace AGS::Common;

/*static*/ AudioFileType ScriptAudioClip::GetAudioFileTypeFromExt(const char *ext)
{
    if (ags_stricmp(ext, "mp3") == 0)
        return eAudioFileMP3;
    else if (ags_stricmp(ext, "wav") == 0)
        return eAudioFileWAV;
    else if (ags_stricmp(ext, "voc") == 0)
        return eAudioFileVOC;
    else if (ags_stricmp(ext, "mid") == 0)
        return eAudioFileMIDI;
    else if ((ags_stricmp(ext, "mod") == 0) || (ags_stricmp(ext, "xm") == 0)
             || (ags_stricmp(ext, "s3m") == 0) || (ags_stricmp(ext, "it") == 0))
        return eAudioFileMOD;
    else if (ags_stricmp(ext, "ogg") == 0)
        return eAudioFileOGG;
    return eAudioFileUndefined;
}

/*static */ const char *ScriptAudioClip::GetExtFromAudioFileType(AudioFileType filetype)
{
    switch (filetype)
    {
    case eAudioFileOGG:
        return "ogg";
    case eAudioFileMP3:
        return "mp3";
    case eAudioFileWAV:
        return "wav";
    case eAudioFileVOC:
        return "voc";
    case eAudioFileMIDI:
        return "mid";
    case eAudioFileMOD:
        return "mod";
    default:
        return "";
    }
}

void ScriptAudioClip::ReadFromFile(Stream *in)
{
    id = in->ReadInt32();
    scriptName.ReadCount(in, LEGACY_AUDIOCLIP_SCRIPTNAMELENGTH);
    fileName.ReadCount(in, LEGACY_AUDIOCLIP_FILENAMELENGTH);
    bundlingType = static_cast<uint8_t>(in->ReadInt8());
    type = static_cast<uint8_t>(in->ReadInt8());
    fileType = static_cast<AudioFileType>(in->ReadInt8());
    defaultRepeat = in->ReadInt8();
    in->ReadInt8(); // alignment padding to int16
    defaultPriority = in->ReadInt16();
    defaultVolume = in->ReadInt16();
    in->ReadInt16(); // alignment padding to int32
    in->ReadInt32(); // reserved
}
