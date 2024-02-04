//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
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

using namespace AGS::Common;

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
