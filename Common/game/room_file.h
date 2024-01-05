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
//
// This unit provides functions for reading compiled room file (CRM)
// into the RoomStruct structure, as well as extracting separate components,
// such as room scripts.
//
//=============================================================================
#ifndef __AGS_CN_GAME_ROOMFILE_H
#define __AGS_CN_GAME_ROOMFILE_H

#include <functional>
#include <memory>
#include <vector>
#include "game/room_version.h"
#include "util/error.h"
#include "util/stream.h"
#include "util/string.h"

struct SpriteInfo;
namespace AGS
{
namespace Common
{

class RoomStruct;

enum RoomFileErrorType
{
    kRoomFileErr_NoError,
    kRoomFileErr_FileOpenFailed,
    kRoomFileErr_FormatNotSupported,
    kRoomFileErr_BlockListFailed,
    kRoomFileErr_UnknownBlockType,
    kRoomFileErr_OldBlockNotSupported,
    kRoomFileErr_IncompatibleEngine,
    kRoomFileErr_ScriptLoadFailed,
    kRoomFileErr_InconsistentData,
    kRoomFileErr_PropertiesBlockFormat,
    kRoomFileErr_InvalidPropertyValues,
    kRoomFileErr_BlockNotFound
};

enum RoomFileBlock
{
    kRoomFblk_None = 0,
    // Main room data
    kRoomFblk_Main = 1,
    // Room script text source (was present in older room formats)
    kRoomFblk_Script = 2,
    // Old versions of compiled script (no longer supported)
    kRoomFblk_CompScript = 3,
    kRoomFblk_CompScript2 = 4,
    // Names of the room objects
    kRoomFblk_ObjectNames = 5,
    // Secondary room backgrounds
    kRoomFblk_AnimBg = 6,
    // Contemporary compiled script
    kRoomFblk_CompScript3 = 7,
    // Custom properties
    kRoomFblk_Properties = 8,
    // Script names of the room objects
    kRoomFblk_ObjectScNames = 9,
    // End of room data tag
    kRoomFile_EOF = 0xFF,
    kRoomFblk_FirstID = kRoomFblk_Main,
    kRoomFblk_LastID = kRoomFblk_ObjectScNames
};

String GetRoomFileErrorText(RoomFileErrorType err);
String GetRoomBlockName(RoomFileBlock id);

typedef TypedCodeError<RoomFileErrorType, GetRoomFileErrorText> RoomFileError;
typedef ErrorHandle<RoomFileError> HRoomFileError;
typedef std::unique_ptr<Stream> UStream;


// RoomDataSource defines a successfully opened room file
struct RoomDataSource
{
    // Name of the asset file
    String              Filename;
    // Room file format version
    RoomFileVersion     DataVersion;
    // A ponter to the opened stream
    UStream             InputStream;

    RoomDataSource();
};


// Opens room data for reading from an arbitrary file
HRoomFileError OpenRoomFile(const String &filename, RoomDataSource &src);
// Opens room data for reading from asset of a given name
HRoomFileError OpenRoomFileFromAsset(const String &filename, RoomDataSource &src);
// Reads room data
HRoomFileError ReadRoomData(RoomStruct *room, Stream *in, RoomFileVersion data_ver);
// Applies necessary updates, conversions and fixups to the loaded data
// making it compatible with current engine
HRoomFileError UpdateRoomData(RoomStruct *room, RoomFileVersion data_ver, bool game_is_hires, const std::vector<SpriteInfo> &sprinfos);
// Extracts text script from the room file, if it's available.
// Historically, text sources were kept inside packed room files before AGS 3.*.
HRoomFileError ExtractScriptText(String &script, Stream *in, RoomFileVersion data_ver);
// Writes all room data to the stream
HRoomFileError WriteRoomData(const RoomStruct *room, Stream *out, RoomFileVersion data_ver);

// Reads room data header using stream assigned to RoomDataSource;
// tests and saves its format index if successful
HRoomFileError ReadRoomHeader(RoomDataSource &src);
// Writes room data header
void WriteRoomHeader(Stream *out, RoomFileVersion data_ver);
// Writes a room data ending
void WriteRoomEnding(Stream *out);

// Type of function that writes single room block.
typedef std::function<void(const RoomStruct *room, Stream *out)> PfnWriteRoomBlock;
// Writes room block with a new-style string id
void WriteRoomBlock(const RoomStruct *room, const String &ext_id, PfnWriteRoomBlock writer, Stream *out);
// Writes room block with a old-style numeric id
void WriteRoomBlock(const RoomStruct *room, RoomFileBlock block, PfnWriteRoomBlock writer, Stream *out);

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_GAME_ROOMFILE_H
