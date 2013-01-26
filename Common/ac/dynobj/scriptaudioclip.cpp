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

#include "ac/dynobj/scriptaudioclip.h"
#include "util/stream.h"

using AGS::Common::Stream;

void ScriptAudioClip::ReadFromFile(Stream *in)
{
    id = in->ReadInt32();
    in->Read(scriptName, SCRIPTAUDIOCLIP_SCRIPTNAMELENGTH);
    in->Read(fileName, SCRIPTAUDIOCLIP_FILENAMELENGTH);
    bundlingType = in->ReadInt8();
    type = in->ReadInt8();
    fileType = in->ReadInt8();
    defaultRepeat = in->ReadInt8();
    defaultPriority = in->ReadInt16();
    defaultVolume = in->ReadInt16();
    reserved = in->ReadInt32();
}
