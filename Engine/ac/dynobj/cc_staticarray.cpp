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
#include "ac/dynobj/cc_staticarray.h"
#include "ac/dynobj/cc_dynamicobject.h"

void CCStaticArray::Create(ICCDynamicObject *mgr, size_t elem_script_size, size_t elem_mem_size, size_t elem_count)
{
    _mgr            = mgr;
    _elemScriptSize = elem_script_size;
    _elemMemSize    = elem_mem_size;
    _elemCount      = elem_count;
}

const char *CCStaticArray::GetElementPtr(const char *address, intptr_t legacy_offset)
{
    return address + (legacy_offset / _elemScriptSize) * _elemMemSize;
}

const char* CCStaticArray::GetFieldPtr(const char *address, intptr_t offset)
{
    return GetElementPtr(address, offset);
}

void CCStaticArray::Read(const char *address, intptr_t offset, void *dest, int size)
{
    const char *el_ptr = GetElementPtr(address, offset);
    return _mgr->Read(el_ptr, offset % _elemScriptSize, dest, size);
}

uint8_t CCStaticArray::ReadInt8(const char *address, intptr_t offset)
{
    const char *el_ptr = GetElementPtr(address, offset);
    return _mgr->ReadInt8(el_ptr, offset % _elemScriptSize);
}

int16_t CCStaticArray::ReadInt16(const char *address, intptr_t offset)
{
    const char *el_ptr = GetElementPtr(address, offset);
    return _mgr->ReadInt16(el_ptr, offset % _elemScriptSize);
}

int32_t CCStaticArray::ReadInt32(const char *address, intptr_t offset)
{
    const char *el_ptr = GetElementPtr(address, offset);
    return _mgr->ReadInt32(el_ptr, offset % _elemScriptSize);
}

float CCStaticArray::ReadFloat(const char *address, intptr_t offset)
{
    const char *el_ptr = GetElementPtr(address, offset);
    return _mgr->ReadFloat(el_ptr, offset % _elemScriptSize);
}

void CCStaticArray::Write(const char *address, intptr_t offset, void *src, int size)
{
    const char *el_ptr = GetElementPtr(address, offset);
    return _mgr->Write(el_ptr, offset % _elemScriptSize, src, size);
}

void CCStaticArray::WriteInt8(const char *address, intptr_t offset, uint8_t val)
{
    const char *el_ptr = GetElementPtr(address, offset);
    return _mgr->WriteInt8(el_ptr, offset % _elemScriptSize, val);
}

void CCStaticArray::WriteInt16(const char *address, intptr_t offset, int16_t val)
{
    const char *el_ptr = GetElementPtr(address, offset);
    return _mgr->WriteInt16(el_ptr, offset % _elemScriptSize, val);
}

void CCStaticArray::WriteInt32(const char *address, intptr_t offset, int32_t val)
{
    const char *el_ptr = GetElementPtr(address, offset);
    return _mgr->WriteInt32(el_ptr, offset % _elemScriptSize, val);
}

void CCStaticArray::WriteFloat(const char *address, intptr_t offset, float val)
{
    const char *el_ptr = GetElementPtr(address, offset);
    return _mgr->WriteFloat(el_ptr, offset % _elemScriptSize, val);
}