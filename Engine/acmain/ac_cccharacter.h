#ifndef __AC_CCCHARACTER_H
#define __AC_CCCHARACTER_H

struct CCCharacter : AGSCCDynamicObject {

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

};

#endif // __AC_CCCHARACTER_H