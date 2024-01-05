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

/* static */ DynObjectRef ScriptUserObject::Create(size_t size)
{
    uint8_t *new_data = new uint8_t[size + MemHeaderSz];
    memset(new_data, 0, size + MemHeaderSz);
    Header &hdr = reinterpret_cast<Header&>(*new_data);
    hdr.Size = size;
    void *obj_ptr = &new_data[MemHeaderSz];
    int32_t handle = ccRegisterManagedObject(obj_ptr, &globalDynamicStruct);
    if (handle == 0)
    {
        delete[] new_data;
        return DynObjectRef();
    }
    return DynObjectRef(handle, obj_ptr, &globalDynamicStruct);
}

int ScriptUserObject::Dispose(void *address, bool /*force*/)
{
    delete[] (static_cast<uint8_t*>(address) - MemHeaderSz);
    return 1;
}

size_t ScriptUserObject::CalcSerializeSize(const void *address)
{
    const Header &hdr = GetHeader(address);
    return hdr.Size + FileHeaderSz;
}

void ScriptUserObject::Serialize(const void *address, AGS::Common::Stream *out)
{
    const Header &hdr = GetHeader(address);
    // NOTE: we only write the data, no header at the moment
    out->Write(address, hdr.Size);
}

void ScriptUserObject::Unserialize(int index, Stream *in, size_t data_sz)
{
    uint8_t *new_data = new uint8_t[(data_sz - FileHeaderSz) + MemHeaderSz];
    Header &hdr = reinterpret_cast<Header&>(*new_data);
    hdr.Size = data_sz - FileHeaderSz;
    in->Read(new_data + MemHeaderSz, data_sz - FileHeaderSz);
    ccRegisterUnserializedObject(index, &new_data[MemHeaderSz], this);
}

ScriptUserObject globalDynamicStruct;


// Allocates managed struct containing two ints: X and Y
ScriptUserObject *ScriptStructHelpers::CreatePoint(int x, int y)
{
    DynObjectRef ref = ScriptUserObject::Create(sizeof(int32_t) * 2);
    ref.Mgr->WriteInt32(ref.Obj, 0, x);
    ref.Mgr->WriteInt32(ref.Obj, sizeof(int32_t), y);
    return static_cast<ScriptUserObject*>(ref.Obj);
}
