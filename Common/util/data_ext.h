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
// A simple data extension format which may be useful as an ammendment to
// any data file in the game.
//
// Format consists of a list of "blocks", each preceded with an integer and an
// optional string identifier, and a size in bytes which lets a reader to skip
// the block completely if necessary.
// Because the serialization algorithm was accomodated to be shared among
// several existing data files, few things in the meta info may be read
// slightly differently depending on flags passed into the function.
//
//-----------------------------------------------------------------------------
//
// Extension format description.
//
// Each block starts with the header.
// * 1 or 4 bytes (depends on flags) - an old-style unsigned numeric ID :
//   - where 0 would indicate following string ID,
//   - and -1 (0xFF in case of 1 byte ID) indicates an end of the block list.
// * 16 bytes - fixed-len string ID of an extension (if num ID == 0).
// * 4 bytes - total length of the data in bytes;
//   - does not include the size of the header (only following data).
//   - new style blocks (w string ID) always use 8 bytes for a block len;
// * Then goes regular data.
//
// After the block is read the stream position supposed to be at
// (start + length of block), where "start" is a first byte of the header.
// If it's further - the reader bails out with error. If it's not far enough -
// the function logs a warning and skips remaining bytes.
// Blocks are read until meeting ID == -1 in the next header, which indicates
// the end of extension list. An EOF met earlier is considered an error.
//
//=============================================================================
#ifndef __AGS_CN_UTIL__DATAEXT_H
#define __AGS_CN_UTIL__DATAEXT_H

#include <functional>
#include "util/error.h"
#include "util/string.h"

namespace AGS
{
namespace Common
{

enum DataExtFlags
{
    // Size of a numeric ID in bytes
    kDataExt_NumID8  = 0x0000, // default
    kDataExt_NumID32 = 0x0001,
    // 32-bit or 64-bit file offset support
    kDataExt_File32  = 0x0000, // default
    kDataExt_File64  = 0x0002
};

enum DataExtErrorType
{
    kDataExtErr_NoError,
    kDataExtErr_UnexpectedEOF,
    kDataExtErr_BlockDataOverlapping
};

String GetDataExtErrorText(DataExtErrorType err);
typedef TypedCodeError<DataExtErrorType, GetDataExtErrorText> DataExtError;


// Tries to opens a next block from the stream, fills in identifier and length on success
HError OpenExtBlock(Stream *in, int flags, int &block_id, String &ext_id, soff_t &block_len);
// Type of function that reads a single data block and tells whether to continue reading
typedef std::function<HError(Stream *in, int block_id, const String &ext_id,
                soff_t block_len, bool &read_next)> PfnReadExtBlock;
// Parses stream as a block list, passing each found block into callback;
// does not read any actual data itself
HError ReadExtData(PfnReadExtBlock reader, int flags, Stream *in);

// Type of function that writes a single data block.
typedef std::function<void(Stream *out)> PfnWriteExtBlock;
void WriteExtBlock(int block, const String &ext_id, PfnWriteExtBlock writer, int flags, Stream *out);
// Writes a block with a new-style string id
inline void WriteExtBlock(const String &ext_id, PfnWriteExtBlock writer, int flags, Stream *out)
{
    WriteExtBlock(0, ext_id, writer, flags, out);
}
// Writes a block with a old-style numeric id
inline void WriteExtBlock(int block, PfnWriteExtBlock writer, int flags, Stream *out)
{
    WriteExtBlock(block, String(), writer, flags, out);
}

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__DATAEXT_H
