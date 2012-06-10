#ifndef __CC_DYNAMICARRAY_H
#define __CC_DYNAMICARRAY_H

#include "cc_dynamicobject.h"   // ICCDynamicObject

#define CC_DYNAMIC_ARRAY_TYPE_NAME "CCDynamicArray"
#define ARRAY_MANAGED_TYPE_FLAG    0x80000000

struct CCDynamicArray : ICCDynamicObject
{
    // return the type name of the object
    virtual const char *GetType() {
        return CC_DYNAMIC_ARRAY_TYPE_NAME;
    }

    virtual int Dispose(const char *address, bool force) {
        address -= 8;

        // If it's an array of managed objects, release
        // their ref counts
        long *elementCount = (long*)address;
        if (elementCount[0] & ARRAY_MANAGED_TYPE_FLAG)
        {
            elementCount[0] &= ~ARRAY_MANAGED_TYPE_FLAG;
            for (int i = 0; i < elementCount[0]; i++)
            {
                if (elementCount[2 + i] != NULL)
                {
                    ccReleaseObjectReference(elementCount[2 + i]);
                }
            }
        }

        delete (void*)address;
        return 1;
    }

    // serialize the object into BUFFER (which is BUFSIZE bytes)
    // return number of bytes used
    virtual int Serialize(const char *address, char *buffer, int bufsize) {
        long *sizeInBytes = &((long*)address)[-1];
        long sizeToWrite = *sizeInBytes + 8;
        if (sizeToWrite > bufsize)
        {
            // buffer not big enough, ask for a bigger one
            return -sizeToWrite;
        }
        memcpy(buffer, address - 8, sizeToWrite);
        return sizeToWrite;
    }

    virtual void Unserialize(int index, const char *serializedData, int dataSize) {
        char *newArray = new char[dataSize];
        memcpy(newArray, serializedData, dataSize);
        ccRegisterUnserializedObject(index, &newArray[8], this);
    }

    long Create(int numElements, int elementSize, bool isManagedType)
    {
        char *newArray = new char[numElements * elementSize + 8];
        memset(newArray, 0, numElements * elementSize + 8);
        long *sizePtr = (long*)newArray;
        sizePtr[0] = numElements;
        sizePtr[1] = numElements * elementSize;
        if (isManagedType) 
            sizePtr[0] |= ARRAY_MANAGED_TYPE_FLAG;
        return ccRegisterManagedObject(&newArray[8], this);
    }

};

extern CCDynamicArray globalDynamicArray;

#endif