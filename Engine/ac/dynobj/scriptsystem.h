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
// Wrapper around script "System" struct, managing access to its variables.
//
//=============================================================================
#ifndef __AGS_EE_DYNOBJ__SCRIPTSYSTEM_H
#define __AGS_EE_DYNOBJ__SCRIPTSYSTEM_H

#include "ac/statobj/agsstaticobject.h"

// The text script's "system" struct
struct ScriptSystem : public AGSStaticObject
{
    int width,height;
    int coldepth;
    int os;
    int windowed;
    int vsync;
    int viewport_width;
    int viewport_height;

    virtual int32_t ReadInt32(const char *address, intptr_t offset) override;
    virtual void    WriteInt32(const char *address, intptr_t offset, int32_t val) override;
};

#endif // __AGS_EE_DYNOBJ__SCRIPTSYSTEM_H
