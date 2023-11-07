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

/* static */ DynObjectRef ScriptUserObject::Create(uint32_t type_id, size_t size)
{
    uint8_t *new_data = new uint8_t[size + MemHeaderSz];
    memset(new_data, 0, size + MemHeaderSz);
    Header &hdr = reinterpret_cast<Header&>(*new_data);
    hdr.TypeId = type_id;
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

int ScriptUserObject::Dispose(void *address, bool force)
{
    const Header &hdr = GetHeader(address);
    // Unref all managed pointers within the struct
    if (!force && (hdr.TypeId > 0))
    {
        TraverseRefs(address, [](int handle) { pool.SubRefNoCheck(handle); });
    }

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
    out->WriteInt32(FileHeaderSz); // header size, reserve for the future
    out->WriteInt32(hdr.TypeId); // type id
    out->Write(address, hdr.Size); // main data
}

void ScriptUserObject::Unserialize(int index, Stream *in, size_t data_sz)
{
    uint8_t *new_data = new uint8_t[(data_sz - FileHeaderSz) + MemHeaderSz];
    Header &hdr = reinterpret_cast<Header&>(*new_data);
    // TODO: should we support older save versions here?
    // might have to use class name (GetType) to distinguish save formats in UnSerializer
    size_t hdr_sz = static_cast<uint32_t>(in->ReadInt32());
    hdr.TypeId = static_cast<uint32_t>(in->ReadInt32());
    hdr.Size = (data_sz - FileHeaderSz);
    in->Read(new_data + MemHeaderSz, hdr.Size);
    ccRegisterUnserializedObject(index, &new_data[MemHeaderSz], this);
}

void ScriptUserObject::RemapTypeids(void *address,
    const std::unordered_map<uint32_t, uint32_t> &typeid_map)
{
    Header &hdr = GetHeaderW(address);
    const auto it = typeid_map.find(hdr.TypeId);
    assert(hdr.TypeId == 0u || it != typeid_map.end());
    hdr.TypeId = (it != typeid_map.end()) ? it->second : 0u;
}

void ScriptUserObject::TraverseRefs(void *address, PfnTraverseRefOp traverse_op)
{
    // TODO: may be a bit faster if we make a "runtime type"
    // struct, merging joint type info and auxiliary helper data,
    // and store a pointer in the arr data.
    // might also have a dummy "type" for "unknown type" arrays.
    const Header &hdr = GetHeader(address);
    if (hdr.TypeId == 0u)
        return;
    assert(ccInstance::GetRTTI()->GetTypes().size() > hdr.TypeId);
    const auto *helper = ccInstance::GetRTTIHelper();
    const auto fref = helper->GetManagedOffsetsForType(hdr.TypeId);
    for (auto it = fref.first; it < fref.second; ++it)
    {
        traverse_op(*(int32_t*)(static_cast<uint8_t*>(address) + *it));
    }
}

ScriptUserObject globalDynamicStruct;


// Allocates managed struct containing two ints: X and Y
ScriptUserObject *ScriptStructHelpers::CreatePoint(int x, int y)
{
    // FIXME: type id! (is it possible to RTTI?)
    DynObjectRef ref = ScriptUserObject::Create(RTTI::NoType, sizeof(int32_t) * 2);
    ref.Mgr->WriteInt32(ref.Obj, 0, x);
    ref.Mgr->WriteInt32(ref.Obj, sizeof(int32_t), y);
    return static_cast<ScriptUserObject*>(ref.Obj);
}
