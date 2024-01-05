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
#include "ac/dynobj/scriptmouse.h"
#include "debug/debug_log.h"
#include "script/cc_common.h"

int32_t ScriptMouse::ReadInt32(void *address, intptr_t offset)
{
    switch (offset)
    {
    case 0: return x;
    case 4: return y;
    default:
        cc_error("ScriptMouse: unsupported variable offset %d", offset);
        return 0;
    }
}

void ScriptMouse::WriteInt32(void *address, intptr_t offset, int32_t val)
{
    switch (offset)
    {
    case 0:
    case 4:
        debug_script_warn("ScriptMouse: attempt to write in readonly variable at offset %d, value", offset, val);
        break;
    default:
        cc_error("ScriptMouse: unsupported variable offset %d", offset);
        break;
    }
}
