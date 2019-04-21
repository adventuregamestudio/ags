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

#include "ac/dynobj/scriptcamera.h"
#include "ac/gamestate.h"

ScriptCamera::ScriptCamera(int id) : _id(id) {}

const char *ScriptCamera::GetType()
{
    return "Camera2";
}

int ScriptCamera::Serialize(const char *address, char *buffer, int bufsize)
{
    StartSerialize(buffer);
    SerializeInt(_id);
    return EndSerialize();
}

void ScriptCamera::Unserialize(int index, const char *serializedData, int dataSize)
{
    StartUnserialize(serializedData, dataSize);
    _id = UnserializeInt();
    ccRegisterUnserializedObject(index, this, this);
}
