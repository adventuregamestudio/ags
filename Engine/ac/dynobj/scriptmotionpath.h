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
#ifndef __AGS_EE_DYNOBJ__SCRIPTMOTIONPATH_H
#define __AGS_EE_DYNOBJ__SCRIPTMOTIONPATH_H

#include <memory>
#include "ac/dynobj/cc_agsdynamicobject.h"
#include "ac/movelist.h"
#include "ac/runtime_defines.h"


class ScriptMotionPath : public AGSCCDynamicObject
{
public:
    ScriptMotionPath(uint32_t mlist_id);
    bool IsValid() const;
    MoveList *GetMoveList() const;
    void Invalidate();

    const char *GetType() override;
    int Dispose(void *address, bool force) override;
    void Unserialize(int index, AGS::Common::Stream *in, size_t data_sz) override;

    static DynObjectRef Create(uint32_t mlist_id);

private:
    // Calculate and return required space for serialization, in bytes
    size_t CalcSerializeSize(const void *address) override;
    // Write object data into the provided stream
    void Serialize(const void *address, AGS::Common::Stream *out) override;

    uint32_t _moveListID = 0u;
};

#endif // __AGS_EE_DYNOBJ__SCRIPTMOTIONPATH_H
