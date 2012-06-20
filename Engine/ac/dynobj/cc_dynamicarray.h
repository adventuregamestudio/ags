#ifndef __CC_DYNAMICARRAY_H
#define __CC_DYNAMICARRAY_H

#include "ac/dynobj/cc_dynamicobject.h"   // ICCDynamicObject

#define CC_DYNAMIC_ARRAY_TYPE_NAME "CCDynamicArray"
#define ARRAY_MANAGED_TYPE_FLAG    0x80000000

struct CCDynamicArray : ICCDynamicObject
{
    // return the type name of the object
    virtual const char *GetType();
    virtual int Dispose(const char *address, bool force);
    // serialize the object into BUFFER (which is BUFSIZE bytes)
    // return number of bytes used
    virtual int Serialize(const char *address, char *buffer, int bufsize);
    virtual void Unserialize(int index, const char *serializedData, int dataSize);
    long Create(int numElements, int elementSize, bool isManagedType);

};

extern CCDynamicArray globalDynamicArray;

#endif