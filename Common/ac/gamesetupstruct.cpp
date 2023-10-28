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

using namespace AGS::Common;

GameSetupStruct::GameSetupStruct()
    : filever(0)
    , roomCount(0)
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

void AdjustFontInfoUsingFlags(FontInfo &finfo, const uint32_t flags)
{
    finfo.Flags = flags;
    if ((flags & FFLG_SIZEMULTIPLIER) != 0)
    {
        finfo.SizeMultiplier = finfo.Size;
        finfo.Size = 0;
    }
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

void GameSetupStruct::ReadInvInfo(Stream *in)
{
    for (int iteratorCount = 0; iteratorCount < numinvitems; ++iteratorCount)
    {
        invinfo[iteratorCount].ReadFromFile(in);
    }
}

void GameSetupStruct::WriteInvInfo(Stream *out)
{
    for (int iteratorCount = 0; iteratorCount < numinvitems; ++iteratorCount)
    {
        invinfo[iteratorCount].WriteToFile(out);
    }
}

HGameFileError GameSetupStruct::read_cursors(Common::Stream *in)
{
    mcurs.resize(numcursors);
    ReadMouseCursors(in);
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
    dict.reset(new WordsDictionary());
    read_dictionary(dict.get(), in);
}

void GameSetupStruct::ReadMouseCursors(Stream *in)
{
    for (int iteratorCount = 0; iteratorCount < numcursors; ++iteratorCount)
    {
        mcurs[iteratorCount].ReadFromFile(in);
    }
}

void GameSetupStruct::WriteMouseCursors(Stream *out)
{
    for (int iteratorCount = 0; iteratorCount < numcursors; ++iteratorCount)
    {
        mcurs[iteratorCount].WriteToFile(out);
    }
}

//-----------------------------------------------------------------------------
// Reading Part 2

void GameSetupStruct::read_characters(Common::Stream *in)
{
    chars.resize(numcharacters);
    ReadCharacters(in);
}

void GameSetupStruct::read_lipsync(Common::Stream *in, GameDataVersion data_ver)
{
    in->ReadArray(&lipSyncFrameLetters[0][0], MAXLIPSYNCFRAMES, 50);
}

void GameSetupStruct::skip_messages(Common::Stream *in,
    const std::array<int, NUM_LEGACY_GLOBALMES> &load_messages, GameDataVersion data_ver)
{
    for (int i = 0; i < NUM_LEGACY_GLOBALMES; ++i)
    {
        if (!load_messages[i])
            continue;
        skip_string_decrypt(in);
    }
}

void GameSetupStruct::ReadCharacters(Stream *in)
{
    for (int iteratorCount = 0; iteratorCount < numcharacters; ++iteratorCount)
    {
        chars[iteratorCount].ReadFromFile(in, loaded_game_file_version);
    }
}

void GameSetupStruct::WriteCharacters(Stream *out)
{
    for (int iteratorCount = 0; iteratorCount < numcharacters; ++iteratorCount)
    {
        chars[iteratorCount].WriteToFile(out);
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
    ReadAudioClips(in, audioclip_count);

    in->ReadInt32(); // [DEPRECATED]
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

void GameSetupStruct::ReadAudioClips(Common::Stream *in, size_t count)
{
    for (size_t i = 0; i < count; ++i)
    {
        audioClips[i].ReadFromFile(in);
    }
}

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
