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
#include "ac/dynobj/scriptoverlay.h"
#include "ac/common.h"
#include "ac/overlay.h"
#include "ac/runtime_defines.h"
#include "ac/screenoverlay.h"
#include "debug/debug_log.h"
#include "util/stream.h"

using namespace AGS::Common;

int ScriptOverlay::Dispose(const char* /*address*/, bool force) 
{
    // since the managed object is being deleted, remove the
    // reference so it doesn't try and dispose something else
    // with that handle later
    int overlayIndex = find_overlay_of_type(overlayId);
    if (overlayIndex >= 0)
    {
        screenover[overlayIndex].associatedOverlayHandle = 0;
    }

    // if this is being removed voluntarily (ie. pointer out of
    // scope) then remove the associateed overlay
    // Otherwise, it's a Restore Game or something so don't
    if ((!force) && (Overlay_GetValid(this)))
    {
        Remove();
    }

    delete this;
    return 1;
}

const char *ScriptOverlay::GetType() {
    return "Overlay";
}

size_t ScriptOverlay::CalcSerializeSize()
{
    return sizeof(int32_t) * 4;
}

void ScriptOverlay::Serialize(const char* /*address*/, Stream *out) {
    out->WriteInt32(overlayId);
    out->WriteInt32(0); // unused (was text window x padding)
    out->WriteInt32(0); // unused (was text window y padding)
    out->WriteInt32(0); // unused (was internal ref flag)
}

void ScriptOverlay::Unserialize(int index, Stream *in, size_t /*data_sz*/) {
    overlayId = in->ReadInt32();
    in->ReadInt32(); // unused (was text window x padding)
    in->ReadInt32(); // unused (was text window y padding)
    in->ReadInt32(); // unused (was internal ref flag)
    ccRegisterUnserializedObject(index, this, this);
}

void ScriptOverlay::Remove() 
{
    int overlayIndex = find_overlay_of_type(overlayId);
    if (overlayIndex < 0)
    {
        debug_script_warn("Overlay.Remove: overlay is invalid, could have been removed earlier.");
        return;
    }
    remove_screen_overlay_index(overlayIndex);
    overlayId = -1;
}
