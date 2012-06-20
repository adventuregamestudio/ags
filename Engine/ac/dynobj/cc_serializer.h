#ifndef __AC_SERIALIZER_H
#define __AC_SERIALIZER_H

#include "ac/dynobj/cc_dynamicobject.h"

struct AGSDeSerializer : ICCObjectReader {

    virtual void Unserialize(int index, const char *objectType, const char *serializedData, int dataSize);
};

extern AGSDeSerializer ccUnserializer;

#endif // __AC_SERIALIZER_H