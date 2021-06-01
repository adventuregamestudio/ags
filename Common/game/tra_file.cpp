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
#include "game/tra_file.h"
#include <string.h>
#include "ac/wordsdictionary.h"
#include "debug/out.h"
#include "util/string_compat.h"
#include "util/string_utils.h"

namespace AGS
{
namespace Common
{

const char *TRASignature = "AGSTranslation";


String GetTraFileErrorText(TraFileErrorType err)
{
    switch (err)
    {
    case kTraFileErr_NoError:
        return "No error.";
    case kTraFileErr_SignatureFailed:
        return "Not an AGS translation file or an unsupported format.";
    case kTraFileErr_FormatNotSupported:
        return "Format version not supported.";
    case kTraFileErr_GameIDMismatch:
        return "Game ID does not match, translation is meant for a different game.";
    case kTraFileErr_UnexpectedEOF:
        return "Unexpected end of file.";
    case kTraFileErr_UnknownBlockType:
        return "Unknown block type.";
    case kTraFileErr_BlockDataOverlapping:
        return "Block data overlapping.";
    }
    return "Unknown error.";
}

String GetTraBlockName(TraFileBlock id)
{
    switch (id)
    {
    case kTraFblk_Dict: return "Dictionary";
    case kTraFblk_GameID: return "GameID";
    case kTraFblk_TextOpts: return "TextOpts";
    }
    return "unknown";
}

HTraFileError ReadTraBlock(Translation &tra, Stream *in, TraFileBlock block, soff_t block_len)
{
    switch (block)
    {
    case kTraFblk_Dict:
        {
            char original[1024];
            char translation[1024];
            // Read lines until we find zero-length key & value
            while (true)
            {
                read_string_decrypt(in, original, 1024);
                read_string_decrypt(in, translation, 1024);
                if (!original[0] && !translation[0])
                    break;
                tra.Dict.insert(std::make_pair(String(original), String(translation)));
            }
        }
        break;
    case kTraFblk_GameID:
        {
            char gamename[256];
            tra.GameUid = in->ReadInt32();
            read_string_decrypt(in, gamename, 256);
            tra.GameName = gamename;
        }
        break;
    case kTraFblk_TextOpts:
        tra.NormalFont = in->ReadInt32();
        tra.SpeechFont = in->ReadInt32();
        tra.RightToLeft = in->ReadInt32();
        break;
    default:
        return new TraFileError(kTraFileErr_UnknownBlockType,
            String::FromFormat("Type: %d, known range: %d - %d.", block, kTraFblk_Dict, kTraFblk_TextOpts));
    }
    return HTraFileError::None();
}

HTraFileError ReadTraData(PfnReadTraBlock reader, Stream *in)
{
    // Test the file signature
    char sigbuf[16] = { 0 };
    in->Read(sigbuf, 15);
    if (ags_stricmp(TRASignature, sigbuf) != 0)
        return new TraFileError(kTraFileErr_SignatureFailed);

    while (!in->EOS())
    {
        TraFileBlock block_id = (TraFileBlock)in->ReadInt32();
        if (block_id == kTraFile_EOF)
            break; // end of list

        soff_t block_len = in->ReadInt32();
        soff_t block_end = in->GetPosition() + block_len;
        String ext_id = GetTraBlockName(block_id);

        bool read_next = true;
        HTraFileError err = reader(in, block_id, block_len, read_next);
        if (!err)
            return err;

        soff_t cur_pos = in->GetPosition();
        if (cur_pos > block_end)
        {
            return new TraFileError(kTraFileErr_BlockDataOverlapping,
                String::FromFormat("Block: %s, expected to end at offset: %lld, finished reading at %lld.",
                    ext_id.GetCStr(), block_end, cur_pos));
        }
        else if (cur_pos < block_end)
        {
            Debug::Printf(kDbgMsg_Warn,
                "WARNING: translation data blocks nonsequential, block type %s expected to end at %lld, finished reading at %lld",
                ext_id.GetCStr(), block_end, cur_pos);
            in->Seek(block_end, Common::kSeekBegin);
        }

        if (!read_next)
            break;
    }
    return HTraFileError::None();
}

HTraFileError TestTraGameID(int game_uid, const String &game_name, Stream *in)
{
    // This reader would only process kTraFblk_GameID and exit as soon as one is found
    Translation tra;
    auto reader = [&tra](Stream *in, TraFileBlock block_id,
        soff_t block_len, bool &read_next)
    {
        if (block_id == kTraFblk_GameID)
        {
            read_next = false;
            return ReadTraBlock(tra, in, block_id, block_len);
        }
        in->Seek(block_len); // skip block
        return HTraFileError::None();
    };

    HTraFileError err = ReadTraData(reader, in);
    if (!err)
        return err;
    // Test the identifiers, if they are not present then skip the test
    if ((tra.GameUid != 0 && (game_uid != tra.GameUid)) ||
        !tra.GameName.IsEmpty() && (game_name != tra.GameName))
        return new TraFileError(kTraFileErr_GameIDMismatch,
            String::FromFormat("The translation is designed for '%s'", tra.GameName.GetCStr()));
    return HTraFileError::None();
}

HTraFileError ReadTraData(Translation &tra, Stream *in)
{
    // This reader will process all blocks inside ReadTraBlock() function,
    // and read compatible data into the given Translation object
    auto reader = [&tra](Stream *in, TraFileBlock block_id,
        soff_t block_len, bool &read_next)
    { return ReadTraBlock(tra, in, block_id, block_len); };
    return ReadTraData(reader, in);
}

// TODO: perhaps merge with encrypt/decrypt utilities
static const char *EncryptText(std::vector<char> &en_buf, const String &s)
{
    if (en_buf.size() < s.GetLength() + 1)
        en_buf.resize(s.GetLength() + 1);
    strncpy(&en_buf.front(), s.GetCStr(), s.GetLength() + 1);
    encrypt_text(&en_buf.front());
    return &en_buf.front();
}

// TODO: perhaps merge with encrypt/decrypt utilities
static const char *EncryptEmptyString(std::vector<char> &en_buf)
{
    en_buf[0] = 0;
    encrypt_text(&en_buf.front());
    return &en_buf.front();
}

HTraFileError WriteGameID(const Translation &tra, Stream *out)
{
    std::vector<char> en_buf;
    out->WriteInt32(tra.GameUid);
    StrUtil::WriteString(EncryptText(en_buf, tra.GameName), tra.GameName.GetLength() + 1, out);
    return HTraFileError::None();
}

HTraFileError WriteDict(const Translation &tra, Stream *out)
{
    std::vector<char> en_buf;
    for (const auto &kv : tra.Dict)
    {
        const String &src = kv.first;
        const String &dst = kv.second;
        if (!dst.IsNullOrSpace())
        {
            String unsrc = StrUtil::Unescape(src);
            String undst = StrUtil::Unescape(dst);
            StrUtil::WriteString(EncryptText(en_buf, unsrc), unsrc.GetLength() + 1, out);
            StrUtil::WriteString(EncryptText(en_buf, undst), undst.GetLength() + 1, out);
        }
    }
    // Write a pair of empty key/values
    StrUtil::WriteString(EncryptEmptyString(en_buf), 1, out);
    StrUtil::WriteString(EncryptEmptyString(en_buf), 1, out);
    return HTraFileError::None();
}

HTraFileError WriteTextOpts(const Translation &tra, Stream *out)
{
    out->WriteInt32(tra.NormalFont);
    out->WriteInt32(tra.SpeechFont);
    out->WriteInt32(tra.RightToLeft);
    return HTraFileError::None();
}

HTraFileError WriteTraData(const Translation &tra, Stream *out)
{
    // Write header
    out->Write(TRASignature, strlen(TRASignature) + 1);

    // Write all blocks
    WriteTraBlock(tra, kTraFblk_GameID, WriteGameID, out);
    WriteTraBlock(tra, kTraFblk_Dict, WriteDict, out);
    WriteTraBlock(tra, kTraFblk_TextOpts, WriteTextOpts, out);

    // Write ending
    out->WriteInt32(kTraFile_EOF);
    return HTraFileError::None();
}

void WriteTraBlock(const Translation &tra, TraFileBlock block_id, PfnWriteTraBlock writer, Stream *out)
{
    // Write block's header
    out->WriteInt32(block_id);
    soff_t sz_at = out->GetPosition();
    out->WriteInt32(0); // block size placeholder
    // Call writer to save actual block contents
    writer(tra, out);

    // Now calculate the block's size...
    soff_t end_at = out->GetPosition();
    soff_t block_size = (end_at - sz_at) - sizeof(int32_t);
    // ...return back and write block's size in the placeholder
    out->Seek(sz_at, Common::kSeekBegin);
    out->WriteInt32((int)block_size);
    // ...and get back to the end of the file
    out->Seek(0, Common::kSeekEnd);
}

} // namespace Common
} // namespace AGS
