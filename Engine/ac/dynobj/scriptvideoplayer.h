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
#ifndef __AC_SCRIPTVIDEOPLAYER_H
#define __AC_SCRIPTVIDEOPLAYER_H

#include "ac/dynobj/cc_agsdynamicobject.h"

struct ScriptVideoPlayer final : AGSCCDynamicObject
{
public:
    ScriptVideoPlayer(int id) : _id(id) {}
    // Get videoplayer's index; negative means the video was removed
    int GetID() const { return _id; }
    // Reset videoplayer's index to indicate that this reference is no longer valid
    void Invalidate() { _id = -1; }

    const char *GetType() override;
    int Dispose(void *address, bool force) override;
    void Unserialize(int index, AGS::Common::Stream *in, size_t data_sz) override;

private:
    // Calculate and return required space for serialization, in bytes
    size_t CalcSerializeSize(const void *address) override;
    // Write object data into the provided stream
    void Serialize(const void *address, AGS::Common::Stream *out) override;

    int _id = -1; // index of videoplayer in the video subsystem
};

#endif // __AC_SCRIPTVIDEOPLAYER_H
