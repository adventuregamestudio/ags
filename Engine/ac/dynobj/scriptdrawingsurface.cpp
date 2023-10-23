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
#include "ac/dynobj/scriptdrawingsurface.h"
#include "ac/common.h"
#include "ac/draw.h"
#include "ac/drawingsurface.h"
#include "ac/gamestate.h"
#include "ac/gamesetupstruct.h"
#include "ac/spritecache.h"
#include "ac/runtime_defines.h"
#include "ac/dynobj/dynobj_manager.h"
#include "game/roomstruct.h"
#include "gfx/bitmap.h"

using namespace AGS::Common;

extern RoomStruct thisroom;
extern SpriteCache spriteset;
extern GameState play;
extern GameSetupStruct game;

Bitmap* ScriptDrawingSurface::GetBitmapSurface()
{
    // TODO: consider creating weak_ptr here, and store one in the DrawingSurface!
    if (roomBackgroundNumber >= 0)
        return thisroom.BgFrames[roomBackgroundNumber].Graphic.get();
    else if (dynamicSpriteNumber >= 0)
        return spriteset[dynamicSpriteNumber];
    else if (dynamicSurfaceNumber >= 0)
        return dynamicallyCreatedSurfaces[dynamicSurfaceNumber].get();
    else if (linkedBitmapOnly != nullptr)
        return linkedBitmapOnly;
    else if (roomMaskType > kRoomAreaNone)
        return thisroom.GetMask(roomMaskType);
    quit("!DrawingSurface: attempted to use surface after Release was called");
    return nullptr;
}

Bitmap *ScriptDrawingSurface::StartDrawing()
{
    return this->GetBitmapSurface();
}

void ScriptDrawingSurface::FinishedDrawingReadOnly()
{
}

void ScriptDrawingSurface::FinishedDrawing()
{
    FinishedDrawingReadOnly();
    modified = 1;
}

int ScriptDrawingSurface::Dispose(void* /*address*/, bool /*force*/) {

    // dispose the drawing surface
    DrawingSurface_Release(this);
    delete this;
    return 1;
}

const char *ScriptDrawingSurface::GetType() {
    return "DrawingSurface";
}

size_t ScriptDrawingSurface::CalcSerializeSize(const void* /*address*/)
{
    return sizeof(int32_t) * 9;
}

void ScriptDrawingSurface::Serialize(const void* /*address*/, Stream *out) {
    // pack mask type in the last byte of a negative integer
    // note: (-1) is reserved for "unused", for backward compatibility
    if (roomMaskType > 0)
        out->WriteInt32(0xFFFFFF00 | roomMaskType);
    else
        out->WriteInt32(roomBackgroundNumber);
    out->WriteInt32(dynamicSpriteNumber);
    out->WriteInt32(dynamicSurfaceNumber);
    out->WriteInt32(currentColour);
    out->WriteInt32(currentColourScript);
    out->WriteInt32(0); // unused, was highResCoordinates
    out->WriteInt32(modified);
    out->WriteInt32(0); // unused, was hasAlphaChannel
    out->WriteInt32(isLinkedBitmapOnly ? 1 : 0);
}

void ScriptDrawingSurface::Unserialize(int index, Stream *in, size_t /*data_sz*/) {
    int room_ds = in->ReadInt32();
    if (room_ds >= 0)
        roomBackgroundNumber = room_ds;
    // negative value may contain a mask type
    else if ((room_ds & 0xFF) != 0xFF)
        roomMaskType = (RoomAreaMask)(room_ds & 0xFF);
    dynamicSpriteNumber = in->ReadInt32();
    dynamicSurfaceNumber = in->ReadInt32();
    currentColour = in->ReadInt32();
    currentColourScript = in->ReadInt32();
    in->ReadInt32(); // unused, was highResCoordinates
    modified = in->ReadInt32();
    in->ReadInt32(); // unused, was hasAlphaChannel
    isLinkedBitmapOnly = (in->ReadInt32() != 0);
    ccRegisterUnserializedObject(index, this, this);
}

ScriptDrawingSurface::ScriptDrawingSurface() 
{
    roomBackgroundNumber = -1;
    roomMaskType = kRoomAreaNone;
    dynamicSpriteNumber = -1;
    dynamicSurfaceNumber = -1;
    isLinkedBitmapOnly = false;
    linkedBitmapOnly = nullptr;
    currentColour = play.raw_color;
    currentColourScript = 0;
    modified = 0;
}
