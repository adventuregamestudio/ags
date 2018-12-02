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

enum ScriptCameraSaveFlags
{
    kScCamPosLocked = 0x01
};

const char *ScriptCamera::GetType()
{
    return "Camera";
}

int ScriptCamera::Serialize(const char *address, char *buffer, int bufsize)
{
    const RoomCamera &cam = play.GetRoomCameraObj();

    StartSerialize(buffer);
    SerializeInt(0); // ID
    SerializeInt(cam.Locked ? kScCamPosLocked : 0); // flags
    SerializeInt(cam.Position.Left);
    SerializeInt(cam.Position.Top);
    SerializeInt(cam.Position.GetWidth());
    SerializeInt(cam.Position.GetHeight());
    SerializeFloat(cam.ScaleX);
    SerializeFloat(cam.ScaleY);
    return EndSerialize();
}

void ScriptCamera::Unserialize(int index, const char *serializedData, int dataSize)
{
    StartUnserialize(serializedData, dataSize);
    UnserializeInt(); // ID, reserved for the future
    int flags = UnserializeInt();
    int x = UnserializeInt();
    int y = UnserializeInt();
    int w = UnserializeInt();
    int h = UnserializeInt();
    float scalex = UnserializeFloat();
    float scaley = UnserializeFloat();
    if (scalex >= 0.f && scaley >= 0.f)
        play.SetRoomCameraAutoSize(scalex, scaley);
    else if (w > 0 && h > 0)
        play.SetRoomCameraSize(Size(w, h));
    else
        play.SetRoomCameraAutoSize(1.f, 1.f);
    if (flags & kScCamPosLocked)
        play.LockRoomCameraAt(x, y);
    else
        play.SetRoomCameraAt(x, y);
    ccRegisterUnserializedObject(index, this, this);
}
