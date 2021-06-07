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
#include "ac/wordsdictionary.h"
#include "debug/out.h"
#include "util/string_compat.h"

namespace AGS
{
namespace Common
{

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
                read_string_decrypt(in, original, sizeof(original));
                read_string_decrypt(in, translation, sizeof(translation));
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
            read_string_decrypt(in, gamename, sizeof(gamename));
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
    const char *tra_sig = "AGSTranslation";
    char sigbuf[16] = { 0 };
    in->Read(sigbuf, 15);
    if (ags_stricmp(tra_sig, sigbuf) != 0)
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

} // namespace Common
} // namespace AGS
