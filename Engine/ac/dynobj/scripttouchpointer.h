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
#ifndef __AGS_EE_DYNOBJ__SCRIPTTOUCHPOINTER_H
#define __AGS_EE_DYNOBJ__SCRIPTTOUCHPOINTER_H

#include "ac/dynobj/cc_agsdynamicobject.h"

struct ScriptTouchPointer final : CCBasicObject
{
public:
    ScriptTouchPointer(int pointer_id)
        : _pointerID(pointer_id) {}

    int GetPointerID() const { return _pointerID; }
    void Invalidate() { _pointerID = -1; }

    const char *GetType() override;
    int Dispose(void *address, bool force);

private:
    int _pointerID = -1;
};

#endif // __AGS_EE_DYNOBJ__SCRIPTTOUCHPOINTER_H
