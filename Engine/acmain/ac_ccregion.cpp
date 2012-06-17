
#include "ac_ccregion.h"

// return the type name of the object
virtual const char *GetType() {
    return "Region";
}

// serialize the object into BUFFER (which is BUFSIZE bytes)
// return number of bytes used
virtual int Serialize(const char *address, char *buffer, int bufsize) {
    ScriptRegion *shh = (ScriptRegion*)address;
    StartSerialize(buffer);
    SerializeInt(shh->id);
    return EndSerialize();
}

virtual void Unserialize(int index, const char *serializedData, int dataSize) {
    StartUnserialize(serializedData, dataSize);
    int num = UnserializeInt();
    ccRegisterUnserializedObject(index, &scrRegion[num], this);
}