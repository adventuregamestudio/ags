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
#include "game/room_file.h"
#include "debug/out.h"
#include "util/data_ext.h"
#include "util/file.h"

namespace AGS
{
namespace Common
{

RoomDataSource::RoomDataSource()
    : DataVersion(kRoomVersion_Undefined)
{
}

String GetRoomFileErrorText(RoomFileErrorType err)
{
    switch (err)
    {
    case kRoomFileErr_NoError:
        return "No error.";
    case kRoomFileErr_FileOpenFailed:
        return "Room file was not found or could not be opened.";
    case kRoomFileErr_FormatNotSupported:
        return "Format version not supported.";
    case kRoomFileErr_BlockListFailed:
        return "There was error reading room data.";
    case kRoomFileErr_UnknownBlockType:
        return "Unknown block type.";
    case kRoomFileErr_OldBlockNotSupported:
        return "Block type is too old and not supported by this version of the engine.";
    case kRoomFileErr_IncompatibleEngine:
        return "This engine cannot handle requested room content.";
    case kRoomFileErr_ScriptLoadFailed:
        return "Script load failed.";
    case kRoomFileErr_InconsistentData:
        return "Inconsistent room data, or file is corrupted.";
    case kRoomFileErr_PropertiesBlockFormat:
        return "Unknown format of the custom properties block.";
    case kRoomFileErr_InvalidPropertyValues:
        return "Errors encountered when reading custom properties.";
    case kRoomFileErr_BlockNotFound:
        return "Required block was not found.";
    }
    return "Unknown error.";
}

HRoomFileError OpenRoomFile(const String &filename, RoomDataSource &src)
{
    // Cleanup source struct
    src = RoomDataSource();
    // Try to open room file
    Stream *in = File::OpenFileRead(filename);
    if (in == nullptr)
        return new RoomFileError(kRoomFileErr_FileOpenFailed, String::FromFormat("Filename: %s.", filename.GetCStr()));
    src.Filename = filename;
    src.InputStream.reset(in);
    return ReadRoomHeader(src);
}

// Read room data header and check that we support this format
HRoomFileError ReadRoomHeader(RoomDataSource &src)
{
    src.DataVersion = (RoomFileVersion)src.InputStream->ReadInt16();
    if (src.DataVersion < kRoomVersion_303b || src.DataVersion > kRoomVersion_Current)
        return new RoomFileError(kRoomFileErr_FormatNotSupported, String::FromFormat("Required format version: %d, supported %d - %d", src.DataVersion, kRoomVersion_250b, kRoomVersion_Current));
    return HRoomFileError::None();
}

String GetRoomBlockName(RoomFileBlock id)
{
    switch (id)
    {
    case kRoomFblk_Main: return "Main";
    case kRoomFblk_Script: return "TextScript";
    case kRoomFblk_CompScript: return "CompScript";
    case kRoomFblk_CompScript2: return "CompScript2";
    case kRoomFblk_ObjectNames: return "ObjNames";
    case kRoomFblk_AnimBg: return "AnimBg";
    case kRoomFblk_CompScript3: return "CompScript3";
    case kRoomFblk_Properties: return "Properties";
    case kRoomFblk_ObjectScNames: return "ObjScNames";
    }
    return "unknown";
}

HRoomFileError ReadRoomData(PfnReadRoomBlock reader, Stream *in, RoomFileVersion data_ver)
{
    // This reader will delegate block reading to the provided user function
    auto ex_reader = [reader, data_ver](Stream *in, int block_id, const String &ext_id,
        soff_t block_len, bool &read_next)
    { return reader(in, (RoomFileBlock)block_id, ext_id, block_len, data_ver, read_next); };

    HError err = ReadExtData(ex_reader,
        kDataExt_NumID8 | ((data_ver < kRoomVersion_350) ? kDataExt_File32 : kDataExt_File64), in);
    return err ? HRoomFileError::None() : new RoomFileError(kRoomFileErr_BlockListFailed, err);
}

// Helper for new-style blocks with string id
void WriteRoomBlock(const RoomStruct *room, const String &ext_id, PfnWriteRoomBlock writer, Stream *out)
{
    WriteExtBlock(ext_id, [room, writer](Stream *out) { writer(room, out); },
        kDataExt_NumID8 | kDataExt_File64, out);
}

// Helper for old-style blocks with only numeric id
void WriteRoomBlock(const RoomStruct *room, RoomFileBlock block, PfnWriteRoomBlock writer, Stream *out)
{
    WriteExtBlock(block, [room, writer](Stream *out) { writer(room, out); },
        kDataExt_NumID8 | kDataExt_File64, out);
}

} // namespace Common
} // namespace AGS
