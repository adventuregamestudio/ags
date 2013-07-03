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
//
//
//=============================================================================
#ifndef __AGS_EE_STATOBJ__STATICARRAY_H
#define __AGS_EE_STATOBJ__STATICARRAY_H

#include "ac/statobj/staticobject.h"
#include "util/array.h"

struct ICCDynamicObject;

struct StaticArray : public ICCStaticObject
{
public:
    StaticArray()
        : _dynamicMgr(NULL)
        , _elemLegacySize(0)
        , _elemRealSize(0)
    {
    }

    inline ICCDynamicObject *GetDynamicManager() const
    {
        return _dynamicMgr;
    }
    // Legacy support for reading and writing object values by their relative offset
    inline const char *GetElementPtr(const char *address, intptr_t legacy_offset)
    {
        return address + (legacy_offset / _elemLegacySize) * _elemRealSize;
    }

    virtual uint8_t GetPropertyUInt8(const char *address, intptr_t offset);
    virtual int16_t GetPropertyInt16(const char *address, intptr_t offset);
    virtual int32_t GetPropertyInt32(const char *address, intptr_t offset);
    virtual void    SetPropertyUInt8(const char *address, intptr_t offset, uint8_t value);
    virtual void    SetPropertyInt16(const char *address, intptr_t offset, int16_t value);
    virtual void    SetPropertyInt32(const char *address, intptr_t offset, int32_t value);

protected:
    ICCDynamicObject    *_dynamicMgr;
    size_t              _elemLegacySize;
    size_t              _elemRealSize;
};

template<class T> class StaticTemplateArray : public StaticArray
{
public:
    void Create(AGS::Common::Array<T> *array, ICCDynamicObject *dynmgr, int elem_legacy_size)
    {
        _array          = array;
        _dynamicMgr     = dynmgr;
        _elemLegacySize = elem_legacy_size;
        _elemRealSize   = sizeof(T);
    }

    inline const char *GetLinearArray()
    {
        return _array ? (const char*)_array->GetCArr() : NULL;
    }

protected:
    AGS::Common::Array<T> *_array;
};

#endif // __AGS_EE_STATOBJ__STATICOBJECT_H
