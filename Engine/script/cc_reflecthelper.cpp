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
#include "script/cc_reflecthelper.h"

namespace AGS
{
namespace Engine
{

static void ResolvePtrsInStruct(const RTTI::Type &ti, const std::vector<RTTI::Type> &types,
    const std::vector<RTTIHelper::TypeFieldsRef> &field_refs,
    std::vector<uint32_t> &offsets, uint32_t start_off)
{
    for (const auto *fi = ti.first_field; fi; fi = fi->next_field)
    {
        const uint32_t field_off = start_off + fi->offset;
        if (fi->flags & RTTI::kField_ManagedPtr)
        { // either a pointer to managed struct, or dynamic array
            offsets.push_back(field_off);
            continue;
        }
        
        assert(fi->f_typeid < types.size());
        if (fi->f_typeid >= types.size())
            continue;

        const auto &ti = types[fi->f_typeid];
        if ((fi->flags & RTTI::kField_Array) && (ti.flags & RTTI::kType_Managed))
        { // regular array of managed pointers
            for (uint32_t el = 0; el < fi->num_elems; ++el)
                offsets.push_back(field_off + el * RTTI::PointerSize);
            continue;
        }

        if (ti.flags & RTTI::kType_Struct)
        { // a struct or an array of structs
            // First try if this struct was already calculated
            if (fi->f_typeid < field_refs.size())
            {
                const auto &fref = field_refs[fi->f_typeid];
                for (uint32_t el = 0; el < fi->num_elems; ++el)
                    for (uint32_t foff = 0; foff < fref.field_num; ++foff)
                        offsets.push_back(field_off + (el * ti.size) + offsets[fref.field_index + foff]);
            }
            // Otherwise, calculate nested structs recursively
            else if (fi->num_elems == 0)
            {
                ResolvePtrsInStruct(ti, types, field_refs, offsets, field_off);
            }
            else
            {
                for (uint32_t el = 0; el < fi->num_elems; ++el)
                    ResolvePtrsInStruct(ti, types, field_refs, offsets, field_off + el * ti.size);
            }
        }
    }
}

// TODO: optimize this by (optionally?) keeping previously generated data
// of the known types, and only generating for new types
// FIXME: allow to skip existing non-GENERATED types
void RTTIHelper::Generate(const RTTI &rtti)
{
    const auto &types = rtti.GetTypes();

    _managedFieldsRef.clear();
    _managedOffsets.clear();
    for (const auto &ti : types)
    {
        // TODO: need to somehow NOT rely on field type having less IDs than its owner:
        // script compiler might support random declaration order at some point?!
        uint32_t man_findex = _managedOffsets.size();
        ResolvePtrsInStruct(ti, types, _managedFieldsRef, _managedOffsets, 0);
        RTTIHelper::TypeFieldsRef ref;
        ref.field_index = man_findex;
        ref.field_num = _managedOffsets.size() - man_findex;
        _managedFieldsRef.push_back(ref);
    }
}

std::pair<std::vector<uint32_t>::const_iterator, std::vector<uint32_t>::const_iterator>
    RTTIHelper::GetManagedOffsetsForType(uint32_t type_id) const
{
    assert(!_managedFieldsRef.empty());
    assert(type_id < _managedFieldsRef.size());
    if (type_id >= _managedFieldsRef.size() || _managedFieldsRef.empty())
        return std::make_pair(_managedOffsets.end(), _managedOffsets.end());
    const auto &fref = _managedFieldsRef[type_id];
    return std::make_pair(_managedOffsets.begin() + fref.field_index,
        _managedOffsets.begin() + fref.field_index + fref.field_num);
}

} // namespace Engine
} // namespace AGS
