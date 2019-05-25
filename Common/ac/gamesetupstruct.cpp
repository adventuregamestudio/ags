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
    , charScripts(nullptr)
    , invScripts(nullptr)
    , roomCount(0)
    , roomNumbers(nullptr)
    , roomNames(nullptr)
    , audioClipCount(0)
    , audioClips(nullptr)
    , audioClipTypeCount(0)
    , audioClipTypes(nullptr)
    , scoreClipID(0)
{
    memset(invinfo, 0, sizeof(invinfo));
    memset(mcurs, 0, sizeof(mcurs));
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

    if (charScripts)
    {
        for (int i = 0; i < numcharacters; ++i)
            delete charScripts[i];
        delete[] charScripts;
        charScripts = nullptr;
    }
    numcharacters = 0;

    if (invScripts)
    {
        for (int i = 1; i < numinvitems; i++)
            delete invScripts[i];
        delete invScripts;
        invScripts = nullptr;
    }
    numinvitems = 0;

    for (int i = 0; i < roomCount; i++)
        delete roomNames[i];
    delete[] roomNames;
    delete[] roomNumbers;
    roomCount = 0;

    delete[] audioClips;
    audioClips = nullptr;
    delete[] audioClipTypes;
    audioClipTypes = nullptr;
    audioClipCount = 0;
    audioClipTypeCount = 0;

    charProps.clear();
    viewNames.clear();
}

// Assigns font info parameters using legacy flags value read from the game data
void SetFontInfoFromLegacyFlags(FontInfo &finfo, const uint8_t data)
{
    finfo.Flags = (data >> 6) & 0xFF;
    finfo.SizePt = data & FFLG_LEGACY_SIZEMASK;
}

void AdjustFontInfoUsingFlags(FontInfo &finfo, const uint32_t flags)
{
    finfo.Flags = flags;
    if ((flags & FFLG_SIZEMULTIPLIER) != 0)
    {
        finfo.SizeMultiplier = finfo.SizePt;
        finfo.SizePt = 0;
    }
}

ScriptAudioClip* GetAudioClipForOldStyleNumber(GameSetupStruct &game, bool is_music, int num)
{
    String clip_name;
    if (is_music)
        clip_name.Format("aMusic%d", num);
    else
        clip_name.Format("aSound%d", num);

    for (int i = 0; i < game.audioClipCount; ++i)
    {
        if (clip_name.Compare(game.audioClips[i].scriptName) == 0)
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
                fonts[i].LineSpacing = Math::Max(0, in->ReadInt32());
        }
    }
    else
    {
        for (int i = 0; i < numfonts; ++i)
        {
            uint32_t flags = in->ReadInt32();
            fonts[i].SizePt = in->ReadInt32();
            fonts[i].Outline = in->ReadInt32();
            fonts[i].YOffset = in->ReadInt32();
            fonts[i].LineSpacing = Math::Max(0, in->ReadInt32());
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

HGameFileError GameSetupStruct::read_cursors(Common::Stream *in, GameDataVersion data_ver)
{
    if (numcursors > MAX_CURSOR)
        return new MainGameFileError(kMGFErr_TooManyCursors, String::FromFormat("Count: %d, max: %d", numcursors, MAX_CURSOR));

    ReadMouseCursors_Aligned(in);
    return HGameFileError::None();
}

void GameSetupStruct::read_interaction_scripts(Common::Stream *in, GameDataVersion data_ver)
{
    numGlobalVars = 0;

    int bb;

    charScripts = new InteractionScripts*[numcharacters];
    invScripts = new InteractionScripts*[numinvitems];
    for (bb = 0; bb < numcharacters; bb++) {
        charScripts[bb] = InteractionScripts::CreateFromStream(in);
    }
    for (bb = 1; bb < numinvitems; bb++) {
        invScripts[bb] = InteractionScripts::CreateFromStream(in);
    }

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

void GameSetupStruct::read_characters(Common::Stream *in, GameDataVersion data_ver)
{
    chars = new CharacterInfo[numcharacters + 5]; // TODO: why +5, is this really needed?

    ReadCharacters_Aligned(in);
}

void GameSetupStruct::read_lipsync(Common::Stream *in, GameDataVersion data_ver)
{
    in->ReadArray(&lipSyncFrameLetters[0][0], MAXLIPSYNCFRAMES, 50);
}

// CLNUP global messages are supposed to be gone, check later
void GameSetupStruct::read_messages(Common::Stream *in, GameDataVersion data_ver)
{
    for (int ee=0;ee<MAXGLOBALMES;ee++) {
        if (!load_messages[ee]) continue;
        messages[ee] = new char[GLOBALMESLENGTH];
        read_string_decrypt(in, messages[ee], GLOBALMESLENGTH);
    }
    delete [] load_messages;
    load_messages = nullptr;
}

void GameSetupStruct::ReadCharacters_Aligned(Stream *in)
{
    AlignedStream align_s(in, Common::kAligned_Read);
    for (int iteratorCount = 0; iteratorCount < numcharacters; ++iteratorCount)
    {
        chars[iteratorCount].ReadFromFile(&align_s);
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
    audioClipTypeCount = in->ReadInt32();

    audioClipTypes = (AudioClipType*)malloc(audioClipTypeCount * sizeof(AudioClipType));
    for (int i = 0; i < audioClipTypeCount; ++i)
    {
        audioClipTypes[i].ReadFromFile(in);
    }

    audioClipCount = in->ReadInt32();
    audioClips = (ScriptAudioClip*)malloc(audioClipCount * sizeof(ScriptAudioClip));
    ReadAudioClips_Aligned(in);
    
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
        roomNumbers = new int[roomCount];
        roomNames = new char*[roomCount];
        String pexbuf;
        for (int bb = 0; bb < roomCount; bb++)
        {
            roomNumbers[bb] = in->ReadInt32();
            pexbuf.Read(in, STD_BUFFER_SIZE);
            roomNames[bb] = new char[pexbuf.GetLength() + 1];
            strcpy(roomNames[bb], pexbuf);
        }
    }
    else
    {
        roomCount = 0;
    }
}

void GameSetupStruct::ReadAudioClips_Aligned(Common::Stream *in)
{
    AlignedStream align_s(in, Common::kAligned_Read);
    for (int i = 0; i < audioClipCount; ++i)
    {
        audioClips[i].ReadFromFile(&align_s);
        align_s.Reset();
    }
}

void GameSetupStruct::ReadFromSaveGame_v321(Stream *in, char* gswas, ccScript* compsc, CharacterInfo* chwas,
                                       WordsDictionary *olddict, char** mesbk)
{
    ReadInvInfo_Aligned(in);
    ReadMouseCursors_Aligned(in);

    // restore pointer members
    globalscript=gswas;
    CompiledScript=compsc;
    chars=chwas;
    dict = olddict;
    for (int vv=0;vv<MAXGLOBALMES;vv++) messages[vv]=mesbk[vv];

    in->ReadArrayOfInt32(&options[0], OPT_HIGHESTOPTION_321 + 1);
    options[OPT_LIPSYNCTEXT] = in->ReadByte();

    ReadCharacters_Aligned(in);
}

//=============================================================================

void GameSetupStruct::ReadFromSavegame(PStream in)
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

void GameSetupStruct::WriteForSavegame(PStream out)
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
