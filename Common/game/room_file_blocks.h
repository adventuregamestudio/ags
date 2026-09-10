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
//
// Provides helper classes and functions for reading and writing block data
// from and into the room file.
//
//=============================================================================
#ifndef __AGS_CN_GAME_ROOMFILEBLOCKS_H
#define __AGS_CN_GAME_ROOMFILEBLOCKS_H

#include <functional>
#include "data/data_ext.h"
#include "game/roomdata.h"
#include "game/room_file.h"

namespace AGS
{
namespace Common
{

// RoomBlockParser allows to manually retrieve room data block by block.
class RoomBlockParser : public DataExtParser
{
public:
    RoomBlockParser(std::unique_ptr<Stream> &&in, RoomFileVersion data_ver);

    virtual String GetOldBlockName(int block_id) const;

private:
    static int GetDataExtFlags(RoomFileVersion data_ver)
    {
        return kDataExt_NumID8 | ((data_ver < kRoomVersion_350) ? kDataExt_File32 : kDataExt_File64);
    }
};

// Type of function that reads (or skips) a single room block
typedef std::function<HError(RoomData *room, RoomDataAux *room_aux, Stream *in, RoomFileBlock block, const String &ext_id,
    soff_t block_len, RoomFileVersion data_ver, const RoomReadOptions &room_read_opts)> PfnReadRoomBlock;

// RoomBlockReader reads whole room data, block by block
class RoomBlockReader : public DataExtReader
{
public:
    RoomBlockReader(RoomData *room, RoomFileVersion data_ver, std::unique_ptr<Stream> &&in,
        const RoomReadOptions &read_opts, PfnReadRoomBlock pfn_read = nullptr);
    RoomBlockReader(RoomData *room, RoomDataAux *room_aux, RoomFileVersion data_ver, std::unique_ptr<Stream> &&in,
        const RoomReadOptions &read_opts, PfnReadRoomBlock pfn_read = nullptr);

private:
    static int GetDataExtFlags(RoomFileVersion data_ver, const RoomReadOptions &read_opts)
    {
        return kDataExt_NumID8 | ((data_ver < kRoomVersion_350) ? kDataExt_File32 : kDataExt_File64)
            | (kDataExt_IgnoreUnread * (read_opts.PartialRead | read_opts.SkipImageData));
    }

    String GetOldBlockName(int block_id) const override;
    HError ReadBlock(Stream *in, int block_id, const String &ext_id,
        soff_t block_len, bool &read_next) override;

    RoomData *_room {};
    RoomDataAux *_roomAux {};
    RoomFileVersion _dataVer {};
    RoomReadOptions _roomReadOpts;
    PfnReadRoomBlock _readFn;
};


//
// TODO: RoomBlockWriter to pair the RoomBlockReader
// 
// Type of function that writes single room block.
typedef std::function<void(const RoomData *room, Stream *out)> PfnWriteRoomBlock;
// Helper for new-style blocks with string id
void WriteRoomBlock(const RoomData *room, const String &ext_id, PfnWriteRoomBlock writer, Stream *out);
void WriteRoomBlock(const String &ext_id, const std::vector<uint8_t> &data, Stream *out);
// Helper for old-style blocks with only numeric id
void WriteRoomBlock(const RoomData *room, RoomFileBlock block, PfnWriteRoomBlock writer, Stream *out);
void WriteRoomBlock(RoomFileBlock block, const std::vector<uint8_t> &data, Stream *out);

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_GAME_ROOMFILEBLOCKS_H
