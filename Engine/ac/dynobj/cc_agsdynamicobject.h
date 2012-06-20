#ifndef __AC_CCDYNAMICOBJECT_H
#define __AC_CCDYNAMICOBJECT_H

#include "ac/dynobj/cc_dynamicobject.h"

struct AGSCCDynamicObject : ICCDynamicObject {
public:
    // default implementation
    virtual int Dispose(const char *address, bool force);

    virtual void Unserialize(int index, const char *serializedData, int dataSize) = 0;

protected:
    int bytesSoFar;
    int totalBytes;
    char *serbuffer;

    void StartSerialize(char *sbuffer);
    void SerializeInt(int val);
    int  EndSerialize();
    void StartUnserialize(const char *sbuffer, int pTotalBytes);
    int  UnserializeInt();

};

#endif // __AC_CCDYNAMICOBJECT_H