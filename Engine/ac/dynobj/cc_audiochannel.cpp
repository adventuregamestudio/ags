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

#include "game/script_objects.h"
#include "media/audio/audiodefines.h"

const char *CCAudioChannel::GetType() {
    return "AudioChannel";
}

int CCAudioChannel::Serialize(const char *address, char *buffer, int bufsize) {
    ScriptAudioChannel *ach = (ScriptAudioChannel*)address;
    StartSerialize(buffer);
    SerializeInt(ach->id);
    return EndSerialize();
}

void CCAudioChannel::Unserialize(int index, const char *serializedData, int dataSize) {
    StartUnserialize(serializedData, dataSize);
    int id = UnserializeInt();
    ccRegisterUnserializedObject(index, &scrAudioChannel[id], this);
}
