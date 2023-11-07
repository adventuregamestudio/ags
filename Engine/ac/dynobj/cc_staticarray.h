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
// CCStaticArray manages access to an array of script objects,
// where an element's size counted by script's bytecode may differ from the
// real element size in the engine's memory.
// The purpose of this is to remove size restriction from the engine's structs
// exposed to scripts.
// NOTE: on the other hand, similar effect could be achieved by separating
// object data into two or more structs, where "base" structs are stored in
// the exposed arrays (part of API), while extending structs are stored
// separately. This is more an issue of engine data design.
//
//=============================================================================
#ifndef __AGS_EE_DYNOBJ__CCSTATICARRAY_H
#define __AGS_EE_DYNOBJ__CCSTATICARRAY_H

#include "ac/dynobj/cc_agsdynamicobject.h"

struct CCStaticArray : public AGSCCStaticObject
{
public:
    ~CCStaticArray() override = default;

    void Create(IScriptObject *mgr, size_t elem_script_size, size_t elem_mem_size, size_t elem_count = SIZE_MAX /*unknown*/);

    inline IScriptObject *GetObjectManager() const
    {
        return _mgr;
    }

    // Legacy support for reading and writing object values by their relative offset
    inline void *GetElementPtr(void *address, intptr_t legacy_offset)
    {
        return static_cast<uint8_t*>(address) + (legacy_offset / _elemScriptSize) * _elemMemSize;
    }

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
    IScriptObject    *_mgr = nullptr;
    size_t              _elemScriptSize = 0u;
    size_t              _elemMemSize = 0u;
    size_t              _elemCount = 0u;
};

#endif // __AGS_EE_DYNOBJ__CCSTATICARRAY_H
