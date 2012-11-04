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

    // Legacy support for reading and writing object values by their relative offset
    virtual void    Read(const char *address, int offset, void *dest, int size);
    virtual uint8_t ReadInt8(const char *address, long offset);
    virtual int16_t ReadInt16(const char *address, long offset);
    virtual int32_t ReadInt32(const char *address, long offset);
    virtual float   ReadFloat(const char *address, long offset);
    virtual void    Write(const char *address, int offset, void *src, int size);
    virtual void    WriteInt8(const char *address, long offset, uint8_t val);
    virtual void    WriteInt16(const char *address, long offset, int16_t val);
    virtual void    WriteInt32(const char *address, long offset, int32_t val);
    virtual void    WriteFloat(const char *address, long offset, float val);
};

extern CCDynamicArray globalDynamicArray;

#endif