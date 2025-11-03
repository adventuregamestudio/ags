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
#ifndef __AC_SCRIPTOVERLAY_H
#define __AC_SCRIPTOVERLAY_H

#include "ac/dynobj/cc_agsdynamicobject.h"

struct ScriptOverlay : AGSCCDynamicObject
{
public:
    ScriptOverlay() = default;
    ScriptOverlay(int over_id) : _overlayID(over_id) {}
    const char *GetType() override;
    int Dispose(void *address, bool force) override;
    void Unserialize(int index, AGS::Common::Stream *in, size_t data_sz) override;
    void Remove();

    int GetOverlayID() const { return _overlayID; }
    void Invalidate() { _overlayID = -1; }

protected:
    // Calculate and return required space for serialization, in bytes
    size_t CalcSerializeSize(const void *address) override;
    // Write object data into the provided stream
    void Serialize(const void *address, AGS::Common::Stream *out) override;

    int _overlayID = -1;
};

struct ScriptAnimatedOverlay final : ScriptOverlay
{
public:
    ScriptAnimatedOverlay() = default;
    ScriptAnimatedOverlay(int over_id) : ScriptOverlay(over_id) {}
    const char *GetType() override;
    int Dispose(void *address, bool force) override;
};

#endif // __AC_SCRIPTOVERLAY_H
