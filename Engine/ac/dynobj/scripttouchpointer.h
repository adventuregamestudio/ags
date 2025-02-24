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
#ifndef __AGS_EE_DYNOBJ__SCRIPTTOUCHPOINTER_H
#define __AGS_EE_DYNOBJ__SCRIPTTOUCHPOINTER_H

#include "ac/dynobj/cc_agsdynamicobject.h"

// TODO: make a shared parent class for the script objects which serve
// as simple wrappers over an object index, and implement AGSCCDynamicObject.
// See classes like CCHotspot, CCWalkableArea, ScriptJoystick etc.
struct ScriptTouchPointer final : AGSCCDynamicObject
{
public:
    ScriptTouchPointer(int pointer_id)
        : _pointerID(pointer_id) {}

    int GetPointerID() const { return _pointerID; }

    const char *GetType() override;
    void Unserialize(int index, AGS::Common::Stream *in, size_t data_sz) override;

private:
    // Calculate and return required space for serialization, in bytes
    size_t CalcSerializeSize(const void *address) override;
    // Write object data into the provided stream
    void Serialize(const void *address, AGS::Common::Stream *out) override;

    int _pointerID = -1;
};

#endif // __AGS_EE_DYNOBJ__SCRIPTTOUCHPOINTER_H
