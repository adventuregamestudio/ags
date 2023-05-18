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
#include "ac/dynobj/managedobjectpool.h"
#include "script/cc_script.h"
#include "script/cc_instance.h"
#include "util/memorystream.h"
#include "util/stream.h"

using namespace AGS::Common;

const char *ScriptUserObject::TypeName = "UserObj2";

// return the type name of the object
const char *ScriptUserObject::GetType()
{
    return TypeName;
}

ScriptUserObject::~ScriptUserObject()
{
    delete [] _data;
}

/* static */ ScriptUserObject *ScriptUserObject::CreateManaged(uint32_t type_id, size_t size)
{
    ScriptUserObject *suo = new ScriptUserObject();
    suo->Create(nullptr, nullptr, type_id, size);
    ccRegisterManagedObject(suo, suo);
    return suo;
}

void ScriptUserObject::Create(const char *data, Stream *in, uint32_t type_id, size_t size)
{
    delete [] _data;
    _data = nullptr;

    _typeid = type_id;
    _size = size;
    if (_size > 0)
    {
        _data = new char[size];
        if (data)
            memcpy(_data, data, _size);
        else if (in)
            in->Read(_data, _size);
        else
            memset(_data, 0, _size);
    }
}

int ScriptUserObject::Dispose(const char* address, bool force)
{
    // Unref all managed pointers within the struct
    if (!force && (_typeid > 0))
    {
        TraverseRefs(address, [](int handle) { pool.SubRefNoCheck(handle); });
    }

    delete this;
    return 1;
}

size_t ScriptUserObject::CalcSerializeSize(const char * /*address*/)
{
    const size_t hdr_sz = sizeof(uint32_t) * 2; // typeid and size
    return _size + hdr_sz;
}

void ScriptUserObject::Serialize(const char *address, AGS::Common::Stream *out)
{
    const size_t hdr_sz = sizeof(uint32_t) * 2; // typeid and size
    out->WriteInt32(hdr_sz); // header size, reserve for the future
    out->WriteInt32(_typeid); // type id
    out->Write(_data, _size); // main data
}

void ScriptUserObject::Unserialize(int index, Stream *in, size_t data_sz)
{
    // TODO: should we support older save versions here?
    // might have to use class name (GetType) to distinguish save formats in UnSerializer
    size_t hdr_sz = static_cast<uint32_t>(in->ReadInt32());
    _typeid = static_cast<uint32_t>(in->ReadInt32());
    Create(nullptr, in, _typeid, data_sz - hdr_sz);
    ccRegisterUnserializedObject(index, this, this);
}

void ScriptUserObject::RemapTypeids(const char* /*address*/,
    const std::unordered_map<uint32_t, uint32_t> &typeid_map)
{
    const auto it = typeid_map.find(_typeid);
    assert(_typeid == 0u || it != typeid_map.end());
    _typeid = (it != typeid_map.end()) ? it->second : 0u;
}

void ScriptUserObject::TraverseRefs(const char *address, PfnTraverseRefOp traverse_op)
{
    // TODO: may be a bit faster if we make a "runtime type"
    // struct, merging joint type info and auxiliary helper data,
    // and store a pointer in the arr data.
    // might also have a dummy "type" for "unknown type" arrays.
    if (_typeid == 0u) return;
    assert(ccInstance::GetRTTI()->GetTypes().size() > _typeid);
    const auto *helper = ccInstance::GetRTTIHelper();
    const auto fref = helper->GetManagedOffsetsForType(_typeid);
    for (auto it = fref.first; it < fref.second; ++it)
    {
        traverse_op(*(int32_t*)(_data + *it));
    }
}

const char* ScriptUserObject::GetFieldPtr(const char* /*address*/, intptr_t offset)
{
    return _data + offset;
}

void ScriptUserObject::Read(const char* /*address*/, intptr_t offset, void *dest, int size)
{
    memcpy(dest, _data + offset, size);
}

uint8_t ScriptUserObject::ReadInt8(const char* /*address*/, intptr_t offset)
{
    return *(uint8_t*)(_data + offset);
}

int16_t ScriptUserObject::ReadInt16(const char* /*address*/, intptr_t offset)
{
    return *(int16_t*)(_data + offset);
}

int32_t ScriptUserObject::ReadInt32(const char* /*address*/, intptr_t offset)
{
    return *(int32_t*)(_data + offset);
}

float ScriptUserObject::ReadFloat(const char* /*address*/, intptr_t offset)
{
    return *(float*)(_data + offset);
}

void ScriptUserObject::Write(const char* /*address*/, intptr_t offset, void *src, int size)
{
    memcpy((void*)(_data + offset), src, size);
}

void ScriptUserObject::WriteInt8(const char* /*address*/, intptr_t offset, uint8_t val)
{
    *(uint8_t*)(_data + offset) = val;
}

void ScriptUserObject::WriteInt16(const char* /*address*/, intptr_t offset, int16_t val)
{
    *(int16_t*)(_data + offset) = val;
}

void ScriptUserObject::WriteInt32(const char* /*address*/, intptr_t offset, int32_t val)
{
    *(int32_t*)(_data + offset) = val;
}

void ScriptUserObject::WriteFloat(const char* /*address*/, intptr_t offset, float val)
{
    *(float*)(_data + offset) = val;
}


// Allocates managed struct containing two ints: X and Y
ScriptUserObject *ScriptStructHelpers::CreatePoint(int x, int y)
{
    // FIXME: type id! (is it possible to RTTI?)
    ScriptUserObject *suo = ScriptUserObject::CreateManaged(RTTI::NoType, sizeof(int32_t) * 2);
    suo->WriteInt32((const char*)suo, 0, x);
    suo->WriteInt32((const char*)suo, sizeof(int32_t), y);
    return suo;
}
