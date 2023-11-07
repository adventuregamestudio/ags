//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2023 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "data/room_utils.h"
#include "util/string_utils.h"

#define MIN_ROOM_HOTSPOTS  20

using namespace AGS::Common;

namespace AGS
{
namespace DataUtil
{

HError ReadFromMainBlock(RoomScNames &data, Stream *in, RoomFileVersion data_ver, soff_t block_len)
{
    soff_t start_pos = in->GetPosition();
    in->ReadInt32(); // BackgroundBPP
    size_t wbh_count = in->ReadInt16(); // WalkBehindCount
                                        // Walk-behinds baselines
    for (size_t i = 0; i < wbh_count; ++i)
        in->ReadInt16();
    size_t hot_count = in->ReadInt32();
    if (hot_count == 0)
        hot_count = MIN_ROOM_HOTSPOTS;
    // Hotspots walk-to points
    for (size_t i = 0; i < hot_count; ++i)
    {
        in->ReadInt16(); // WalkTo.X
        in->ReadInt16(); // WalkTo.Y
    }
    // Hotspots names
    for (size_t i = 0; i < hot_count; ++i)
    {
        StrUtil::ReadString(in);
    }
    // Hotspot script names
    data.HotspotNames.resize(hot_count);
    for (size_t i = 0; i < hot_count; ++i)
    {
        data.HotspotNames[i] = StrUtil::ReadString(in);
    }
    // Skip the rest
    in->Seek(start_pos + block_len, kSeekBegin);
    return HError::None();
}

HError ReadObjScNamesBlock(RoomScNames &data, Stream *in, RoomFileVersion data_ver)
{
    size_t obj_count = in->ReadByte();
    data.ObjectNames.resize(obj_count);
    for (size_t i = 0; i < obj_count; ++i)
    {
        data.ObjectNames[i] = StrUtil::ReadString(in);
    }
    return HError::None();
}

HError ReadRoomScNames(RoomScNames &data, Stream *in, RoomFileBlock block, const String &ext_id,
    soff_t block_len, RoomFileVersion data_ver)
{
    switch (block)
    {
    case kRoomFblk_Main:
        return ReadFromMainBlock(data, in, data_ver, block_len);
    case kRoomFblk_ObjectScNames:
        return ReadObjScNamesBlock(data, in, data_ver);
    default:
        in->Seek(block_len); // skip block
        return HError::None();
    }
}

} // namespace DataUtil
} // namespace AGS
