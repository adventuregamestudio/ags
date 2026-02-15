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
#include "game/tra_file.h"
#include <string.h>
#include "ac/wordsdictionary.h"
#include "debug/out.h"
#include "game/data_helpers.h"
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

static HError ReadFontOverrides(Translation &tra, Stream *in)
{
    uint32_t override_count = static_cast<uint32_t>(in->ReadInt32());
    for (uint32_t i = 0; i < override_count; ++i)
    {
        FontInfo finfo;
        int override_index = in->ReadInt32();
        // ID >= 0 means a replacement is another built-in font
        // ID < 0 means a replacement is a runtime-generated font
        finfo.FontID = in->ReadInt32();
        if (finfo.FontID < 0)
        {
            // This corresponds to the standard font info format in game data
            uint32_t flags = in->ReadInt32();
            finfo.Size = in->ReadInt32();
            finfo.Outline = in->ReadInt32();
            finfo.YOffset = in->ReadInt32();
            finfo.LineSpacing = std::max(0, in->ReadInt32());
            finfo.SetFlags(flags);
            // This corresponds to the "v360_fonts" extension in game data
            // NOTE: we have a 3.6.0 extension right here, because this TRA
            // extension is introduced later. But if there will be more
            // font extensions, then we must have a distinct ext in TRA as well!
            finfo.AutoOutlineThickness = in->ReadInt32();
            finfo.AutoOutlineStyle =
                static_cast<enum FontInfo::AutoOutlineStyle>(in->ReadInt32());
            finfo.CharacterSpacing = in->ReadInt32();
            finfo.CustomHeight = in->ReadInt32();
            in->ReadInt32(); // reserved
            in->ReadInt32();
            // This is added for consistency with 4.* font extension
            finfo.FileName = StrUtil::ReadString(in);
        }
        tra.FontOverrides[override_index] = finfo;
    }
    return HError::None();
}

HError ReadTraBlock(Translation &tra, Stream *in, TraFileBlock block, const String &ext_id, soff_t /*block_len*/)
{
    switch (block)
    {
    case kTraFblk_Dict:
        {
            std::vector<char> buf;
            // Read lines until we find zero-length key & value
            while (true)
            {
                String src_line = read_string_decrypt(in, buf);
                String dst_line = read_string_decrypt(in, buf);
                if (src_line.IsEmpty() && dst_line.IsEmpty())
                    break;
                // Skip empty keys, skip key repeats
                if (src_line.IsEmpty() || (tra.Dict.find(src_line) != tra.Dict.end()))
                    continue;
                tra.Dict.insert(std::make_pair(src_line, dst_line));
            }
        }
        return HError::None();
    case kTraFblk_GameID:
        {
            tra.GameUid = in->ReadInt32();
            tra.GameName = read_string_decrypt(in);
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
    else if (ext_id.CompareNoCase("ext_fonts") == 0)
    {
        return ReadFontOverrides(tra, in);
    }
    
    return new TraFileError(kTraFileErr_UnknownBlockType,
        String::FromFormat("Type: %s", ext_id.GetCStr()));
}


// TRABlockReader reads whole TRA data, block by block
class TRABlockReader : public DataExtReader
{
public:
    TRABlockReader(Translation &tra, std::unique_ptr<Stream> &&in)
        : DataExtReader(std::move(in), kDataExt_NumID32 | kDataExt_File32)
        , _tra(tra) {}

    // Reads only the Game ID block and stops
    HError ReadGameID()
    {
        HError err = FindOne(kTraFblk_GameID);
        if (!err)
            return err;
        return ReadTraBlock(_tra, _in.get(), kTraFblk_GameID, "", _blockLen);
    }

private:
    String GetOldBlockName(int block_id) const override
    { return GetTraBlockName((TraFileBlock)block_id); }
    soff_t GetOverLeeway(int block_id) const override
    {
        // TRA files made by pre-3.0 editors have a block length miscount by 1 byte
        if (block_id == kTraFblk_GameID) return 1;
        return 0;
    }
    HError ReadBlock(Stream *in, int block_id, const String &ext_id,
        soff_t block_len, bool &read_next) override
    {
        read_next = true;
        return ReadTraBlock(_tra, _in.get(), (TraFileBlock)block_id, ext_id, block_len);
    }

    Translation &_tra;
};


HError TestTraGameID(int game_uid, const String &game_name, std::unique_ptr<Stream> &&in)
{
    HError err = OpenTraFile(in.get());
    if (!err)
        return err;

    Translation tra;
    TRABlockReader reader(tra, std::move(in));
    err = reader.ReadGameID();
    if (!err)
        return err;
    // Test the identifiers, if they are not present then skip the test
    if ((tra.GameUid != 0 && (game_uid != tra.GameUid)) ||
        (!tra.GameName.IsEmpty() && (game_name != tra.GameName)))
        return new TraFileError(kTraFileErr_GameIDMismatch,
            String::FromFormat("The translation is designed for '%s'", tra.GameName.GetCStr()));
    return HError::None();
}

HError ReadTraData(Translation &tra, std::unique_ptr<Stream> &&in)
{
    HError err = OpenTraFile(in.get());
    if (!err)
        return err;

    TRABlockReader reader(tra, std::move(in));
    return reader.Read();
}

// TODO: perhaps merge with encrypt/decrypt utilities
static const char *EncryptText(std::vector<char> &en_buf, const String &s)
{
    if (en_buf.size() < s.GetLength() + 1)
        en_buf.resize(s.GetLength() + 1);
    memcpy(en_buf.data(), s.GetCStr(), s.GetLength() + 1);
    encrypt_text(en_buf.data());
    return en_buf.data();
}

// TODO: perhaps merge with encrypt/decrypt utilities
static const char *EncryptEmptyString(std::vector<char> &en_buf)
{
    en_buf[0] = 0;
    encrypt_text(en_buf.data());
    return en_buf.data();
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
            String unsrc = StrUtil::Unescape(PreprocessLineForOldStyleLinebreaks(src));
            String undst = StrUtil::Unescape(PreprocessLineForOldStyleLinebreaks(dst));
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

void WriteFontOverrides(const Translation &tra, Stream *out)
{
    out->WriteInt32(tra.FontOverrides.size());
    for (const auto &font_override : tra.FontOverrides)
    {
        const FontInfo &finfo = font_override.second;
        out->WriteInt32(font_override.first);
        // ID >= 0 means a replacement is another built-in font
        // ID < 0 means a replacement is a runtime-generated font
        out->WriteInt32(finfo.FontID);
        if (finfo.FontID < 0)
        {
            // This corresponds to the standard font info format in game data
            out->WriteInt32(finfo.Flags);
            out->WriteInt32(finfo.Size);
            out->WriteInt32(finfo.Outline);
            out->WriteInt32(finfo.YOffset);
            out->WriteInt32(finfo.LineSpacing);
            // This corresponds to the "v360_fonts" extension in game data
            // NOTE: we have a 3.6.0 extension right here, because this TRA
            // extension is introduced later. But if there will be more
            // font extensions, then we must have a distinct ext in TRA as well!
            out->WriteInt32(finfo.AutoOutlineThickness);
            static_cast<enum FontInfo::AutoOutlineStyle>(out->WriteInt32(finfo.AutoOutlineStyle));
            out->WriteInt32(finfo.CharacterSpacing);
            out->WriteInt32(finfo.CustomHeight);
            out->WriteInt32(0); // reserved
            out->WriteInt32(0);
            // This is added for consistency with 4.* font extension
            StrUtil::WriteString(finfo.FileName, out);
        }
    }
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

void WriteTraData(const Translation &tra, std::unique_ptr<Stream> &&out)
{
    // Write header
    out->Write(TRASignature, strlen(TRASignature) + 1);

    // Write all blocks
    WriteTraBlock(tra, kTraFblk_GameID, WriteGameID, out.get());
    WriteTraBlock(tra, kTraFblk_Dict, WriteDict, out.get());
    WriteTraBlock(tra, kTraFblk_TextOpts, WriteTextOpts, out.get());
    WriteTraBlock(tra, "ext_sopts", WriteStrOptions, out.get());
    if (tra.FontOverrides.size() > 0)
    {
        WriteTraBlock(tra, "ext_fonts", WriteFontOverrides, out.get());
    }

    // Write ending
    out->WriteInt32(kTraFile_EOF);
}

} // namespace Common
} // namespace AGS
