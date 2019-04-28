
#include <string.h>
#include "ac/statobj/staticarray.h"
#include "ac/dynobj/cc_dynamicobject.h"

void StaticArray::Create(int elem_script_size, int elem_real_size, int elem_count)
{
    _staticMgr      = nullptr;
    _dynamicMgr     = nullptr;
    _elemScriptSize = elem_script_size;
    _elemRealSize   = elem_real_size;
    _elemCount      = elem_count;
}

void StaticArray::Create(ICCStaticObject *stcmgr, int elem_script_size, int elem_real_size, int elem_count)
{
    _staticMgr      = stcmgr;
    _dynamicMgr     = nullptr;
    _elemScriptSize = elem_script_size;
    _elemRealSize   = elem_real_size;
    _elemCount      = elem_count;
}

void StaticArray::Create(ICCDynamicObject *dynmgr, int elem_script_size, int elem_real_size, int elem_count)
{
    _staticMgr      = nullptr;
    _dynamicMgr     = dynmgr;
    _elemScriptSize = elem_script_size;
    _elemRealSize   = elem_real_size;
    _elemCount      = elem_count;
}

const char *StaticArray::GetElementPtr(const char *address, intptr_t legacy_offset)
{
    return address + (legacy_offset / _elemScriptSize) * _elemRealSize;
}

const char* StaticArray::GetFieldPtr(const char *address, intptr_t offset)
{
    return GetElementPtr(address, offset);
}

void StaticArray::Read(const char *address, intptr_t offset, void *dest, int size)
{
    const char *el_ptr = GetElementPtr(address, offset);
    if (_staticMgr)
    {
        return _staticMgr->Read(el_ptr, offset % _elemScriptSize, dest, size);
    }
    else if (_dynamicMgr)
    {
        return _dynamicMgr->Read(el_ptr, offset % _elemScriptSize, dest, size);
    }
    memcpy(dest, el_ptr + offset % _elemScriptSize, size);
}

uint8_t StaticArray::ReadInt8(const char *address, intptr_t offset)
{
    const char *el_ptr = GetElementPtr(address, offset);
    if (_staticMgr)
    {
        return _staticMgr->ReadInt8(el_ptr, offset % _elemScriptSize);
    }
    else if (_dynamicMgr)
    {
        return _dynamicMgr->ReadInt8(el_ptr, offset % _elemScriptSize);
    }
    return *(uint8_t*)(el_ptr + offset % _elemScriptSize);
}

int16_t StaticArray::ReadInt16(const char *address, intptr_t offset)
{
    const char *el_ptr = GetElementPtr(address, offset);
    if (_staticMgr)
    {
        return _staticMgr->ReadInt16(el_ptr, offset % _elemScriptSize);
    }
    else if (_dynamicMgr)
    {
        return _dynamicMgr->ReadInt16(el_ptr, offset % _elemScriptSize);
    }
    return *(uint16_t*)(el_ptr + offset % _elemScriptSize);
}

int32_t StaticArray::ReadInt32(const char *address, intptr_t offset)
{
    const char *el_ptr = GetElementPtr(address, offset);
    if (_staticMgr)
    {
        return _staticMgr->ReadInt32(el_ptr, offset % _elemScriptSize);
    }
    else if (_dynamicMgr)
    {
        return _dynamicMgr->ReadInt32(el_ptr, offset % _elemScriptSize);
    }
    return *(uint32_t*)(el_ptr + offset % _elemScriptSize);
}

float StaticArray::ReadFloat(const char *address, intptr_t offset)
{
    const char *el_ptr = GetElementPtr(address, offset);
    if (_staticMgr)
    {
        return _staticMgr->ReadFloat(el_ptr, offset % _elemScriptSize);
    }
    else if (_dynamicMgr)
    {
        return _dynamicMgr->ReadFloat(el_ptr, offset % _elemScriptSize);
    }
    return *(float*)(el_ptr + offset % _elemScriptSize);
}

void StaticArray::Write(const char *address, intptr_t offset, void *src, int size)
{
    const char *el_ptr = GetElementPtr(address, offset);
    if (_staticMgr)
    {
        return _staticMgr->Write(el_ptr, offset % _elemScriptSize, src, size);
    }
    else if (_dynamicMgr)
    {
        return _dynamicMgr->Write(el_ptr, offset % _elemScriptSize, src, size);
    }
    else
    {
        memcpy((void*)(el_ptr + offset % _elemScriptSize), src, size);
    }
}

void StaticArray::WriteInt8(const char *address, intptr_t offset, uint8_t val)
{
    const char *el_ptr = GetElementPtr(address, offset);
    if (_staticMgr)
    {
        return _staticMgr->WriteInt8(el_ptr, offset % _elemScriptSize, val);
    }
    else if (_dynamicMgr)
    {
        return _dynamicMgr->WriteInt8(el_ptr, offset % _elemScriptSize, val);
    }
    else
    {
        *(uint8_t*)(el_ptr + offset % _elemScriptSize) = val;
    }
}

void StaticArray::WriteInt16(const char *address, intptr_t offset, int16_t val)
{
    const char *el_ptr = GetElementPtr(address, offset);
    if (_staticMgr)
    {
        return _staticMgr->WriteInt16(el_ptr, offset % _elemScriptSize, val);
    }
    else if (_dynamicMgr)
    {
        return _dynamicMgr->WriteInt16(el_ptr, offset % _elemScriptSize, val);
    }
    else
    {
        *(uint16_t*)(el_ptr + offset % _elemScriptSize) = val;
    }
}

void StaticArray::WriteInt32(const char *address, intptr_t offset, int32_t val)
{
    const char *el_ptr = GetElementPtr(address, offset);
    if (_staticMgr)
    {
        return _staticMgr->WriteInt32(el_ptr, offset % _elemScriptSize, val);
    }
    else if (_dynamicMgr)
    {
        return _dynamicMgr->WriteInt32(el_ptr, offset % _elemScriptSize, val);
    }
    else
    {
        *(uint32_t*)(el_ptr + offset % _elemScriptSize) = val;
    }
}

void StaticArray::WriteFloat(const char *address, intptr_t offset, float val)
{
    const char *el_ptr = GetElementPtr(address, offset);
    if (_staticMgr)
    {
        return _staticMgr->WriteFloat(el_ptr, offset % _elemScriptSize, val);
    }
    else if (_dynamicMgr)
    {
        return _dynamicMgr->WriteFloat(el_ptr, offset % _elemScriptSize, val);
    }
    else
    {
        *(float*)(el_ptr + offset % _elemScriptSize) = val;
    }
}
