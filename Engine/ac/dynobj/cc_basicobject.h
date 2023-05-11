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
//
// Basic implementation of the dynamic object interface,
// intended to be used as a parent for object/manager classes that do not
// require specific implementation.
//
//=============================================================================
#ifndef __AGS_EE_DYNOBJ__BASICOBJECT_H
#define __AGS_EE_DYNOBJ__BASICOBJECT_H

#include <string.h> // memcpy
#include "ac/dynobj/cc_dynamicobject.h"


struct CCBasicObject : ICCDynamicObject
{
protected:
    virtual ~CCBasicObject() = default;

public:
    // Dispose the object
    int Dispose(const char* /*address*/, bool /*force*/) override
    {
        // cannot be removed from memory
        return 0;
    }
    // Serialize the object into BUFFER (which is BUFSIZE bytes)
    // return number of bytes used
    int Serialize(const char* /*address*/, char* /*buffer*/, int /*bufsize*/) override
    {
        return 0; // does not save data
    }
    // Remap typeid fields using the provided map
    void RemapTypeids(const char* /*address*/,
        const std::unordered_map<uint32_t, uint32_t>& /*typeid_map*/) override
    {
        /* do nothing */
    }
    // Traverse all managed references in this object, and run callback for each of them
    void TraverseRefs(const char* /*address*/, PfnTraverseRefOp /*traverse_op*/) override
    {
        /* do nothing */
    }

    //
    // Legacy support for reading and writing object fields by their relative offset
    //
    const char* GetFieldPtr(const char* address, intptr_t offset) override
    {
        return address + offset;
    }

    void Read(const char* address, intptr_t offset, void* dest, int size) override
    {
        memcpy(dest, address + offset, size);
    }

    uint8_t ReadInt8(const char* address, intptr_t offset) override
    {
        return *(uint8_t*)(address + offset);
    }

    int16_t ReadInt16(const char* address, intptr_t offset) override
    {
        return *(int16_t*)(address + offset);
    }

    int32_t ReadInt32(const char* address, intptr_t offset) override
    {
        return *(int32_t*)(address + offset);
    }

    float ReadFloat(const char* address, intptr_t offset) override
    {
        return *(float*)(address + offset);
    }

    void Write(const char* address, intptr_t offset, void* src, int size) override
    {
        memcpy((void*)(address + offset), src, size);
    }

    void WriteInt8(const char* address, intptr_t offset, uint8_t val) override
    {
        *(uint8_t*)(address + offset) = val;
    }

    void WriteInt16(const char* address, intptr_t offset, int16_t val) override
    {
        *(int16_t*)(address + offset) = val;
    }

    void WriteInt32(const char* address, intptr_t offset, int32_t val) override
    {
        *(int32_t*)(address + offset) = val;
    }

    void WriteFloat(const char* address, intptr_t offset, float val) override
    {
        *(float*)(address + offset) = val;
    }
};

#endif // __AGS_EE_DYNOBJ__BASICOBJECT_H