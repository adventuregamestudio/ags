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
#include "ac/audiocliptype.h"
#include "ac/gamesetupstruct.h"
#include "ac/wordsdictionary.h"
#include "ac/dynobj/scriptaudioclip.h"
#include "game/interactions.h"
#include "util/alignedstream.h"

using namespace AGS::Common;

GameSetupStruct::GameSetupStruct()
    : filever(0)
    , roomCount(0)
    , scoreClipID(0)
{
    memset(invinfo, 0, sizeof(invinfo));
    memset(lipSyncFrameLetters, 0, sizeof(lipSyncFrameLetters));
    memset(guid, 0, sizeof(guid));
    memset(saveGameFileExtension, 0, sizeof(saveGameFileExtension));
    memset(saveGameFolderName, 0, sizeof(saveGameFolderName));
}

GameSetupStruct::~GameSetupStruct()
{
    Free();
}

void GameSetupStruct::Free()
{
    GameSetupStructBase::Free();

    charScripts.clear();
    numcharacters = 0;
    invScripts.clear();
    numinvitems = 0;

    roomNames.clear();
    roomNumbers.clear();
    roomCount = 0;

    audioClips.clear();
    audioClipTypes.clear();

    charProps.clear();
    viewNames.clear();
}

// Assigns font info parameters using legacy flags value read from the game data
void SetFontInfoFromLegacyFlags(FontInfo &finfo, const uint8_t data)
{
    finfo.Flags = (data >> 6) & 0xFF;
    finfo.Size = data & FFLG_LEGACY_SIZEMASK;
}

void AdjustFontInfoUsingFlags(FontInfo &finfo, const uint32_t flags)
{
    finfo.Flags = flags;
    if ((flags & FFLG_SIZEMULTIPLIER) != 0)
    {
        finfo.SizeMultiplier = finfo.Size;
        finfo.Size = 0;
    }
}

ScriptAudioClip* GetAudioClipForOldStyleNumber(GameSetupStruct &game, bool is_music, int num)
{
    String clip_name;
    if (is_music)
        clip_name.Format("aMusic%d", num);
    else
        clip_name.Format("aSound%d", num);

    for (size_t i = 0; i < game.audioClips.size(); ++i)
    {
        if (clip_name.CompareNoCase(game.audioClips[i].scriptName) == 0)
            return &game.audioClips[i];
    }
    return nullptr;
}

//-----------------------------------------------------------------------------
// Reading Part 1

void GameSetupStruct::read_savegame_info(Common::Stream *in, GameDataVersion data_ver)
{
        in->Read(&guid[0], MAX_GUID_LENGTH);
        in->Read(&saveGameFileExtension[0], MAX_SG_EXT_LENGTH);
        in->Read(&saveGameFolderName[0], MAX_SG_FOLDER_LEN);
}

void GameSetupStruct::read_font_infos(Common::Stream *in, GameDataVersion data_ver)
{
    fonts.resize(numfonts);
    if (data_ver < kGameVersion_350)
    {
        for (int i = 0; i < numfonts; ++i)
            SetFontInfoFromLegacyFlags(fonts[i], in->ReadInt8());
        for (int i = 0; i < numfonts; ++i)
            fonts[i].Outline = in->ReadInt8(); // size of char
        if (data_ver < kGameVersion_341)
            return;
        for (int i = 0; i < numfonts; ++i)
        {
            fonts[i].YOffset = in->ReadInt32();
            if (data_ver >= kGameVersion_341_2)
                fonts[i].LineSpacing = std::max(0, in->ReadInt32());
        }
    }
    else
    {
        for (int i = 0; i < numfonts; ++i)
        {
            uint32_t flags = in->ReadInt32();
            fonts[i].Size = in->ReadInt32();
            fonts[i].Outline = in->ReadInt32();
            fonts[i].YOffset = in->ReadInt32();
            fonts[i].LineSpacing = std::max(0, in->ReadInt32());
            AdjustFontInfoUsingFlags(fonts[i], flags);
        }
    }
}

void GameSetupStruct::ReadInvInfo_Aligned(Stream *in)
{
    AlignedStream align_s(in, Common::kAligned_Read);
    for (int iteratorCount = 0; iteratorCount < numinvitems; ++iteratorCount)
    {
        invinfo[iteratorCount].ReadFromFile(&align_s);
        align_s.Reset();
    }
}

void GameSetupStruct::WriteInvInfo_Aligned(Stream *out)
{
    AlignedStream align_s(out, Common::kAligned_Write);
    for (int iteratorCount = 0; iteratorCount < numinvitems; ++iteratorCount)
    {
        invinfo[iteratorCount].WriteToFile(&align_s);
        align_s.Reset();
    }
}

HGameFileError GameSetupStruct::read_cursors(Common::Stream *in)
{
    ReadMouseCursors_Aligned(in);
    return HGameFileError::None();
}

void GameSetupStruct::read_interaction_scripts(Common::Stream *in, GameDataVersion data_ver)
{
    charScripts.resize(numcharacters);
    invScripts.resize(numinvitems);
    for (size_t i = 0; i < (size_t)numcharacters; ++i)
        charScripts[i].reset(InteractionScripts::CreateFromStream(in));
    // NOTE: new inventory items' events are loaded starting from 1 for some reason
    for (size_t i = 1; i < (size_t)numinvitems; ++i)
        invScripts[i].reset(InteractionScripts::CreateFromStream(in));
}

void GameSetupStruct::read_words_dictionary(Common::Stream *in)
{
    if (load_dictionary) {
        dict = new WordsDictionary();
        read_dictionary (dict, in);
    }
}

void GameSetupStruct::ReadMouseCursors_Aligned(Stream *in)
{
    mcurs.resize(numcursors);
    AlignedStream align_s(in, Common::kAligned_Read);
    for (int iteratorCount = 0; iteratorCount < numcursors; ++iteratorCount)
    {
        mcurs[iteratorCount].ReadFromFile(&align_s);
        align_s.Reset();
    }
}

void GameSetupStruct::WriteMouseCursors_Aligned(Stream *out)
{
    AlignedStream align_s(out, Common::kAligned_Write);
    for (int iteratorCount = 0; iteratorCount < numcursors; ++iteratorCount)
    {
        mcurs[iteratorCount].WriteToFile(&align_s);
        align_s.Reset();
    }
}

//-----------------------------------------------------------------------------
// Reading Part 2

void GameSetupStruct::read_characters(Common::Stream *in)
{
    chars = new CharacterInfo[numcharacters];

    ReadCharacters_Aligned(in, false);
}

void GameSetupStruct::read_lipsync(Common::Stream *in, GameDataVersion data_ver)
{
    in->ReadArray(&lipSyncFrameLetters[0][0], MAXLIPSYNCFRAMES, 50);
}

// CLNUP global messages are supposed to be gone, check later
void GameSetupStruct::read_messages(Common::Stream *in, GameDataVersion data_ver)
{
    char mbuf[GLOBALMESLENGTH];
    for (int i=0; i < MAXGLOBALMES; ++i)
    {
        if (!load_messages[i]) continue;
        read_string_decrypt(in, mbuf, GLOBALMESLENGTH);
        messages[i] = mbuf;
    }
    delete [] load_messages;
    load_messages = nullptr;
}

void GameSetupStruct::ReadCharacters_Aligned(Stream *in, bool is_save)
{
    AlignedStream align_s(in, Common::kAligned_Read);
    const GameDataVersion data_ver = is_save ? kGameVersion_Undefined : loaded_game_file_version;
    const int save_ver = is_save ? 0 : -1;
    for (int iteratorCount = 0; iteratorCount < numcharacters; ++iteratorCount)
    {
        chars[iteratorCount].ReadFromFile(&align_s, data_ver, save_ver);
        align_s.Reset();
    }
}

void GameSetupStruct::WriteCharacters_Aligned(Stream *out)
{
    AlignedStream align_s(out, Common::kAligned_Write);
    for (int iteratorCount = 0; iteratorCount < numcharacters; ++iteratorCount)
    {
        chars[iteratorCount].WriteToFile(&align_s);
        align_s.Reset();
    }
}

//-----------------------------------------------------------------------------
// Reading Part 3

HGameFileError GameSetupStruct::read_customprops(Common::Stream *in, GameDataVersion data_ver)
{
    dialogScriptNames.resize(numdialog);
    viewNames.resize(numviews);
    if (Properties::ReadSchema(propSchema, in) != kPropertyErr_NoError)
        return new MainGameFileError(kMGFErr_InvalidPropertySchema);

    int errors = 0;

    charProps.resize(numcharacters);
    for (int i = 0; i < numcharacters; ++i)
    {
        errors += Properties::ReadValues(charProps[i], in);
    }
    for (int i = 0; i < numinvitems; ++i)
    {
        errors += Properties::ReadValues(invProps[i], in);
    }

        if (errors > 0)
            return new MainGameFileError(kMGFErr_InvalidPropertyValues);

        for (int i = 0; i < numviews; ++i)
            viewNames[i] = String::FromStream(in);

    for (int i = 0; i < numinvitems; ++i)
        invScriptNames[i] = String::FromStream(in);

    for (int i = 0; i < numdialog; ++i)
        dialogScriptNames[i] = String::FromStream(in);
    return HGameFileError::None();
}

HGameFileError GameSetupStruct::read_audio(Common::Stream *in, GameDataVersion data_ver)
{
    size_t audiotype_count = in->ReadInt32();
    audioClipTypes.resize(audiotype_count);
    for (size_t i = 0; i < audiotype_count; ++i)
    {
        audioClipTypes[i].ReadFromFile(in);
    }

    size_t audioclip_count = in->ReadInt32();
    audioClips.resize(audioclip_count);
    ReadAudioClips_Aligned(in, audioclip_count);

    scoreClipID = in->ReadInt32();
    return HGameFileError::None();
}

// Temporarily copied this from acruntim.h;
// it is unknown if this should be defined for all solution, or only runtime
#define STD_BUFFER_SIZE 3000

void GameSetupStruct::read_room_names(Stream *in, GameDataVersion data_ver)
{
    if (options[OPT_DEBUGMODE] != 0)
    {
        roomCount = in->ReadInt32();
        roomNumbers.resize(roomCount);
        roomNames.resize(roomCount);
        for (int i = 0; i < roomCount; ++i)
        {
            roomNumbers[i] = in->ReadInt32();
            roomNames[i].Read(in);
        }
    }
    else
    {
        roomCount = 0;
    }
}

void GameSetupStruct::ReadAudioClips_Aligned(Common::Stream *in, size_t count)
{
    AlignedStream align_s(in, Common::kAligned_Read);
    for (size_t i = 0; i < count; ++i)
    {
        audioClips[i].ReadFromFile(&align_s);
        align_s.Reset();
    }
}

//=============================================================================
#if defined (OBSOLETE)
#include <stdio.h>
#endif // OBSOLETE

void GameSetupStruct::ReadFromSavegame(Stream *in)
{
    // of GameSetupStruct
    in->ReadArrayOfInt32(options, OPT_HIGHESTOPTION_321 + 1);
    options[OPT_LIPSYNCTEXT] = in->ReadInt32();
    // of GameSetupStructBase
    playercharacter = in->ReadInt32();
    dialog_bullet = in->ReadInt32();
    hotdot = in->ReadInt16();
    hotdotouter = in->ReadInt16();
    invhotdotsprite = in->ReadInt32();
    default_lipsync_frame = in->ReadInt32();
}

void GameSetupStruct::WriteForSavegame(Stream *out)
{
    // of GameSetupStruct
    out->WriteArrayOfInt32(options, OPT_HIGHESTOPTION_321 + 1);
    out->WriteInt32(options[OPT_LIPSYNCTEXT]);
    // of GameSetupStructBase
    out->WriteInt32(playercharacter);
    out->WriteInt32(dialog_bullet);
    out->WriteInt16(hotdot);
    out->WriteInt16(hotdotouter);
    out->WriteInt32(invhotdotsprite);
    out->WriteInt32(default_lipsync_frame);
}
