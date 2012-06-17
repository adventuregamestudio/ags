
#include <stdio.h>
#include "ac_ccobject.h"
#include "acrun/ac_scriptobject.h"
#include "wgt2allg.h"
#include "acrun/ac_runninggame.h"

// return the type name of the object
const char *CCObject::GetType() {
    return "Object";
}

// serialize the object into BUFFER (which is BUFSIZE bytes)
// return number of bytes used
int CCObject::Serialize(const char *address, char *buffer, int bufsize) {
    ScriptObject *shh = (ScriptObject*)address;
    StartSerialize(buffer);
    SerializeInt(shh->id);
    return EndSerialize();
}

void CCObject::Unserialize(int index, const char *serializedData, int dataSize) {
    StartUnserialize(serializedData, dataSize);
    int num = UnserializeInt();
    ccRegisterUnserializedObject(index, &scrObj[num], this);
}