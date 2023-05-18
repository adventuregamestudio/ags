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
#include <string.h>
#include "ac/dynobj/cc_agsdynamicobject.h"
#include "ac/common.h"               // quit()
#include "util/memorystream.h"

using namespace AGS::Common;

// *** The script serialization routines for built-in types

int AGSCCDynamicObject::Serialize(const char *address, char *buffer, int bufsize) {
    // If the required space is larger than the provided buffer,
    // then return negated required space, notifying the caller that a larger buffer is necessary
    size_t req_size = CalcSerializeSize(address);
    if (bufsize < 0 || req_size > static_cast<size_t>(bufsize))
        return -(static_cast<int32_t>(req_size));

    MemoryStream mems(reinterpret_cast<uint8_t*>(buffer), bufsize, kStream_Write);
    Serialize(address, &mems);
    return static_cast<int32_t>(mems.GetPosition());
}
