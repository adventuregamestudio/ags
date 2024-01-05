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
#include "game/tra_file.h"
#include <string.h>
#include "ac/wordsdictionary.h"
#include "debug/out.h"
#include "util/data_ext.h"
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
    default: return "Unknown error.";
    }
}

String GetTraBlockName(TraFileBlock id)
{
    switch (id)
    {
    case kTraFblk_Dict: return "Dictionary";
    case kTraFblk_GameID: return "GameID";
    case kTraFblk_TextOpts: return "TextOpts";
    default: return "unknown";
    }
}

HError OpenTraFile(Stream *in)
{
    // Test the file signature
    char sigbuf[16] = { 0 };
    in->Read(sigbuf, 15);
    if (ags_stricmp(TRASignature, sigbuf) != 0)
        return new TraFileError(kTraFileErr_SignatureFailed);
    return HError::None();
}

HError ReadTraBlock(Translation &tra, Stream *in, TraFileBlock block, const String &ext_id, soff_t /*block_len*/)
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
                read_string_decrypt(in, original, sizeof(original));
                read_string_decrypt(in, translation, sizeof(translation));
                if (!original[0] && !translation[0])
                    break;
                tra.Dict.insert(std::make_pair(String(original), String(translation)));
            }
        }
        return HError::None();
    case kTraFblk_GameID:
        {
            char gamename[256];
            tra.GameUid = in->ReadInt32();
            read_string_decrypt(in, gamename, sizeof(gamename));
            tra.GameName = gamename;
        }
        return HError::None();
    case kTraFblk_TextOpts:
        tra.NormalFont = in->ReadInt32();
        tra.SpeechFont = in->ReadInt32();
        tra.RightToLeft = in->ReadInt32();
        return HError::None();
    case kTraFblk_None:
        // continue reading extensions with string ID
        break;
    default:
        return new TraFileError(kTraFileErr_UnknownBlockType,
            String::FromFormat("Type: %d, known range: %d - %d.", block, kTraFblk_Dict, kTraFblk_TextOpts));
    }

    if (ext_id.CompareNoCase("ext_sopts") == 0)
    {
        StrUtil::ReadStringMap(tra.StrOptions, in);
        return HError::None();
    }
    
    return new TraFileError(kTraFileErr_UnknownBlockType,
        String::FromFormat("Type: %s", ext_id.GetCStr()));
}


// TRABlockReader reads whole TRA data, block by block
class TRABlockReader : public DataExtReader
{
public:
    TRABlockReader(Translation &tra, Stream *in)
        : DataExtReader(in, kDataExt_NumID32 | kDataExt_File32)
        , _tra(tra) {}

    // Reads only the Game ID block and stops
    HError ReadGameID()
    {
        HError err = FindOne(kTraFblk_GameID);
        if (!err)
            return err;
        return ReadTraBlock(_tra, _in, kTraFblk_GameID, "", _blockLen);
    }

private:
    String GetOldBlockName(int block_id) const override
    { return GetTraBlockName((TraFileBlock)block_id); }
    soff_t GetOverLeeway(int block_id) const
    {
        // TRA files made by pre-3.0 editors have a block length miscount by 1 byte
        if (block_id == kTraFblk_GameID) return 1;
        return 0;
    }
    HError ReadBlock(int block_id, const String &ext_id,
        soff_t block_len, bool &read_next) override
    {
        read_next = true;
        return ReadTraBlock(_tra, _in, (TraFileBlock)block_id, ext_id, block_len);
    }

    Translation &_tra;
};


HError TestTraGameID(int game_uid, const String &game_name, Stream *in)
{
    HError err = OpenTraFile(in);
    if (!err)
        return err;

    Translation tra;
    TRABlockReader reader(tra, in);
    err = reader.ReadGameID();
    if (!err)
        return err;
    // Test the identifiers, if they are not present then skip the test
    if ((tra.GameUid != 0 && (game_uid != tra.GameUid)) ||
        !tra.GameName.IsEmpty() && (game_name != tra.GameName))
        return new TraFileError(kTraFileErr_GameIDMismatch,
            String::FromFormat("The translation is designed for '%s'", tra.GameName.GetCStr()));
    return HError::None();
}

HError ReadTraData(Translation &tra, Stream *in)
{
    HError err = OpenTraFile(in);
    if (!err)
        return err;

    TRABlockReader reader(tra, in);
    return reader.Read();
}

// TODO: perhaps merge with encrypt/decrypt utilities
static const char *EncryptText(std::vector<char> &en_buf, const String &s)
{
    if (en_buf.size() < s.GetLength() + 1)
        en_buf.resize(s.GetLength() + 1);
    memcpy(&en_buf.front(), s.GetCStr(), s.GetLength() + 1);
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

void WriteGameID(const Translation &tra, Stream *out)
{
    std::vector<char> en_buf;
    out->WriteInt32(tra.GameUid);
    StrUtil::WriteString(EncryptText(en_buf, tra.GameName), tra.GameName.GetLength() + 1, out);
}

void WriteDict(const Translation &tra, Stream *out)
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
}

void WriteTextOpts(const Translation &tra, Stream *out)
{
    out->WriteInt32(tra.NormalFont);
    out->WriteInt32(tra.SpeechFont);
    out->WriteInt32(tra.RightToLeft);
}

void WriteStrOptions(const Translation &tra, Stream *out)
{
    StrUtil::WriteStringMap(tra.StrOptions, out);
}

inline void WriteTraBlock(const Translation &tra, TraFileBlock block,
    void(*writer)(const Translation &tra, Stream *out), Stream *out)
{
    WriteExtBlock(block, [&tra, writer](Stream *out){ writer(tra, out); },
        kDataExt_NumID32 | kDataExt_File32, out);
}

inline void WriteTraBlock(const Translation &tra, const String &ext_id,
    void(*writer)(const Translation &tra, Stream *out), Stream *out)
{
    WriteExtBlock(ext_id, [&tra, writer](Stream *out) { writer(tra, out); },
        kDataExt_NumID32 | kDataExt_File32, out);
}

void WriteTraData(const Translation &tra, Stream *out)
{
    // Write header
    out->Write(TRASignature, strlen(TRASignature) + 1);

    // Write all blocks
    WriteTraBlock(tra, kTraFblk_GameID, WriteGameID, out);
    WriteTraBlock(tra, kTraFblk_Dict, WriteDict, out);
    WriteTraBlock(tra, kTraFblk_TextOpts, WriteTextOpts, out);
    WriteTraBlock(tra, "ext_sopts", WriteStrOptions, out);

    // Write ending
    out->WriteInt32(kTraFile_EOF);
}

} // namespace Common
} // namespace AGS
