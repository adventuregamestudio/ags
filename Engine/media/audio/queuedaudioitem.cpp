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

#include "media/audio/queuedaudioitem.h"
#include "ac/common_defines.h"
#include "util/datastream.h"

using AGS::Common::DataStream;

// [IKM] 2012-07-02: these functions are used during load/save game,
// and read/written as-is, hence cachedClip pointer should be serialized
// simply like pointer (although that probably does not mean much sense?)
void QueuedAudioItem::ReadFromFile(DataStream *in)
{
    char padding[3] = {0,0,0};
    audioClipIndex = in->ReadInt16();
    priority = in->ReadInt16();
    repeat = in->ReadBool();
    in->Read(&padding, 3); // <-- padding
    in->ReadInt32(); // FOR BACKWARD COMPATIBILITY OF SAVEGAMES
    cachedClip = NULL;
}

void QueuedAudioItem::WriteToFile(DataStream *out)
{
    char padding[3] = {0,0,0};
    out->WriteInt16(audioClipIndex);
    out->WriteInt16(priority);
    out->WriteBool(repeat);
    out->Write(&padding, 3); // <-- padding
    out->WriteInt32(0); // FOR BACKWARD COMPATIBILITY OF SAVEGAMES
}
