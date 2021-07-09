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
#include "util/data_ext.h"
#include "debug/out.h"
#include "util/stream.h"

namespace AGS
{
namespace Common
{

String GetDataExtErrorText(DataExtErrorType err)
{
    switch (err)
    {
    case kDataExtErr_NoError:
        return "No error.";
    case kDataExtErr_UnexpectedEOF:
        return "Unexpected end of file.";
    case kDataExtErr_BlockDataOverlapping:
        return "Block data overlapping.";
    }
    return "Unknown error.";
}

HError DataExtParser::OpenBlock()
{
    //    - 1 or 4 bytes - an old-style unsigned numeric ID:
    //               where 0 would indicate following string ID,
    //               and -1 indicates end of the block list.
    //    - 16 bytes - string ID of an extension (if numeric ID is 0).
    //    - 4 or 8 bytes - length of extension data, in bytes.
    block_id = ((flags & kDataExt_NumID32) != 0) ?
        in->ReadInt32() :
        in->ReadInt8();

    if (block_id < 0)
        return HError::None(); // end of list
    if (in->EOS())
        return new DataExtError(kDataExtErr_UnexpectedEOF);

    if (block_id > 0)
    { // old-style block identified by a numeric id
        block_len = ((flags & kDataExt_File64) != 0) ? in->ReadInt64() : in->ReadInt32();
        ext_id = GetOldBlockName(block_id);
    }
    else
    { // new style block identified by a string id
        ext_id = String::FromStreamCount(in, 16);
        block_len = in->ReadInt64();
    }
    block_start = in->GetPosition();
    return HError::None();
}

void DataExtParser::SkipBlock()
{
    if (block_id >= 0)
        in->Seek(block_len);
}

HError DataExtParser::PostAssert()
{
    const soff_t cur_pos = in->GetPosition();
    const soff_t block_end = block_start + block_len;
    if (cur_pos > block_end)
    {
        return new DataExtError(kDataExtErr_BlockDataOverlapping,
            String::FromFormat("Block: '%s', expected to end at offset: %lld, finished reading at %lld.",
                ext_id.GetCStr(), block_end, cur_pos));
    }
    else if (cur_pos < block_end)
    {
        Debug::Printf(kDbgMsg_Warn, "WARNING: data blocks nonsequential, block '%s' expected to end at %lld, finished reading at %lld",
            ext_id.GetCStr(), block_end, cur_pos);
        in->Seek(block_end, Common::kSeekBegin);
    }
    return HError::None();
}

HError DataExtParser::FindOne(int id)
{
    if (id <= 0) return new DataExtError(kDataExtErr_BlockNotFound);

    HError err = HError::None();
    for (err = OpenBlock(); err && !AtEnd(); err = OpenBlock())
    {
        if (id == block_id)
            return HError::None();
        in->Seek(block_len); // skip it
    }
    if (!err)
        return err;
    return new DataExtError(kDataExtErr_BlockNotFound);
}

HError DataExtReader::Read()
{
    HError err = HError::None();
    bool read_next = true;
    for (err = OpenBlock(); err && !AtEnd() && read_next; err = OpenBlock())
    {
        // Call the reader function to read current block's data
        read_next = true;
        err = ReadBlock(block_id, ext_id, block_len, read_next);
        if (!err)
            return err;
        // Test that we did not read too much or too little
        err = PostAssert();
        if (!err)
            return err;
    }
    return err;
}

// Generic function that saves a block and automatically adds its size into header
void WriteExtBlock(int block, const String &ext_id, PfnWriteExtBlock writer, int flags, Stream *out)
{
    // Write block's header
    (flags & kDataExt_NumID32) != 0 ?
        out->WriteInt32(block) :
        out->WriteInt8(block);
    if (block == 0) // new-style string id
        ext_id.WriteCount(out, 16);
    soff_t sz_at = out->GetPosition();
    // block size placeholder
    ((flags & kDataExt_File64) != 0) ?
        out->WriteInt64(0) :
        out->WriteInt32(0);

    // Call writer to save actual block contents
    writer(out);

    // Now calculate the block's size...
    soff_t end_at = out->GetPosition();
    soff_t block_size = (end_at - sz_at) - sizeof(int64_t);
    // ...return back and write block's size in the placeholder
    out->Seek(sz_at, Common::kSeekBegin);
    ((flags & kDataExt_File64) != 0) ?
        out->WriteInt64(block_size) :
        out->WriteInt32((int32_t)block_size);
    // ...and get back to the end of the file
    out->Seek(0, Common::kSeekEnd);
}

} // namespace Common
} // namespace AGS