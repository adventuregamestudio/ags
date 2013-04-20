
#include "ac/game_version.h"
#include "game/game_objects.h"
#include "game/roomstate.h"
#include "util/alignedstream.h"

extern GameDataVersion loaded_game_file_version;

namespace AGS
{
namespace Engine
{

using Common::AlignedStream;
using Common::Stream;

RoomState::RoomState()
{
    InitDefaults();
}

RoomState::~RoomState()
{
}

void RoomState::Free()
{
    BeenHere = false;
    Objects.Free();
    Hotspots.Free();
    Regions.Free();
    WalkBehinds.Free();

    InteractionVariableValues.Free();
    ScriptData.Free();
    
    ObjectCount = 0;
    ScriptDataSize = 0;
}

void RoomState::ReadFromFile_v321(Stream *in)
{
    Free();
    InitDefaults();

    BeenHere = in->ReadInt32() != 0;
    ObjectCount = in->ReadInt32();
    ReadRoomObjects_Aligned(in);
    int16_t flag_states[MAX_FLAGS];
    in->ReadArrayOfInt16(flag_states, MAX_FLAGS);
    ScriptDataSize = in->ReadInt32();
    in->ReadInt32(); // tsdata pointer

    // TODO: use room base to get real objects count
    for (int i = 0; i < MAX_HOTSPOTS; ++i)
    {
        Hotspots[i].Interaction.ReadFromFile(in);
    }
    for (int i = 0; i < MAX_INIT_SPR; ++i)
    {
        Objects[i].Interaction.ReadFromFile(in);
    }
    for (int i = 0; i < MAX_REGIONS; ++i)
    {
        Regions[i].Interaction.ReadFromFile(in);
    }
    Interaction.ReadFromFile(in);
    for (int i = 0; i < MAX_HOTSPOTS; ++i)
    {
        Hotspots[i].Enabled = in->ReadInt8() != 0;
    }
    for (int i = 0; i < MAX_REGIONS; ++i)
    {
        Regions[i].Enabled = in->ReadInt8() != 0;
    }
    for (int i = 0; i < MAX_OBJ; ++i)
    {
        WalkBehinds[i].Baseline = in->ReadInt16();
    }
    InteractionVariableValues.ReadRawOver(in, MAX_GLOBAL_VARIABLES);
}

void RoomState::WriteToFile_v321(Stream *out)
{
    out->WriteInt32(BeenHere ? 1 : 0);
    out->WriteInt32(ObjectCount);
    WriteRoomObjects_Aligned(out);
    int16_t flag_states[MAX_FLAGS];
    memset(flag_states, 0, sizeof(flag_states));
    out->WriteArrayOfInt16(flag_states, MAX_FLAGS);
    out->WriteInt32(ScriptDataSize);
    out->WriteInt32(ScriptDataSize > 0 ? 1 : 0);

    // TODO: write real objects count
    for (int i = 0; i < MAX_HOTSPOTS; ++i)
    {
        Hotspots[i].Interaction.WriteToFile(out);
    }
    for (int i = 0; i < MAX_INIT_SPR; ++i)
    {
        Objects[i].Interaction.WriteToFile(out);
    }
    for (int i = 0; i < MAX_REGIONS; ++i)
    {
        Regions[i].Interaction.WriteToFile(out);
    }
    Interaction.WriteToFile(out);
    for (int i = 0; i < MAX_HOTSPOTS; ++i)
    {
        out->WriteInt8(Hotspots[i].Enabled ? 1 : 0);
    }
    for (int i = 0; i < MAX_REGIONS; ++i)
    {
        out->WriteInt8(Regions[i].Enabled ? 1 : 0);
    }
    for (int i = 0; i < MAX_OBJ; ++i)
    {
        out->WriteInt16(WalkBehinds[i].Baseline);
    }
    InteractionVariableValues.WriteRaw(out);
}

void RoomState::InitDefaults()
{
    BeenHere = false;
    ObjectCount = 0;
    ScriptDataSize = 0;

    // TODO: this is done for safety reasons;
    // should be reworked when all the code is altered to remove any
    // usage of MAX_* constants for room objects and regions.
    Hotspots.New(MAX_HOTSPOTS);
    Objects.New(MAX_INIT_SPR);
    Regions.New(MAX_REGIONS);
    WalkBehinds.New(MAX_OBJ);
    InteractionVariableValues.New(MAX_GLOBAL_VARIABLES);
}

void RoomState::ReadRoomObjects_Aligned(Common::Stream *in)
{
    AlignedStream align_s(in, Common::kAligned_Read);
    // TODO: use room base to get real objects count
    for (int i = 0; i < MAX_INIT_SPR; ++i)
    {
        Objects[i].ReadFromFile(&align_s);
        align_s.Reset();
    }
}

void RoomState::WriteRoomObjects_Aligned(Common::Stream *out)
{
    AlignedStream align_s(out, Common::kAligned_Write);
    // TODO: write real objects count
    for (int i = 0; i < MAX_INIT_SPR; ++i)
    {
        Objects[i].WriteToFile(&align_s);
        align_s.Reset();
    }
}

// Replaces all accesses to the roomstats array
RoomState* GetRoomState(int room)
{
    if (room_statuses[room] == NULL)
    {
        // First access, allocate and initialise the status
        room_statuses[room] = new RoomState();
    }
    return room_statuses[room];
}

// Used in places where it is only important to know whether the player
// had previously entered the room. In this case it is not necessary
// to initialise the status because a player can only have been in
// a room if the status is already initialised.
bool IsRoomStateValid(int room)
{
    return (room_statuses[room] != NULL);
}

void ResetRoomStates()
{
    for (int i = 0; i < MAX_ROOMS; i++)
    {
        if (room_statuses[i] != NULL)
        {
            room_statuses[i]->Free();

            // Don't delete the status on 2.x. The status struct contains NewInteraction
            // pointer that are also referenced in the current room struct.
            // If they are freed here this will lead to an access violation when the
            // room unloading function tries to frees them.
            if (loaded_game_file_version > kGameVersion_272)
            {
                delete room_statuses[i];
                room_statuses[i] = NULL;
            }
        }
    }
}

} // namespace Engine
} // namespace AGS
