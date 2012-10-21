//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================

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
    int *elementCount = (int*)address;
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

    delete address;
    return 1;
}

// serialize the object into BUFFER (which is BUFSIZE bytes)
// return number of bytes used
int CCDynamicArray::Serialize(const char *address, char *buffer, int bufsize) {
    int *sizeInBytes = &((int*)address)[-1];
    int sizeToWrite = *sizeInBytes + 8;
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
    int *sizePtr = (int*)newArray;
    sizePtr[0] = numElements;
    sizePtr[1] = numElements * elementSize;
    if (isManagedType) 
        sizePtr[0] |= ARRAY_MANAGED_TYPE_FLAG;
    return ccRegisterManagedObject(&newArray[8], this);
}

CCDynamicArray globalDynamicArray;
