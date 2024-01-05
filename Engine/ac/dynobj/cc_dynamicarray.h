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
#include "ac/dynobj/cc_agsdynamicobject.h"
#include "util/stream.h"


#define ARRAY_MANAGED_TYPE_FLAG    0x80000000

struct CCDynamicArray final : AGSCCDynamicObject
{
public:
    static const char *TypeName;

    struct Header
    {
        // May contain ARRAY_MANAGED_TYPE_FLAG
        uint32_t ElemCount = 0u;
        // TODO: refactor and store "elem size" instead
        uint32_t TotalSize = 0u;
    };

    CCDynamicArray() = default;
    ~CCDynamicArray() = default;

    inline static const Header &GetHeader(const void *address)
    {
        return reinterpret_cast<const Header&>(*(static_cast<const uint8_t*>(address) - MemHeaderSz));
    }

    // Create managed array object and return a pointer to the beginning of a buffer
    static DynObjectRef Create(int numElements, int elementSize, bool isManagedType);

    // return the type name of the object
    const char *GetType() override;
    int Dispose(void *address, bool force) override;
    void Unserialize(int index, AGS::Common::Stream *in, size_t data_sz) override;

private:
    // The size of the array's header in memory, prepended to the element data
    static const size_t MemHeaderSz = sizeof(Header);
    // The size of the serialized header
    static const size_t FileHeaderSz = sizeof(uint32_t) * 2;

    // Savegame serialization
    // Calculate and return required space for serialization, in bytes
    size_t CalcSerializeSize(const void *address) override;
    // Write object data into the provided stream
    void Serialize(const void *address, AGS::Common::Stream *out) override;
};

extern CCDynamicArray globalDynamicArray;


// Helper functions for setting up dynamic arrays.
namespace DynamicArrayHelpers
{
    // Create array of managed strings
    DynObjectRef CreateStringArray(const std::vector<const char*>);
};

#endif