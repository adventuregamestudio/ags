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

#include <vector>
#include <unordered_map>
#include "ac/dynobj/cc_dynamicobject.h"   // ICCDynamicObject

#define ARRAY_MANAGED_TYPE_FLAG    0x80000000

struct CCDynamicArray final : ICCDynamicObject
{
public:
    static const char *TypeName;

    struct Header
    {
        // Type id of elements, refering the RTTI
        // May contain ARRAY_MANAGED_TYPE_FLAG
        uint32_t TypeID = 0u;
        uint32_t ElemCount = 0u;
        // TODO: refactor and store "elem size" instead
        uint32_t TotalSize = 0u;
    };

    inline static const Header &GetHeader(const char *address)
    {
        return reinterpret_cast<const Header&>(*(address - MemHeaderSz));
    }

    // return the type name of the object
    const char *GetType() override;
    int Dispose(const char *address, bool force) override;
    // serialize the object into BUFFER (which is BUFSIZE bytes)
    // return number of bytes used
    int Serialize(const char *address, char *buffer, int bufsize) override;
    void Unserialize(int index, AGS::Common::Stream *in, size_t data_sz);
    // Create managed array object and return a pointer to the beginning of a buffer
    DynObjectRef CreateOld(uint32_t elem_count, uint32_t elem_size, bool isManagedType)
        { return CreateImpl(0u, isManagedType, elem_count, elem_size); }
    DynObjectRef CreateNew(uint32_t type_id, uint32_t elem_count, uint32_t elem_size)
        { return CreateImpl(type_id, false, elem_count, elem_size); }

    // Remap typeid fields using the provided map
    void RemapTypeids(const char* address,
        const std::unordered_map<uint32_t, uint32_t> &typeid_map) override;

    // Legacy support for reading and writing object values by their relative offset
    const char* GetFieldPtr(const char *address, intptr_t offset) override;
    void    Read(const char *address, intptr_t offset, void *dest, int size) override;
    uint8_t ReadInt8(const char *address, intptr_t offset) override;
    int16_t ReadInt16(const char *address, intptr_t offset) override;
    int32_t ReadInt32(const char *address, intptr_t offset) override;
    float   ReadFloat(const char *address, intptr_t offset) override;
    void    Write(const char *address, intptr_t offset, void *src, int size) override;
    void    WriteInt8(const char *address, intptr_t offset, uint8_t val) override;
    void    WriteInt16(const char *address, intptr_t offset, int16_t val) override;
    void    WriteInt32(const char *address, intptr_t offset, int32_t val) override;
    void    WriteFloat(const char *address, intptr_t offset, float val) override;

private:
    // The size of the array's header in memory, prepended to the element data
    static const size_t MemHeaderSz = sizeof(uint32_t) * 3;
    // The size of the serialized header
    static const size_t FileHeaderSz = sizeof(uint32_t) * 3;

    DynObjectRef CreateImpl(uint32_t type_id, bool is_managed, uint32_t elem_count, uint32_t elem_size);
};

extern CCDynamicArray globalDynamicArray;

// Helper functions for setting up dynamic arrays.
namespace DynamicArrayHelpers
{
    // Create array of managed strings
    DynObjectRef CreateStringArray(const std::vector<const char*>);
};

#endif