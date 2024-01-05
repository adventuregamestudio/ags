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
#include "ac/dynobj/scriptsystem.h"
#include "debug/debug_log.h"
#include "script/cc_common.h"

int32_t ScriptSystem::ReadInt32(void *address, intptr_t offset)
{
    const int index = offset / sizeof(int32_t);
    switch (index)
    {
    case 0: return width;
    case 1: return height;
    case 2: return coldepth;
    case 3: return os;
    case 4: return windowed;
    case 5: return vsync;
    case 6: return viewport_width;
    case 7: return viewport_height;
    default:
        cc_error("ScriptSystem: unsupported variable offset %d", offset);
        return 0;
    }
}

void ScriptSystem::WriteInt32(void *address, intptr_t offset, int32_t val)
{
    const int index = offset / sizeof(int32_t);
    switch (index)
    {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 6:
    case 7:
        debug_script_warn("ScriptSystem: attempt to write in readonly variable at offset %d, value %d", offset, val);
        break;
    case 5: vsync = val; break;
    default:
        cc_error("ScriptSystem: unsupported variable offset %d", offset);
        break;
    }
}
