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
// Managed object, which size and contents are defined by user script
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

    ScriptUserObject() = default;
    
protected:
    virtual ~ScriptUserObject();

public:
    static ScriptUserObject *CreateManaged(uint32_t type_id, size_t size);
    void Create(const uint8_t *data, AGS::Common::Stream *in, uint32_t type_id, size_t size);

    // return the type name of the object
    const char *GetType() override;
    int Dispose(void *address, bool force) override;
    void Unserialize(int index, AGS::Common::Stream *in, size_t data_sz) override;

    // Remap typeid fields using the provided map
    void RemapTypeids(void *address,
        const std::unordered_map<uint32_t, uint32_t> &typeid_map) override;
    // Traverse all managed references in this object, and run callback for each of them
    void TraverseRefs(void *address, PfnTraverseRefOp traverse_op) override;

    // Support for reading and writing object values by their relative offset
    void   *GetFieldPtr(void *address, intptr_t offset) override;
    void    Read(void *address, intptr_t offset, uint8_t *dest, size_t size) override;
    uint8_t ReadInt8(void *address, intptr_t offset) override;
    int16_t ReadInt16(void *address, intptr_t offset) override;
    int32_t ReadInt32(void *address, intptr_t offset) override;
    float   ReadFloat(void *address, intptr_t offset) override;
    void    Write(void *address, intptr_t offset, const uint8_t *src, size_t size) override;
    void    WriteInt8(void *address, intptr_t offset, uint8_t val) override;
    void    WriteInt16(void *address, intptr_t offset, int16_t val) override;
    void    WriteInt32(void *address, intptr_t offset, int32_t val) override;
    void    WriteFloat(void *address, intptr_t offset, float val) override;

private:
    uint32_t _typeid = 0u; // type id from rtti
    // NOTE: the managed object interface's Serialize() function requires
    // the object to return negative value of size in case the provided
    // buffer was not large enough. Since this interface is also a part of
    // Plugin API, we cannot modify that without more significant changes.
    // This means that if the size is larger than INT32_MAX, Serialize()
    // will work unreliably.
    size_t   _size = 0u;
    uint8_t *_data = nullptr;
    // Savegame serialization
    // Calculate and return required space for serialization, in bytes
    size_t CalcSerializeSize(void *address) override;
    // Write object data into the provided stream
    void Serialize(void *address, AGS::Common::Stream *out) override;
};


// Helper functions for setting up custom managed structs based on ScriptUserObject.
namespace ScriptStructHelpers
{
    // Creates a managed Point object, represented as a pair of X and Y coordinates.
    ScriptUserObject *CreatePoint(int x, int y);
};

#endif // __AGS_EE_DYNOBJ__SCRIPTUSERSTRUCT_H
