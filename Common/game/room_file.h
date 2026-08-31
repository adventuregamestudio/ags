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
// This unit provides functions for reading compiled room file (CRM)
// into the RoomData structure, as well as extracting separate components,
// such as room scripts.
//
//=============================================================================
#ifndef __AGS_CN_GAME_ROOMFILE_H
#define __AGS_CN_GAME_ROOMFILE_H

#include <functional>
#include <memory>
#include <vector>
#include "game/room_version.h"
#include "game/roomdata.h"
#include "util/error.h"
#include "util/stream.h"
#include "util/string.h"

struct SpriteInfo;

namespace AGS
{
namespace Common
{

enum RoomFileErrorType
{
    kRoomFileErr_NoError,
    kRoomFileErr_FileOpenFailed,
    kRoomFileErr_SignatureFailed,
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
    // Signature of the current room format
    static const String Signature;

    // Name of the asset file
    String              Filename;
    // Room file format version
    RoomFileVersion     DataVersion = kRoomVersion_Undefined;
    // Tool identifier (like version) this game was compiled with
    String              CompiledWith;
    // A ponter to the opened stream
    UStream             InputStream;

    RoomDataSource() = default;
    RoomDataSource(const String &filename, UStream &&stream, RoomFileVersion data_ver = kRoomVersion_Undefined)
        : Filename(filename), InputStream(std::move(stream)), DataVersion(data_ver)
    { }
};

// Auxiliary room data, not commonly needed for running the room at runtime,
// but may be required for importing design-time data or other.
class RoomDataAux
{
public:
    String ScriptText;
};

// Full room data, includes both runtime and design-time data,
// including obsolete data from 2.x editors
class RoomDataExt : public RoomData, public RoomDataAux
{
};


// Opens room data for reading from the file; on success assigns a stream to RoomDataSource
HRoomFileError OpenRoomFile(const String &filename, RoomDataSource &src);
// Opens room data for reading from the arbitrary stream found in RoomDataSource
HRoomFileError OpenRoomFile(RoomDataSource &src);
// Reads room data
HRoomFileError ReadRoomData(RoomData *room, std::unique_ptr<Stream> &&in, RoomFileVersion data_ver);
// Reads room data and fills in auxiliary data, if one is found in the stream.
HRoomFileError ReadRoomData(RoomDataExt *room, std::unique_ptr<Stream> &&in, RoomFileVersion data_ver);
// Applies necessary updates, conversions and fixups to the loaded data
// making it compatible with current engine
HRoomFileError UpdateRoomData(RoomData *room, RoomFileVersion data_ver, const std::vector<SpriteInfo> *sprinfos);
// Loads new room data into the given RoomData object and upgrade it to the latest version
HRoomFileError LoadRoom(RoomData *room, std::unique_ptr<Stream> &&in, const std::vector<SpriteInfo> *sprinfos);
// Writes all room data to the stream
HRoomFileError WriteRoomData(const RoomData *room, const RoomDataAux *room_aux, Stream *out, RoomFileVersion data_ver, const String &compiled_with);
// Writes all extended room data to the stream
HRoomFileError WriteRoomData(const RoomDataExt *room, Stream *out, RoomFileVersion data_ver, const String &compiled_with);

//
// Helper functions for the non-standard room file reading.
//

// Additional reading options that let skip certain data.
// Purposed mainly for the cli tools that perform individual data extraction;
// helps to reduce operation time and memory consumption (fwiw).
struct RoomReadOptions
{
    bool PartialRead = false; // tells to ignore if a block is not fully read
    bool SkipImageData = false; // tells to skip room images (backgrounds, masks)
};

// Reads room data from the only specified room file blocks
HRoomFileError ReadRoomData(RoomData *room, RoomDataAux *room_aux, std::unique_ptr<Stream> &&in, RoomFileVersion data_ver,
    const std::vector<std::pair<RoomFileBlock, String>> &blocks_to_read, const RoomReadOptions &read_opts);

// Extracts text script from the room file, if it's available.
// Historically, text sources were kept inside packed room files before AGS 3.*.
// FIXME: this is only used by 3.x Editor when importing pre-3.x rooms, remove in 4.x.
HRoomFileError ExtractScriptText(String &script, std::unique_ptr<Stream> &&in, RoomFileVersion data_ver);

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_GAME_ROOMFILE_H
