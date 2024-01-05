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

#ifndef __AC_SCRIPTCAMERA_H
#define __AC_SCRIPTCAMERA_H

#include "ac/dynobj/cc_agsdynamicobject.h"

// ScriptCamera keeps a reference to actual room Camera in script.
struct ScriptCamera final : AGSCCDynamicObject
{
public:
    ScriptCamera(int id);

    // Get camera index; negative means the camera was deleted
    int GetID() const { return _id; }
    void SetID(int id) { _id = id; }
    // Reset camera index to indicate that this reference is no longer valid
    void Invalidate() { _id = -1; }

    const char *GetType() override;
    int Dispose(void *address, bool force) override;
    void Unserialize(int index, AGS::Common::Stream *in, size_t data_sz) override;

protected:
    // Calculate and return required space for serialization, in bytes
    size_t CalcSerializeSize(const void *address) override;
    // Write object data into the provided stream
    void Serialize(const void *address, AGS::Common::Stream *out) override;

private:
    int _id = -1; // index of camera in the game state array
};

// Unserialize camera from the memory stream
ScriptCamera *Camera_Unserialize(int handle, AGS::Common::Stream *in, size_t data_sz);

#endif // __AC_SCRIPTCAMERA_H
