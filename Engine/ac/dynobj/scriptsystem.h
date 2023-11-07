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
//
// Wrapper around script "System" struct, managing access to its variables.
// ScriptSystem is a readable/writeable struct which had been exposed to
//
//=============================================================================
#ifndef __AGS_EE_DYNOBJ__SCRIPTSYSTEM_H
#define __AGS_EE_DYNOBJ__SCRIPTSYSTEM_H

#include "ac/dynobj/cc_agsdynamicobject.h"

struct ScriptSystem : public AGSCCStaticObject
{
    int width,height;
    int coldepth;
    int os;
    int windowed;
    int vsync;
    int viewport_width;
    int viewport_height;

    int32_t ReadInt32(void *address, intptr_t offset) override;
    void    WriteInt32(void *address, intptr_t offset, int32_t val) override;
};

#endif // __AGS_EE_DYNOBJ__SCRIPTSYSTEM_H
