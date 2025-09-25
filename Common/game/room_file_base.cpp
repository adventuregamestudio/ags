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
    case kRoomFileErr_SignatureFailed:
        return "Not an AGS room file or an unsupported format.";
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
    default: return "Unknown error.";
    }
}

const String RoomDataSource::Signature = "AGS Room File v2";

HRoomFileError OpenRoomFile(const String &filename, RoomDataSource &src)
{
    // Cleanup source struct
    src = RoomDataSource();
    // Try to open room file
    auto in = File::OpenFileRead(filename);
    if (in == nullptr)
        return new RoomFileError(kRoomFileErr_FileOpenFailed, String::FromFormat("Filename: %s.", filename.GetCStr()));
    src.Filename = filename;
    src.InputStream = std::move(in);
    return ReadRoomHeader(src);
}

// Read room data header and check that we support this format
HRoomFileError ReadRoomHeader(RoomDataSource &src)
{
    Stream *in = src.InputStream.get();
    // Older format started with a 16-bit version number right away.
    // The new format has this field filled with 0xFFFF
    uint16_t old_room_ver = in->ReadInt16();
    if (old_room_ver == 0xFFFF)
    {
        // New format
        String data_sig = String::FromStreamCount(in, RoomDataSource::Signature.GetLength());
        if (data_sig.Compare(RoomDataSource::Signature))
            return new RoomFileError(kRoomFileErr_SignatureFailed);
        src.DataVersion = static_cast<RoomFileVersion>(in->ReadInt32());
        src.CompiledWith = StrUtil::ReadString(in);
    }
    else
    {
        // Old format
        src.DataVersion = static_cast<RoomFileVersion>(old_room_ver);
    }

    if (src.DataVersion < kRoomVersion_LowSupport || src.DataVersion > kRoomVersion_Current)
    {
        return new RoomFileError(kRoomFileErr_FormatNotSupported,
            String::FromFormat("Room was compiled with %s. Required format version: %d, supported %d - %d",
                !src.CompiledWith.IsEmpty() ? src.CompiledWith.GetCStr() : "(unknown)", src.DataVersion, kRoomVersion_LowSupport, kRoomVersion_Current));
    }
    return HRoomFileError::None();
}

void WriteRoomHeader(Stream *out, RoomFileVersion data_ver, const String &compiled_with)
{
    out->WriteInt16(0xFFFF); // old 16-bit version field, mark as extended version
    out->Write(RoomDataSource::Signature.GetCStr(), RoomDataSource::Signature.GetLength());
    out->WriteInt32(data_ver);
    StrUtil::WriteString(compiled_with, out);
}

void WriteRoomEnding(Stream *out)
{
    out->WriteByte(kRoomFile_EOF);
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
    default: return "unknown";
    }
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
