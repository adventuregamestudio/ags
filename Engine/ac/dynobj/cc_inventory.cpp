//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "ac/dynobj/cc_inventory.h"
#include "ac/dynobj/scriptobjects.h"
#include "ac/dynobj/dynobj_manager.h"
#include "ac/characterinfo.h"
#include "util/stream.h"

using namespace AGS::Common;
extern std::vector<int> StaticInventoryArray;
extern std::vector<ScriptInvItem> scrInv;

ScriptInvItem CCInventory::_dummy;

// return the type name of the object
const char *CCInventory::GetType()
{
    return "Inventory";
}

size_t CCInventory::CalcSerializeSize(const void* /*address*/)
{
    return sizeof(int32_t);
}

void CCInventory::Serialize(const void *address, Stream *out)
{
    const ScriptInvItem *shh = static_cast<const ScriptInvItem*>(address);
    out->WriteInt32(shh->id);
}

void CCInventory::Unserialize(int index, Stream *in, size_t /*data_sz*/)
{
    // NOTE: older versions apparently had MAX_INV script objects registered in managed memory.
    // TODO: share this algorithm with all managed object unserialization (make template function?).
    int num = in->ReadInt32();
    if (num >= 0 && static_cast<uint32_t>(num) < scrInv.size())
    {
        StaticInventoryArray[num] = ccRegisterUnserializedPersistentObject(index, &scrInv[num], this);
    }
    else
    {
        // WARNING: we must not reallocate StaticInventoryArray or scrInv, because that will break script array
        // and script objects registration respectively.
        ccRegisterUnserializedPersistentObject(index, &_dummy, this);
    }
}
