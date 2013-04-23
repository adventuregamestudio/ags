
#include <string.h>
#include "ac/statobj/staticarray.h"
#include "ac/dynobj/cc_dynamicobject.h"

void StaticArray::Create(int elem_legacy_size, int elem_real_size, int elem_count)
{
    _staticMgr      = NULL;
    _dynamicMgr     = NULL;
    _elemLegacySize = elem_legacy_size;
    _elemRealSize   = elem_real_size;
    _elemCount      = elem_count;
}

void StaticArray::Create(ICCStaticObject *stcmgr, int elem_legacy_size, int elem_real_size, int elem_count)
{
    _staticMgr      = stcmgr;
    _dynamicMgr     = NULL;
    _elemLegacySize = elem_legacy_size;
    _elemRealSize   = elem_real_size;
    _elemCount      = elem_count;
}

void StaticArray::Create(ICCDynamicObject *dynmgr, int elem_legacy_size, int elem_real_size, int elem_count)
{
    _staticMgr      = NULL;
    _dynamicMgr     = dynmgr;
    _elemLegacySize = elem_legacy_size;
    _elemRealSize   = elem_real_size;
    _elemCount      = elem_count;
}

const char *StaticArray::GetElementPtr(const char *address, intptr_t legacy_offset)
{
    return address + (legacy_offset / _elemLegacySize) * _elemRealSize;
}

uint8_t StaticArray::GetPropertyUInt8(const char *address, intptr_t offset)
{
    const char *el_ptr = GetElementPtr(address, offset);
    if (_staticMgr)
    {
        return _staticMgr->GetPropertyUInt8(el_ptr, offset % _elemLegacySize);
    }
    else if (_dynamicMgr)
    {
        return _dynamicMgr->GetPropertyUInt8(el_ptr, offset % _elemLegacySize);
    }
    return *(uint8_t*)(el_ptr + offset % _elemLegacySize);
}

int16_t StaticArray::GetPropertyInt16(const char *address, intptr_t offset)
{
    const char *el_ptr = GetElementPtr(address, offset);
    if (_staticMgr)
    {
        return _staticMgr->GetPropertyInt16(el_ptr, offset % _elemLegacySize);
    }
    else if (_dynamicMgr)
    {
        return _dynamicMgr->GetPropertyInt16(el_ptr, offset % _elemLegacySize);
    }
    return *(uint16_t*)(el_ptr + offset % _elemLegacySize);
}

int32_t StaticArray::GetPropertyInt32(const char *address, intptr_t offset)
{
    const char *el_ptr = GetElementPtr(address, offset);
    if (_staticMgr)
    {
        return _staticMgr->GetPropertyInt32(el_ptr, offset % _elemLegacySize);
    }
    else if (_dynamicMgr)
    {
        return _dynamicMgr->GetPropertyInt32(el_ptr, offset % _elemLegacySize);
    }
    return *(uint32_t*)(el_ptr + offset % _elemLegacySize);
}

void StaticArray::SetPropertyUInt8(const char *address, intptr_t offset, uint8_t value)
{
    const char *el_ptr = GetElementPtr(address, offset);
    if (_staticMgr)
    {
        return _staticMgr->SetPropertyUInt8(el_ptr, offset % _elemLegacySize, value);
    }
    else if (_dynamicMgr)
    {
        return _dynamicMgr->SetPropertyUInt8(el_ptr, offset % _elemLegacySize, value);
    }
    else
    {
        *(uint8_t*)(el_ptr + offset % _elemLegacySize) = value;
    }
}

void StaticArray::SetPropertyInt16(const char *address, intptr_t offset, int16_t value)
{
    const char *el_ptr = GetElementPtr(address, offset);
    if (_staticMgr)
    {
        return _staticMgr->SetPropertyInt16(el_ptr, offset % _elemLegacySize, value);
    }
    else if (_dynamicMgr)
    {
        return _dynamicMgr->SetPropertyInt16(el_ptr, offset % _elemLegacySize, value);
    }
    else
    {
        *(uint16_t*)(el_ptr + offset % _elemLegacySize) = value;
    }
}

void StaticArray::SetPropertyInt32(const char *address, intptr_t offset, int32_t value)
{
    const char *el_ptr = GetElementPtr(address, offset);
    if (_staticMgr)
    {
        return _staticMgr->SetPropertyInt32(el_ptr, offset % _elemLegacySize, value);
    }
    else if (_dynamicMgr)
    {
        return _dynamicMgr->SetPropertyInt32(el_ptr, offset % _elemLegacySize, value);
    }
    else
    {
        *(uint32_t*)(el_ptr + offset % _elemLegacySize) = value;
    }
}
