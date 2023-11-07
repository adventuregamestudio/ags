//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2023 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// ScriptUserObject is a dynamic (managed) struct manager.
//
//=============================================================================
#ifndef __AGS_EE_DYNOBJ__SCRIPTUSERSTRUCT_H
#define __AGS_EE_DYNOBJ__SCRIPTUSERSTRUCT_H

#include "ac/dynobj/cc_agsdynamicobject.h"
#include "util/stream.h"


struct ScriptUserObject final : AGSCCDynamicObject
{
public:
    static const char *TypeName;

    struct Header
    {
        // Type id of the struct, refering the RTTI
        uint32_t TypeId = 0u;
        uint32_t Size = 0u;
        // NOTE: we use signed int for Size at the moment, because the managed
        // object interface's Serialize() function requires the object to return
        // negative value of size in case the provided buffer was not large
        // enough. Since this interface is also a part of Plugin API, we would
        // need more significant change to program before we could use different
        // approach.
    };

    ScriptUserObject() = default;
    ~ScriptUserObject() = default;

    inline static const Header &GetHeader(const void *address)
    {
        return reinterpret_cast<const Header&>(*(static_cast<const uint8_t*>(address) - MemHeaderSz));
    }

    // Create managed struct object and return a pointer to the beginning of a buffer
    static DynObjectRef Create(uint32_t type_id, size_t size);

    // return the type name of the object
    const char *GetType() override;
    int Dispose(void *address, bool force) override;
    void Unserialize(int index, AGS::Common::Stream *in, size_t data_sz) override;

    // Remap typeid fields using the provided map
    void RemapTypeids(void *address,
        const std::unordered_map<uint32_t, uint32_t> &typeid_map) override;
    // Traverse all managed references in this object, and run callback for each of them
    void TraverseRefs(void *address, PfnTraverseRefOp traverse_op) override;

private:
    // The size of the array's header in memory, prepended to the element data
    static const size_t MemHeaderSz = sizeof(Header);
    // The size of the serialized header
    static const size_t FileHeaderSz = sizeof(uint32_t) * 2; // hdr size + typeid
    // Writeable GetHeader variant for internal purposes 
    inline static Header &GetHeaderW(void *address)
    {
        return reinterpret_cast<Header&>(*(static_cast<uint8_t*>(address) - MemHeaderSz));
    }

    // Savegame serialization
    // Calculate and return required space for serialization, in bytes
    size_t CalcSerializeSize(const void *address) override;
    // Write object data into the provided stream
    void Serialize(const void *address, AGS::Common::Stream *out) override;
};

extern ScriptUserObject globalDynamicStruct;


// Helper functions for setting up custom managed structs based on ScriptUserObject.
namespace ScriptStructHelpers
{
    // Creates a managed Point object, represented as a pair of X and Y coordinates.
    ScriptUserObject *CreatePoint(int x, int y);
};

#endif // __AGS_EE_DYNOBJ__SCRIPTUSERSTRUCT_H
