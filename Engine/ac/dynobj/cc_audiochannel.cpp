
#include "ac/dynobj/cc_audiochannel.h"
#include "ac/audiodefines.h"
#include "ac/dynobj/scriptaudiochannel.h"

extern ScriptAudioChannel scrAudioChannel[MAX_SOUND_CHANNELS + 1];

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
