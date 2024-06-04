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
#include "ac/dynobj/scriptpathfinder.h"
#include "ac/dynobj/dynobj_manager.h"
#include "ac/room.h"
#include "ac/spritecache.h"
#include "ac/walkablearea.h"
#include "util/stream.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern SpriteCache spriteset;

//-----------------------------------------------------------------------------
// ScriptMaskPathfinder
//-----------------------------------------------------------------------------

ScriptMaskPathfinder *ScriptMaskPathfinder::CreateFromMaskSprite(int mask_sprite)
{
    ScriptMaskPathfinder *pathfind = new ScriptMaskPathfinder();
    pathfind->_finder = Pathfinding::CreateDefaultMaskPathfinder();
    if (mask_sprite > 0)
        pathfind->_finder->SetWalkableArea(spriteset[mask_sprite]);
    ccRegisterManagedObject(pathfind, pathfind);
    return pathfind;
}

IRouteFinder *ScriptMaskPathfinder::GetRouteFinder()
{
    return _finder.get();
}

void ScriptMaskPathfinder::SyncPathfinder()
{
    _finder->SetWalkableArea((_maskSprite > 0) ? spriteset[_maskSprite] : nullptr);
}

void ScriptMaskPathfinder::SetMaskSprite(int mask_sprite)
{
    _finder->SetWalkableArea((_maskSprite > 0) ? spriteset[_maskSprite] : nullptr);
}

const char *ScriptMaskPathfinder::GetType()
{
    return "MaskPathfinder";
}

int ScriptMaskPathfinder::Dispose(void *address, bool force)
{
    delete this;
    return 1;
}

void ScriptMaskPathfinder::Unserialize(int index, Stream *in, size_t data_sz)
{
    _maskSprite = in->ReadInt32();
    _finder = Pathfinding::CreateDefaultMaskPathfinder();
    if (_maskSprite > 0)
        _finder->SetWalkableArea(spriteset[_maskSprite]);
    ccRegisterUnserializedObject(index, this, this);
}

size_t ScriptMaskPathfinder::CalcSerializeSize(const void *address)
{
    return sizeof(uint32_t) * 1;
}

void ScriptMaskPathfinder::Serialize(const void *address, Stream *out)
{
    out->WriteInt32(_maskSprite);
}

//-----------------------------------------------------------------------------
// RoomPathfinder
//-----------------------------------------------------------------------------

RoomPathfinder *RoomPathfinder::Create()
{
    RoomPathfinder *pathfind = new RoomPathfinder();
    pathfind->_finder = get_room_pathfinder();
    ccRegisterManagedObject(pathfind, pathfind);
    return pathfind;
}

IRouteFinder *RoomPathfinder::GetRouteFinder()
{
    return _finder;
}

void RoomPathfinder::SyncPathfinder()
{
    _finder->SetWalkableArea(prepare_walkable_areas(-1));
}

const char *RoomPathfinder::GetType()
{
    return "RoomPathfinder";
}

int RoomPathfinder::Dispose(void *address, bool force)
{
    delete this;
    return 1;
}

void RoomPathfinder::Unserialize(int index, AGS::Common::Stream *in, size_t data_sz)
{
    _finder = get_room_pathfinder();
    ccRegisterUnserializedObject(index, this, this);
}

size_t RoomPathfinder::CalcSerializeSize(const void *address)
{
    return 0u;
}

void RoomPathfinder::Serialize(const void *address, AGS::Common::Stream *out)
{
}
