//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "ac/dynobj/cc_walkablearea.h"
#include "ac/dynobj/scriptobjects.h"
#include "ac/dynobj/dynobj_manager.h"
#include "ac/common_defines.h"
#include "game/roomstruct.h"
#include "util/stream.h"

using namespace AGS::Common;

extern ScriptWalkableArea scrWalkarea[MAX_WALK_AREAS];

// return the type name of the object
const char *CCWalkableArea::GetType() {
    return "WalkableArea";
}

size_t CCWalkableArea::CalcSerializeSize(const void* /*address*/)
{
    return sizeof(int32_t);
}

void CCWalkableArea::Serialize(const void *address, Stream *out) {
    const ScriptWalkableArea *shh = static_cast<const ScriptWalkableArea*>(address);
    out->WriteInt32(shh->id);
}

void CCWalkableArea::Unserialize(int index, Stream *in, size_t /*data_sz*/) {
    int num = in->ReadInt32();
    ccRegisterUnserializedPersistentObject(index, &scrWalkarea[num], this);
}
