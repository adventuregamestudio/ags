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
#include "gfx/gfx_def.h"

using namespace AGS::Common;

extern RoomStruct thisroom;
extern SpriteCache spriteset;
extern GameSetupStruct game;

Bitmap *ScriptDrawingSurface::GetBitmapSurface()
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

void ScriptDrawingSurface::SetDrawingColor(int color_number)
{
    _currentScriptColor = color_number;
    Bitmap *ds = GetBitmapSurface();
    if (color_number == SCR_COLOR_TRANSPARENT)
    {
        _currentColor = ds->GetMaskColor();
    }
    else
    {
        _currentColor = ds->GetCompatibleColor(color_number);
    }
}

void ScriptDrawingSurface::SetBlendMode(BlendMode blend_mode)
{
    _currentBlendMode = blend_mode;
}

Bitmap *ScriptDrawingSurface::StartDrawing()
{
    return GetBitmapSurface();
}

Bitmap *ScriptDrawingSurface::StartDrawingWithBrush()
{
    Bitmap *ds = GetBitmapSurface();
    _alphaBlending = (ds->GetColorDepth() == 32) && (_currentBlendMode != kBlend_Copy)
        && ((geta32(_currentColor) != 0xFF) || (_currentBlendMode != kBlend_Normal));
    if (_alphaBlending)
    {
        drawing_mode(DRAW_MODE_TRANS, nullptr, 0, 0);
        SetBlender(_currentBlendMode, 0xFF);
    }
    return ds;
}

Bitmap *ScriptDrawingSurface::StartDrawingReadOnly()
{
    return GetBitmapSurface();
}

void ScriptDrawingSurface::FinishedDrawing()
{
    modified = true;
}

void ScriptDrawingSurface::FinishedDrawingWithBrush()
{
    if (_alphaBlending)
    {
        drawing_mode(DRAW_MODE_SOLID, nullptr, 0, 0);
    }
    modified = true;
}

void ScriptDrawingSurface::FinishedDrawingReadOnly()
{
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

void ScriptDrawingSurface::Serialize(const void* /*address*/, Stream *out)
{
    // pack mask type in the last byte of a negative integer
    // note: (-1) is reserved for "unused", for backward compatibility
    if (roomMaskType > 0)
        out->WriteInt32(0xFFFFFF00 | roomMaskType);
    else
        out->WriteInt32(roomBackgroundNumber);
    out->WriteInt32(dynamicSpriteNumber);
    out->WriteInt32(dynamicSurfaceNumber);
    out->WriteInt32(_currentColor);
    out->WriteInt32(_currentScriptColor);
    // NOTE: could turn any of the boolean fields below into a bit flag set
    out->WriteInt32(0); // unused, was highResCoordinates
    out->WriteInt32(modified ? 1 : 0);
    out->WriteInt32(0); // unused, was hasAlphaChannel
    out->WriteInt32(isLinkedBitmapOnly ? 1 : 0); // NOTE: could turn this into bit flag set
    // version data sz: 36
    out->WriteInt32(_currentBlendMode);
    out->WriteInt32(0); // reserve to 4 ints
    out->WriteInt32(0);
    out->WriteInt32(0);
}

void ScriptDrawingSurface::Unserialize(int index, Stream *in, size_t data_sz)
{
    int room_ds = in->ReadInt32();
    if (room_ds >= 0)
        roomBackgroundNumber = room_ds;
    // negative value may contain a mask type
    else if ((room_ds & 0xFF) != 0xFF)
        roomMaskType = (RoomAreaMask)(room_ds & 0xFF);
    dynamicSpriteNumber = in->ReadInt32();
    dynamicSurfaceNumber = in->ReadInt32();
    _currentColor = in->ReadInt32();
    _currentScriptColor = in->ReadInt32();
    in->ReadInt32(); // unused, was highResCoordinates
    modified = (in->ReadInt32() != 0);
    in->ReadInt32(); // unused, was hasAlphaChannel
    isLinkedBitmapOnly = (in->ReadInt32() != 0);
    if (data_sz > 36)
    {
        _currentBlendMode = (BlendMode)in->ReadInt32();
    }
    _alphaBlending = false;
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
    modified = false;
    _alphaBlending = false;
    _currentColor = 0;
    _currentScriptColor = SCR_COLOR_TRANSPARENT;
    _currentBlendMode = kBlend_Normal;
}
