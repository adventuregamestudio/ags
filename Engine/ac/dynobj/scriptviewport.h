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

#ifndef __AC_SCRIPTVIEWPORT_H
#define __AC_SCRIPTVIEWPORT_H

#include "ac/dynobj/cc_agsdynamicobject.h"

// ScriptViewport manages room Viewport struct in script.
// Currently it has no members and actual data is stored in "GameState" struct.
// Also in practice there is only single room viewport at the moment.
struct ScriptViewport final : AGSCCDynamicObject
{
    virtual const char *GetType();
    virtual int Serialize(const char *address, char *buffer, int bufsize);
    virtual void Unserialize(int index, const char *serializedData, int dataSize);
};

#endif // __AC_SCRIPTVIEWPORT_H
