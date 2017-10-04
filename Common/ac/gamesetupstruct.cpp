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

#include "ac/gamesetupstruct.h"
#include "ac/common.h"
#include "core/asset.h"
#include "core/assetmanager.h"
#include "util/string_utils.h"      // fputstring, etc
#include "util/string.h"
#include "util/alignedstream.h"
#include "util/math.h"

using namespace AGS::Common;

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
    return NULL;
}

//-----------------------------------------------------------------------------
// Reading Part 1

void GameSetupStruct::read_savegame_info(Common::Stream *in, GameDataVersion data_ver)
{
        in->Read(&guid[0], MAX_GUID_LENGTH);
        in->Read(&saveGameFileExtension[0], MAX_SG_EXT_LENGTH);
        in->Read(&saveGameFolderName[0], MAX_SG_FOLDER_LEN);
}

void GameSetupStruct::read_font_flags(Common::Stream *in, GameDataVersion data_ver)
{
    in->Read(&fontflags[0], numfonts);
    in->Read(&fontoutline[0], numfonts);
    if (data_ver < kGameVersion_341)
    {
        memset(fontvoffset, 0, sizeof(fontvoffset));
        memset(fontlnspace, 0, sizeof(fontlnspace));
        return;
    }
    // Extended font parameters
    for (int i = 0; i < numfonts; ++i)
    {
        fontvoffset[i] = in->ReadInt32();
        if (data_ver >= kGameVersion_341_2)
            fontlnspace[i] = in->ReadInt32();
    }
}

MainGameFileError GameSetupStruct::read_sprite_flags(Common::Stream *in, GameDataVersion data_ver)
{
    int numToRead = in->ReadInt32();

    if (numToRead > MAX_SPRITES)
        return kMGFErr_TooManySprites;
    in->Read(&spriteflags[0], numToRead);
    memset(spriteflags + numToRead, 0, MAX_SPRITES - numToRead);
    return kMGFErr_NoError;
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

MainGameFileError GameSetupStruct::read_cursors(Common::Stream *in, GameDataVersion data_ver)
{
    if (numcursors > MAX_CURSOR)
        return kMGFErr_TooManyCursors;

    ReadMouseCursors_Aligned(in);
    return kMGFErr_NoError;
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
        dict = (WordsDictionary*)malloc(sizeof(WordsDictionary));
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
    chars=(CharacterInfo*)calloc(1,sizeof(CharacterInfo)*numcharacters+5);

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
        messages[ee]=(char*)malloc(500);
        read_string_decrypt(in, messages[ee]);
    }
    delete [] load_messages;
    load_messages = NULL;
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

MainGameFileError GameSetupStruct::read_customprops(Common::Stream *in, GameDataVersion data_ver)
{
    dialogScriptNames.resize(numdialog);
    viewNames.resize(numviews);
    if (Properties::ReadSchema(propSchema, in) != kPropertyErr_NoError)
        return kMGFErr_InvalidPropertySchema;

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

    for (int i = 0; i < numviews; ++i)
        viewNames[i] = String::FromStream(in);

    for (int i = 0; i < numinvitems; ++i)
        invScriptNames[i] = String::FromStream(in);

    for (int i = 0; i < numdialog; ++i)
        dialogScriptNames[i] = String::FromStream(in);
    return kMGFErr_NoError;
}

MainGameFileError GameSetupStruct::read_audio(Common::Stream *in, GameDataVersion data_ver)
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

    return kMGFErr_NoError;
}

// Temporarily copied this from acruntim.h;
// it is unknown if this should be defined for all solution, or only runtime
#define STD_BUFFER_SIZE 3000

void GameSetupStruct::read_room_names(Stream *in, GameDataVersion data_ver)
{
    if (options[OPT_DEBUGMODE] != 0)
    {
        roomCount = in->ReadInt32();
        roomNumbers = (int*)malloc(roomCount * sizeof(int));
        roomNames = (char**)malloc(roomCount * sizeof(char*));
        String pexbuf;
        for (int bb = 0; bb < roomCount; bb++)
        {
            roomNumbers[bb] = in->ReadInt32();
            pexbuf.Read(in, STD_BUFFER_SIZE);
            roomNames[bb] = (char*)malloc(pexbuf.GetLength() + 1);
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
    int bb;

    ReadInvInfo_Aligned(in);
    ReadMouseCursors_Aligned(in);

    // restore pointer members
    globalscript=gswas;
    compiled_script=compsc;
    chars=chwas;
    dict = olddict;
    for (int vv=0;vv<MAXGLOBALMES;vv++) messages[vv]=mesbk[vv];

    in->ReadArrayOfInt32(&options[0], OPT_HIGHESTOPTION_321 + 1);
    options[OPT_LIPSYNCTEXT] = in->ReadByte();

    ReadCharacters_Aligned(in);
}

void GameSetupStruct::WriteForSaveGame_v321(Stream *out)
{
    WriteInvInfo_Aligned(out);
    WriteMouseCursors_Aligned(out);

    out->WriteArrayOfInt32 (&options[0], OPT_HIGHESTOPTION_321 + 1);
    out->WriteInt8 (options[OPT_LIPSYNCTEXT]);

    WriteCharacters_Aligned(out);
}

//=============================================================================
