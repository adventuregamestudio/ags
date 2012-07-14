
#include "ac/dynobj/cc_hotspot.h"
#include "ac/dynobj/scripthotspot.h"
#include "ac/common_defines.h"

extern ScriptHotspot scrHotspot[MAX_HOTSPOTS];

// return the type name of the object
const char *CCHotspot::GetType() {
    return "Hotspot";
}

// serialize the object into BUFFER (which is BUFSIZE bytes)
// return number of bytes used
int CCHotspot::Serialize(const char *address, char *buffer, int bufsize) {
    ScriptHotspot *shh = (ScriptHotspot*)address;
    StartSerialize(buffer);
    SerializeInt(shh->id);
    return EndSerialize();
}

void CCHotspot::Unserialize(int index, const char *serializedData, int dataSize) {
    StartUnserialize(serializedData, dataSize);
    int num = UnserializeInt();
    ccRegisterUnserializedObject(index, &scrHotspot[num], this);
}