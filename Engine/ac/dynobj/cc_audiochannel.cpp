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
#include "ac/dynobj/cc_audiochannel.h"
#include "ac/dynobj/scriptaudiochannel.h"
#include "ac/dynobj/dynobj_manager.h"
#include "media/audio/audio_system.h"
#include "util/stream.h"

using namespace AGS::Common;

extern ScriptAudioChannel scrAudioChannel[MAX_GAME_CHANNELS];

const char *CCAudioChannel::GetType() {
    return "AudioChannel";
}

size_t CCAudioChannel::CalcSerializeSize(const void* /*address*/)
{
    return sizeof(int32_t);
}

void CCAudioChannel::Serialize(const void *address, Stream *out) {
    const ScriptAudioChannel *ach = static_cast<const ScriptAudioChannel*>(address);
    out->WriteInt32(ach->id);
}

void CCAudioChannel::Unserialize(int index, Stream *in, size_t /*data_sz*/) {
    int id = in->ReadInt32();
    ccRegisterUnserializedObject(index, &scrAudioChannel[id], this);
}
