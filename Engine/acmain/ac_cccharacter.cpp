
#include "ac_cccharacter.h"

// return the type name of the object
virtual const char *GetType() {
    return "Character";
}

// serialize the object into BUFFER (which is BUFSIZE bytes)
// return number of bytes used
virtual int Serialize(const char *address, char *buffer, int bufsize) {
    CharacterInfo *chaa = (CharacterInfo*)address;
    StartSerialize(buffer);
    SerializeInt(chaa->index_id);
    return EndSerialize();
}

virtual void Unserialize(int index, const char *serializedData, int dataSize) {
    StartUnserialize(serializedData, dataSize);
    int num = UnserializeInt();
    ccRegisterUnserializedObject(index, &game.chars[num], this);
}