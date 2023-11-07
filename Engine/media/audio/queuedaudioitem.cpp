//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2023 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================

#include "media/audio/queuedaudioitem.h"
#include "ac/common_defines.h"
#include "util/stream.h"

using AGS::Common::Stream;

// [IKM] 2012-07-02: these functions are used during load/save game,
// and read/written as-is, hence cachedClip pointer should be serialized
// simply like pointer (although that probably does not mean much sense?)
void QueuedAudioItem::ReadFromFile(Stream *in)
{
    audioClipIndex = in->ReadInt16();
    priority = in->ReadInt16();
    repeat = in->ReadBool();
    in->ReadInt32(); // cachedClip
}

void QueuedAudioItem::WriteToFile(Stream *out) const
{
    out->WriteInt16(audioClipIndex);
    out->WriteInt16(priority);
    out->WriteBool(repeat);
    out->WriteInt32(0); // cachedClip
}
