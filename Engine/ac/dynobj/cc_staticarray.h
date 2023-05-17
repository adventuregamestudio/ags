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

    void Create(ICCDynamicObject *mgr, size_t elem_script_size, size_t elem_mem_size, size_t elem_count = SIZE_MAX /*unknown*/);

    inline ICCDynamicObject *GetObjectManager() const
    {
        return _mgr;
    }

    // Legacy support for reading and writing object values by their relative offset
    virtual const char *GetElementPtr(const char *address, intptr_t legacy_offset);

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
    ICCDynamicObject    *_mgr = nullptr;
    size_t              _elemScriptSize = 0u;
    size_t              _elemMemSize = 0u;
    size_t              _elemCount = 0u;
};

#endif // __AGS_EE_DYNOBJ__CCSTATICARRAY_H
