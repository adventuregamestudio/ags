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
#include <memory.h>
#include "scriptuserobject.h"
#include "ac/dynobj/dynobj_manager.h"
#include "util/stream.h"

using namespace AGS::Common;

const char *ScriptUserObject::TypeName = "UserObject";

// return the type name of the object
const char *ScriptUserObject::GetType()
{
    return TypeName;
}

ScriptUserObject::~ScriptUserObject()
{
    delete [] _data;
}

/* static */ ScriptUserObject *ScriptUserObject::CreateManaged(size_t size)
{
    ScriptUserObject *suo = new ScriptUserObject();
    suo->Create(nullptr, nullptr, size);
    ccRegisterManagedObject(suo, suo);
    return suo;
}

void ScriptUserObject::Create(const uint8_t *data, Stream *in, size_t size)
{
    delete [] _data;
    _data = nullptr;

    _size = size;
    if (_size > 0)
    {
        _data = new uint8_t[size];
        if (data)
            memcpy(_data, data, _size);
        else if (in)
            in->Read(_data, _size);
        else
            memset(_data, 0, _size);
    }
}

int ScriptUserObject::Dispose(void* /*address*/, bool /*force*/)
{
    delete this;
    return 1;
}

size_t ScriptUserObject::CalcSerializeSize(void* /*address*/)
{
    return _size;
}

void ScriptUserObject::Serialize(void* /*address*/, AGS::Common::Stream *out)
{
    out->Write(_data, _size);
}

void ScriptUserObject::Unserialize(int index, Stream *in, size_t data_sz)
{
    Create(nullptr, in, data_sz);
    ccRegisterUnserializedObject(index, this, this);
}

void* ScriptUserObject::GetFieldPtr(void* /*address*/, intptr_t offset)
{
    return _data + offset;
}

void ScriptUserObject::Read(void* /*address*/, intptr_t offset, uint8_t *dest, size_t size)
{
    memcpy(dest, _data + offset, size);
}

uint8_t ScriptUserObject::ReadInt8(void* /*address*/, intptr_t offset)
{
    return *(uint8_t*)(_data + offset);
}

int16_t ScriptUserObject::ReadInt16(void* /*address*/, intptr_t offset)
{
    return *(int16_t*)(_data + offset);
}

int32_t ScriptUserObject::ReadInt32(void* /*address*/, intptr_t offset)
{
    return *(int32_t*)(_data + offset);
}

float ScriptUserObject::ReadFloat(void* /*address*/, intptr_t offset)
{
    return *(float*)(_data + offset);
}

void ScriptUserObject::Write(void* /*address*/, intptr_t offset, const uint8_t *src, size_t size)
{
    memcpy((void*)(_data + offset), src, size);
}

void ScriptUserObject::WriteInt8(void* /*address*/, intptr_t offset, uint8_t val)
{
    *(uint8_t*)(_data + offset) = val;
}

void ScriptUserObject::WriteInt16(void* /*address*/, intptr_t offset, int16_t val)
{
    *(int16_t*)(_data + offset) = val;
}

void ScriptUserObject::WriteInt32(void* /*address*/, intptr_t offset, int32_t val)
{
    *(int32_t*)(_data + offset) = val;
}

void ScriptUserObject::WriteFloat(void* /*address*/, intptr_t offset, float val)
{
    *(float*)(_data + offset) = val;
}


// Allocates managed struct containing two ints: X and Y
ScriptUserObject *ScriptStructHelpers::CreatePoint(int x, int y)
{
    ScriptUserObject *suo = ScriptUserObject::CreateManaged(sizeof(int32_t) * 2);
    suo->WriteInt32(suo, 0, x);
    suo->WriteInt32(suo, sizeof(int32_t), y);
    return suo;
}
