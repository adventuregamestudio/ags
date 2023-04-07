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
// Helper classes for easier working with the runtime reflection (RTTI etc).
//
//=============================================================================
#ifndef __AGS_EE_SCRIPT__REFLECTHELPER_H
#define __AGS_EE_SCRIPT__REFLECTHELPER_H

#include "script/cc_reflect.h"

namespace AGS
{
namespace Engine
{

class RTTIHelper
{
public:
    // Helper struct for referencing a range of type's fields
    struct TypeFieldsRef
    {
        uint32_t field_num = 0u; // number of fields, if any
        uint32_t field_index = 0u; // first field index in the fields table
    };

    // Generate data based on full RTTI; this overwrite whole data
    // WARNING: assumes RTTI has sequential type ids (joint RTTI); FIXME: enforce this?
    // FIXME: allow to skip existing non-GENERATED types
    void Generate(const RTTI &rtti);

    // Returns a range of managed offsets (as a pair of [begin;end) iterators),
    // containing a list of managed fields offsets for the given type
    std::pair<std::vector<uint32_t>::const_iterator, std::vector<uint32_t>::const_iterator>
        GetManagedOffsetsForType(uint32_t type_id) const;

private:
    // Quick-reference for the managed pointer fields in types
    std::vector<TypeFieldsRef> _managedFieldsRef;
    std::vector<uint32_t> _managedOffsets; // includes nested regular structs!
};

}
}

#endif // __AGS_EE_SCRIPT__REFLECTHELPER_H