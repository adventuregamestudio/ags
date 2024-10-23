//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#ifndef __CC_DYNAMICARRAY_H
#define __CC_DYNAMICARRAY_H

#include <vector>
#include <unordered_map>
#include "ac/dynobj/cc_agsdynamicobject.h"
#include "scriptstring.h"


#define ARRAY_MANAGED_TYPE_FLAG    0x80000000

struct CCDynamicArray final : AGSCCDynamicObject
{
public:
    static const char *TypeName;

    struct Header
    {
        // Type id of elements, refering the RTTI
        // May contain ARRAY_MANAGED_TYPE_FLAG
        uint32_t TypeID = 0u;
        uint32_t ElemCount = 0u; // number of elements, up to INT32_MAX !
        // TODO: refactor and store "elem size" instead
        uint32_t TotalSize = 0u; // total size in bytes

        inline bool IsPointerArray() const { return (TypeID & ARRAY_MANAGED_TYPE_FLAG) != 0; }
        inline uint32_t GetElemCount() const { return ElemCount; }
    };

    CCDynamicArray() = default;
    ~CCDynamicArray() = default;

    inline static const Header &GetHeader(const void *address)
    {
        return reinterpret_cast<const Header&>(*(static_cast<const uint8_t*>(address) - MemHeaderSz));
    }

    // Create managed array object and return a pointer to the beginning of a buffer
    static DynObjectRef CreateOld(uint32_t elem_count, uint32_t elem_size, bool is_managed)
        { return CreateImpl(0u, is_managed, elem_count, elem_size); }
    static DynObjectRef CreateNew(uint32_t type_id, uint32_t elem_count, uint32_t elem_size)
        { return CreateImpl(type_id, false, elem_count, elem_size); }

    // return the type name of the object
    const char *GetType() override;
    int Dispose(void *address, bool force) override;
    void Unserialize(int index, AGS::Common::Stream *in, size_t data_sz) override;

    // Remap typeid fields using the provided map
    void RemapTypeids(void* address,
        const std::unordered_map<uint32_t, uint32_t> &typeid_map) override;
    // Traverse all managed references in this object, and run callback for each of them
    void TraverseRefs(void *address, PfnTraverseRefOp traverse_op) override;

private:
    // The size of the array's header in memory, prepended to the element data
    static const size_t MemHeaderSz = sizeof(Header);
    // The size of the serialized header
    static const size_t FileHeaderSz = sizeof(uint32_t) * 3;
    // Writeable GetHeader variant for internal purposes 
    inline static Header &GetHeaderW(void *address)
    {
        return reinterpret_cast<Header&>(*(static_cast<uint8_t*>(address) - MemHeaderSz));
    }

    static DynObjectRef CreateImpl(uint32_t type_id, bool is_managed, uint32_t elem_count, uint32_t elem_size);

    // Savegame serialization
    // Calculate and return required space for serialization, in bytes
    size_t CalcSerializeSize(const void *address) override;
    // Write object data into the provided stream
    void Serialize(const void *address, AGS::Common::Stream *out) override;
};

extern CCDynamicArray globalDynamicArray;


// Helper functions for setting up dynamic arrays.
// TODO: move to the separate source file and merge with script struct helpers?!
namespace DynamicArrayHelpers
{
    // Create array of managed strings from strings that exists somewhere
    DynObjectRef CreateStringArray(const std::vector<const char*>);
    // Create array of managed strings from script strings
    DynObjectRef CreateStringArrayFromBuffers(std::vector<ScriptString::Buffer> &&);
    // Create array from managed list of dynamic objects
    DynObjectRef CreateScriptArray(std::vector<DynObjectRef> &&items);
    // Resolves a dynamic array of managed pointers (handles) to a list of object addresses
    bool ResolvePointerArray(const void* arrobj, std::vector<void*> &objects);
    // Resolves a dynamic array of managed pointers (handles) to a list of DynObjectRef structs,
    // which define object address, manager and handle
    bool ResolvePointerArray(const void* arrobj, std::vector<DynObjectRef> &objects);
};

#endif
