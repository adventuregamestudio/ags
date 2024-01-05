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
#include "ac/dynobj/cc_staticarray.h"
#include "ac/dynobj/cc_scriptobject.h"

void CCStaticArray::Create(IScriptObject *mgr, size_t elem_script_size, size_t elem_mem_size, size_t elem_count)
{
    _mgr            = mgr;
    _elemScriptSize = elem_script_size;
    _elemMemSize    = elem_mem_size;
    _elemCount      = elem_count;
}

void* CCStaticArray::GetFieldPtr(void *address, intptr_t offset)
{
    return GetElementPtr(address, offset);
}

void CCStaticArray::Read(void *address, intptr_t offset, uint8_t *dest, size_t size)
{
    void *el_ptr = GetElementPtr(address, offset);
    return _mgr->Read(el_ptr, offset % _elemScriptSize, dest, size);
}

uint8_t CCStaticArray::ReadInt8(void *address, intptr_t offset)
{
    void *el_ptr = GetElementPtr(address, offset);
    return _mgr->ReadInt8(el_ptr, offset % _elemScriptSize);
}

int16_t CCStaticArray::ReadInt16(void *address, intptr_t offset)
{
    void *el_ptr = GetElementPtr(address, offset);
    return _mgr->ReadInt16(el_ptr, offset % _elemScriptSize);
}

int32_t CCStaticArray::ReadInt32(void *address, intptr_t offset)
{
    void *el_ptr = GetElementPtr(address, offset);
    return _mgr->ReadInt32(el_ptr, offset % _elemScriptSize);
}

float CCStaticArray::ReadFloat(void *address, intptr_t offset)
{
    void *el_ptr = GetElementPtr(address, offset);
    return _mgr->ReadFloat(el_ptr, offset % _elemScriptSize);
}

void CCStaticArray::Write(void *address, intptr_t offset, const uint8_t *src, size_t size)
{
    void *el_ptr = GetElementPtr(address, offset);
    return _mgr->Write(el_ptr, offset % _elemScriptSize, src, size);
}

void CCStaticArray::WriteInt8(void *address, intptr_t offset, uint8_t val)
{
    void *el_ptr = GetElementPtr(address, offset);
    return _mgr->WriteInt8(el_ptr, offset % _elemScriptSize, val);
}

void CCStaticArray::WriteInt16(void *address, intptr_t offset, int16_t val)
{
    void *el_ptr = GetElementPtr(address, offset);
    return _mgr->WriteInt16(el_ptr, offset % _elemScriptSize, val);
}

void CCStaticArray::WriteInt32(void *address, intptr_t offset, int32_t val)
{
    void *el_ptr = GetElementPtr(address, offset);
    return _mgr->WriteInt32(el_ptr, offset % _elemScriptSize, val);
}

void CCStaticArray::WriteFloat(void *address, intptr_t offset, float val)
{
    void *el_ptr = GetElementPtr(address, offset);
    return _mgr->WriteFloat(el_ptr, offset % _elemScriptSize, val);
}
