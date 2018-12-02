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

#include "ac/dynobj/scriptviewport.h"
#include "ac/gamestate.h"

const char *ScriptViewport::GetType()
{
    return "Viewport";
}

int ScriptViewport::Serialize(const char *address, char *buffer, int bufsize)
{
    const Rect &view = play.GetRoomViewport();

    StartSerialize(buffer);
    SerializeInt(0); // ID
    SerializeInt(view.Left);
    SerializeInt(view.Top);
    SerializeInt(view.GetWidth());
    SerializeInt(view.GetHeight());
    SerializeInt(0); // camera reference (ID)
    return EndSerialize();
}

void ScriptViewport::Unserialize(int index, const char *serializedData, int dataSize)
{
    StartUnserialize(serializedData, dataSize);
    UnserializeInt(); // ID, reserved for the future
    int x = UnserializeInt();
    int y = UnserializeInt();
    int w = UnserializeInt();
    int h = UnserializeInt();
    UnserializeInt(); // camera ID, reserved for the future
    play.SetRoomViewport(RectWH(x, y, w, h));
    ccRegisterUnserializedObject(index, this, this);
}
