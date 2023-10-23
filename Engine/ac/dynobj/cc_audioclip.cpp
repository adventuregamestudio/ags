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
#include "ac/dynobj/cc_audioclip.h"
#include "ac/dynobj/scriptaudioclip.h"
#include "ac/dynobj/dynobj_manager.h"
#include "ac/gamesetupstruct.h"
#include "util/stream.h"

using namespace AGS::Common;

extern GameSetupStruct game;

const char *CCAudioClip::GetType() {
    return "AudioClip";
}

size_t CCAudioClip::CalcSerializeSize(const void* /*address*/)
{
    return sizeof(int32_t);
}

void CCAudioClip::Serialize(const void *address, Stream *out) {
    const ScriptAudioClip *ach = static_cast<const ScriptAudioClip*>(address);
    out->WriteInt32(ach->id);
}

void CCAudioClip::Unserialize(int index, Stream *in, size_t /*data_sz*/) {
    int id = in->ReadInt32();
    ccRegisterUnserializedPersistentObject(index, &game.audioClips[id], this);
}
