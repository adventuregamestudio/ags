#ifndef __AC_CCCHARACTER_H
#define __AC_CCCHARACTER_H

struct CCCharacter : AGSCCDynamicObject {

    // return the type name of the object
    virtual const char *GetType();

    // serialize the object into BUFFER (which is BUFSIZE bytes)
    // return number of bytes used
    virtual int Serialize(const char *address, char *buffer, int bufsize);

    virtual void Unserialize(int index, const char *serializedData, int dataSize);

};

#endif // __AC_CCCHARACTER_H