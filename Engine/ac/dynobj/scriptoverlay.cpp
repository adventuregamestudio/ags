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
#include "ac/dynobj/scriptoverlay.h"
#include "ac/dynobj/dynobj_manager.h"
#include "ac/common.h"
#include "ac/overlay.h"
#include "ac/runtime_defines.h"
#include "ac/screenoverlay.h"
#include "debug/debug_log.h"
#include "util/stream.h"

using namespace AGS::Common;

const char *ScriptOverlay::GetType()
{
    return "Overlay";
}

int ScriptOverlay::Dispose(void* /*address*/, bool force)
{
    // since the managed object is being deleted, remove the
    // reference so it doesn't try and dispose something else
    // with that handle later
    if (_overlayID >= 0)
    {
        auto *over = get_overlay(_overlayID);
        if (over)
        {
            over->DetachScriptObject();
        }
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

size_t ScriptOverlay::CalcSerializeSize(const void* /*address*/)
{
    return sizeof(int32_t);
}

void ScriptOverlay::Serialize(const void* /*address*/, Stream *out)
{
    out->WriteInt32(_overlayID);
}

void ScriptOverlay::Unserialize(int index, Stream *in, size_t /*data_sz*/)
{
    _overlayID = in->ReadInt32();
    // NOTE: some older formats had 3 more int32 here, which we exclude now
    ccRegisterUnserializedObject(index, this, this);
}

void ScriptOverlay::Remove()
{
    if (_overlayID < 0)
    {
        debug_script_warn("Overlay.Remove: overlay is invalid, could have been removed earlier.");
        return;
    }
    remove_screen_overlay(_overlayID);
    _overlayID = -1;
}

const char *ScriptAnimatedOverlay::GetType()
{
    return "AnimatedOverlay";
}

int ScriptAnimatedOverlay::Dispose(void *address, bool force)
{
    if (!force && _overlayID >= 0)
    {
        RemoveAnimatedOverlay(_overlayID);
    }

    return ScriptOverlay::Dispose(address, force);
}
