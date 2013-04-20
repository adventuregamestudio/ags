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
//
// RoomState, a class of dynamic room data
//
//=============================================================================
#ifndef __AGS_EE_GAME__ROOMSTATE_H
#define __AGS_EE_GAME__ROOMSTATE_H

#include "core/types.h"
#include "ac/interaction.h"
#include "game/roomobject.h"
#include "util/array.h"

namespace AGS
{

namespace Common { class Stream; }

namespace Engine
{

using Common::Array;
using Common::Stream;

struct RoomHotspot
{
    bool            Enabled;
    NewInteraction  Interaction;
};

struct RoomRegion
{
    bool            Enabled;
    NewInteraction  Interaction;
};

struct WalkBehind
{
    int16_t Baseline;
};

class RoomState
{
public:
    RoomState();
    ~RoomState();

    void Free();

    void ReadFromFile_v321(Stream *in);
    void WriteToFile_v321(Stream *out);

private:
    void InitDefaults();
    void ReadRoomObjects_Aligned(Stream *in);
    void WriteRoomObjects_Aligned(Stream *out);

    // TODO: all members are currently public; hide them later
public:
    bool                BeenHere;
    
    Array<RoomHotspot>  Hotspots;
    Array<RoomObject>   Objects;
    Array<RoomRegion>   Regions;
    Array<WalkBehind>   WalkBehinds;

    NewInteraction      Interaction;
    Array<int32_t>      InteractionVariableValues;

    Array<char>         ScriptData;    

    // TODO: remove these, after refactoring the game load process
    // and making separate structs for every room entity type
    int32_t             ObjectCount;
    int32_t             ScriptDataSize;
};

// Get room state by index; returns NULL if room state was not created
// (meaning room was never loaded yet)
RoomState* GetRoomState(int room);
// Used in places where it is only important to know whether the player
// had previously entered the room. In this case it is not necessary
// to initialise the status because a player can only have been in
// a room if the status is already initialised.
bool IsRoomStateValid(int room);
void ResetRoomStates();

} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_GAME__ROOMSTATE_H
