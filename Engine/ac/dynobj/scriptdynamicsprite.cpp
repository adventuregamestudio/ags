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
#include "ac/dynobj/scriptdynamicsprite.h"
#include "ac/dynobj/dynobj_manager.h"
#include "ac/dynamicsprite.h"
#include "util/stream.h"

using namespace AGS::Common;

const char *ScriptDynamicSprite::TypeID = "DynamicSprite";

int ScriptDynamicSprite::Dispose(void* /*address*/, bool force) {
    // always dispose
    if ((slot) && (!force))
        free_dynamic_sprite(slot);

    delete this;
    return 1;
}

const char *ScriptDynamicSprite::GetType() {
    return ScriptDynamicSprite::TypeID;
}

size_t ScriptDynamicSprite::CalcSerializeSize(const void* /*address*/)
{
    return sizeof(int32_t);
}

void ScriptDynamicSprite::Serialize(const void* /*address*/, Stream *out) {
    out->WriteInt32(slot);
}

void ScriptDynamicSprite::Unserialize(int index, Stream *in, size_t /*data_sz*/) {
    slot = in->ReadInt32();
    ccRegisterUnserializedObject(index, this, this);
}

ScriptDynamicSprite::ScriptDynamicSprite(int theSlot) {
    slot = theSlot;
    ccRegisterManagedObject(this, this);
}
