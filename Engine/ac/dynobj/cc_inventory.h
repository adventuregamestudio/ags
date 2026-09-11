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
#ifndef __AC_CCINVENTORY_H
#define __AC_CCINVENTORY_H

#include "ac/dynobj/cc_agsdynamicobject.h"
#include "ac/dynobj/scriptobjects.h"

struct CCInventory final : AGSCCDynamicObject
{
public:
    // return the type name of the object
    const char *GetType() override;
    void Unserialize(int index, AGS::Common::Stream *in, size_t data_sz) override;

private:
    // Calculate and return required space for serialization, in bytes
    size_t CalcSerializeSize(const void *address) override;
    // Write object data into the provided stream
    void Serialize(const void *address, AGS::Common::Stream *out) override;

    static ScriptInvItem _dummy; // for registering invalid restored objects
};

#endif // __AC_CCINVENTORY_H
