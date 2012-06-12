
#include <stdio.h>
#include <string.h>
#include "cc_dynamicarray.h"

// return the type name of the object
const char *CCDynamicArray::GetType() {
    return CC_DYNAMIC_ARRAY_TYPE_NAME;
}

int CCDynamicArray::Dispose(const char *address, bool force) {
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
int CCDynamicArray::Serialize(const char *address, char *buffer, int bufsize) {
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

void CCDynamicArray::Unserialize(int index, const char *serializedData, int dataSize) {
    char *newArray = new char[dataSize];
    memcpy(newArray, serializedData, dataSize);
    ccRegisterUnserializedObject(index, &newArray[8], this);
}

long CCDynamicArray::Create(int numElements, int elementSize, bool isManagedType)
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

CCDynamicArray globalDynamicArray;
