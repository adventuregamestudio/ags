//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "game/room_file_blocks.h"

namespace AGS
{
namespace Common
{

//
// RoomBlockParser
//

RoomBlockParser::RoomBlockParser(std::unique_ptr<Stream> &&in, RoomFileVersion data_ver)
    : DataExtParser(std::move(in), GetDataExtFlags(data_ver))
{
}

String RoomBlockParser::GetOldBlockName(int block_id) const
{
    return GetRoomBlockName((RoomFileBlock)block_id);
}

// Implemented in room_file.cpp
HError ReadRoomBlock(RoomData *room, RoomDataAux *room_aux, Stream *in, RoomFileBlock block, const String &ext_id,
    soff_t block_len, RoomFileVersion data_ver, const RoomReadOptions &read_opts);

//
// RoomBlockReader
//

RoomBlockReader::RoomBlockReader(RoomData *room, RoomFileVersion data_ver, std::unique_ptr<Stream> &&in,
        const RoomReadOptions &read_opts, PfnReadRoomBlock pfn_read)
    : DataExtReader(std::move(in), GetDataExtFlags(data_ver, read_opts))
    , _room(room)
    , _roomAux(nullptr)
    , _dataVer(data_ver)
    , _roomReadOpts(read_opts)
    , _readFn(pfn_read ? pfn_read : ReadRoomBlock)
{}

RoomBlockReader::RoomBlockReader(RoomData *room, RoomDataAux *room_aux, RoomFileVersion data_ver, std::unique_ptr<Stream> &&in,
    const RoomReadOptions &read_opts, PfnReadRoomBlock pfn_read)
    : DataExtReader(std::move(in), GetDataExtFlags(data_ver, read_opts))
    , _room(room)
    , _roomAux(room_aux)
    , _dataVer(data_ver)
    , _roomReadOpts(read_opts)
    , _readFn(pfn_read ? pfn_read : ReadRoomBlock)
{}

String RoomBlockReader::GetOldBlockName(int block_id) const
{
    return GetRoomBlockName((RoomFileBlock)block_id);
}

HError RoomBlockReader::ReadBlock(Stream *in, int block_id, const String &ext_id, soff_t block_len, bool &read_next)
{
    read_next = true;
    return _readFn(_room, _roomAux, in, (RoomFileBlock)block_id, ext_id, block_len, _dataVer, _roomReadOpts);
}

//
// Room block writer
//

void WriteRoomBlock(const RoomData *room, const String &ext_id, PfnWriteRoomBlock writer, Stream *out)
{
    WriteExtBlock(ext_id, [room, writer](Stream *out) { writer(room, out); },
        kDataExt_NumID8 | kDataExt_File64, out);
}

void WriteRoomBlock(const String &ext_id, const std::vector<uint8_t> &data, Stream *out)
{
    WriteExtBlock(ext_id, data, kDataExt_NumID8 | kDataExt_File64, out);
}

void WriteRoomBlock(const RoomData *room, RoomFileBlock block, PfnWriteRoomBlock writer, Stream *out)
{
    WriteExtBlock(block, [room, writer](Stream *out) { writer(room, out); },
        kDataExt_NumID8 | kDataExt_File64, out);
}

void WriteRoomBlock(RoomFileBlock block, const std::vector<uint8_t> &data, Stream *out)
{
    WriteExtBlock(block, data, kDataExt_NumID8 | kDataExt_File64, out);
}

} // namespace Common
} // namespace AGS
