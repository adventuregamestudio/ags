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
#include "ac/dynobj/scriptcamera.h"
#include "ac/dynobj/dynobj_manager.h"
#include "ac/gamestate.h"
#include "util/stream.h"

using namespace AGS::Common;

ScriptCamera::ScriptCamera(int id) : _id(id) {}

const char *ScriptCamera::GetType()
{
    return "Camera2";
}

int ScriptCamera::Dispose(void* /*address*/, bool /*force*/)
{
    // Note that ScriptCamera is a reference to actual Camera object,
    // and this deletes the reference, while camera may remain in GameState.
    delete this;
    return 1;
}

size_t ScriptCamera::CalcSerializeSize(const void* /*address*/)
{
    return sizeof(int32_t);
}

void ScriptCamera::Serialize(const void* /*address*/, Stream *out)
{
    out->WriteInt32(_id);
}

void ScriptCamera::Unserialize(int index, Stream *in, size_t /*data_sz*/)
{
    _id = in->ReadInt32();
    ccRegisterUnserializedObject(index, this, this);
}

ScriptCamera *Camera_Unserialize(int handle, Stream *in, size_t /*data_sz*/)
{
    // The way it works now, we must not create a new script object,
    // but acquire one from the GameState, which keeps the first reference.
    // This is essential because GameState should be able to invalidate any
    // script references when Camera gets removed.
    const int id = in->ReadInt32();
    if (id >= 0)
    {
        auto scam = play.RegisterRoomCamera(id, handle);
        if (scam)
            return scam;
    }
    return new ScriptCamera(-1); // make invalid reference
}
