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
    // NOTE: for historical reasons this refers to blocks with numeric ID ONLY;
    // new-style blocks with a 16-char ID always write 64-bit offset
    kDataExt_File32  = 0x0000, // default
    kDataExt_File64  = 0x0002
};

enum DataExtErrorType
{
    kDataExtErr_NoError,
    kDataExtErr_UnexpectedEOF,
    kDataExtErr_BlockNotFound,
    kDataExtErr_BlockDataOverlapping
};

String GetDataExtErrorText(DataExtErrorType err);
typedef TypedCodeError<DataExtErrorType, GetDataExtErrorText> DataExtError;


// DataExtReader parses a generic extendable block list and
// does format checks, but does not read any data itself.
// Use it to open blocks, and assert reading correctness.
class DataExtParser
{
public:
    DataExtParser(Stream *in, int flags) : _in(in), _flags(flags) {}
    virtual ~DataExtParser() = default;

    // Returns the conventional string ID for an old-style block with numeric ID
    virtual String GetOldBlockName(int block_id) const
    { return String::FromFormat("id:%d", block_id); }
    // Provides a leeway for over-reading (reading past the reported block length):
    // the parser will not error if the mistake is in this range of bytes
    virtual soff_t GetOverLeeway(int /*block_id*/) const { return 0; }

    // Gets a stream
    inline Stream *GetStream() { return _in; }
    // Tells if the end of the block list was reached
    inline bool AtEnd() const { return _blockID < 0; }
    // Gets parser flags
    inline int GetFlags() const { return _flags; }
    // Gets current block ID
    inline int GetBlockID() const { return _blockID; }
    inline String GetBlockName() const
    { return _blockID < 0 ? "" : (_blockID > 0 ? GetOldBlockName(_blockID) : _extID); }
    inline soff_t GetBlockOffset() const { return _blockStart; }
    // Gets current block length
    inline soff_t GetBlockLength() const { return _blockLen; }
    // Tries to opens a next standard block from the stream,
    // fills in identifier and length on success
    HError OpenBlock();
    // Skips current block
    void   SkipBlock();
    // Asserts current stream position after a block was read
    HError PostAssert();
    // Parses a block list in search for a particular block,
    // if found opens it.
    HError FindOne(int id);

protected:
    Stream *_in {};
    int _flags {};

    int _blockID {-1};
    String _extID;
    soff_t _blockStart {};
    soff_t _blockLen {};
};

// DataExtReader is a virtual base class of a block list reader; provides
// a helper method for reading all the blocks one by one, but leaves data
// reading for the child classes. A child class must override ReadBlock method.
// TODO: don't extend Parser, but have it as a member?
class DataExtReader : protected DataExtParser
{
public:
    virtual ~DataExtReader() = default;

    // Parses a block list, calls ReadBlock for each found block
    HError Read();

protected:
    DataExtReader(Stream *in, int flags) : DataExtParser(in, flags) {}
    // Reads a single data block and tell whether to continue reading;
    // default implementation skips the block
    virtual HError ReadBlock(int block_id, const String &ext_id,
        soff_t block_len, bool &read_next) = 0;
};


// Type of function that writes a single data block.
typedef std::function<void(Stream *out)> PfnWriteExtBlock;
void WriteExtBlock(int block, const String &ext_id, const PfnWriteExtBlock& writer, int flags, Stream *out);
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
