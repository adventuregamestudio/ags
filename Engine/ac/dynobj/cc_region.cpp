
#include "ac/dynobj/cc_region.h"
#include "ac/dynobj/scriptregion.h"
#include "ac/ac_defines.h"

extern ScriptRegion scrRegion[MAX_REGIONS];

// return the type name of the object
const char *CCRegion::GetType() {
    return "Region";
}

// serialize the object into BUFFER (which is BUFSIZE bytes)
// return number of bytes used
int CCRegion::Serialize(const char *address, char *buffer, int bufsize) {
    ScriptRegion *shh = (ScriptRegion*)address;
    StartSerialize(buffer);
    SerializeInt(shh->id);
    return EndSerialize();
}

void CCRegion::Unserialize(int index, const char *serializedData, int dataSize) {
    StartUnserialize(serializedData, dataSize);
    int num = UnserializeInt();
    ccRegisterUnserializedObject(index, &scrRegion[num], this);
}
