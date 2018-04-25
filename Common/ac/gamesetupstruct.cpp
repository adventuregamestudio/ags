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
    if (data_ver > kGameVersion_272) // only 3.x
    {
        in->Read(&guid[0], MAX_GUID_LENGTH);
        in->Read(&saveGameFileExtension[0], MAX_SG_EXT_LENGTH);
        in->Read(&saveGameFolderName[0], MAX_SG_FOLDER_LEN);
    }
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

HGameFileError GameSetupStruct::read_sprite_flags(Common::Stream *in, GameDataVersion data_ver)
{
    int numToRead;
    if (data_ver < kGameVersion_256)
        numToRead = 6000; // Fixed number of sprites on < 2.56
    else
        numToRead = in->ReadInt32();

    if (numToRead > MAX_SPRITES)
        return new MainGameFileError(kMGFErr_TooManySprites, String::FromFormat("Count: %d, max: %d", numToRead, MAX_SPRITES));
    in->Read(&spriteflags[0], numToRead);
    memset(spriteflags + numToRead, 0, MAX_SPRITES - numToRead);
    return HGameFileError::None();
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

    if (data_ver > kGameVersion_272) // 3.x
    {
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
    else // 2.x
    {
        int bb;

        charScripts = NULL;
        invScripts = NULL;
        intrChar = new Interaction*[numcharacters];

        for (bb = 0; bb < numcharacters; bb++) {
            intrChar[bb] = Interaction::CreateFromStream(in);
        }
        for (bb = 0; bb < numinvitems; bb++) {
            intrInv[bb] = Interaction::CreateFromStream(in);
        }

        numGlobalVars = in->ReadInt32();
        for (bb = 0; bb < numGlobalVars; bb++) {
            globalvars[bb].Read(in);
        }
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
    if (data_ver >= kGameVersion_254) // lip syncing was introduced in 2.54
        in->ReadArray(&lipSyncFrameLetters[0][0], MAXLIPSYNCFRAMES, 50);
}

void GameSetupStruct::read_messages(Common::Stream *in, GameDataVersion data_ver)
{
    for (int ee=0;ee<MAXGLOBALMES;ee++) {
        if (!load_messages[ee]) continue;
        messages[ee]=(char*)malloc(500);

        if (data_ver < kGameVersion_261) // Global messages are not encrypted on < 2.61
        {
            char* nextchar = messages[ee];

            // TODO: probably this is same as fgetstring
            while (1)
            {
                *nextchar = in->ReadInt8();
                if (*nextchar == 0)
                    break;
                nextchar++;
            }
        }
        else
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

HGameFileError GameSetupStruct::read_customprops(Common::Stream *in, GameDataVersion data_ver)
{
    dialogScriptNames.resize(numdialog);
    viewNames.resize(numviews);
    if (data_ver >= kGameVersion_260) // >= 2.60
    {
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
    }
    return HGameFileError::None();
}

HGameFileError GameSetupStruct::read_audio(Common::Stream *in, GameDataVersion data_ver)
{
    if (data_ver >= kGameVersion_320)
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
    }
    return HGameFileError::None();
}

// Temporarily copied this from acruntim.h;
// it is unknown if this should be defined for all solution, or only runtime
#define STD_BUFFER_SIZE 3000

void GameSetupStruct::read_room_names(Stream *in, GameDataVersion data_ver)
{
    if ((data_ver >= kGameVersion_301) && (options[OPT_DEBUGMODE] != 0))
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

    if (invScripts == NULL)
    {
        for (bb = 0; bb < numinvitems; bb++)
            intrInv[bb]->ReadTimesRunFromSavedgame(in);
        for (bb = 0; bb < numcharacters; bb++)
            intrChar[bb]->ReadTimesRunFromSavedgame(in);
    }

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

//=============================================================================

void ConvertOldGameStruct (OldGameSetupStruct *ogss, GameSetupStruct *gss) {
    int i;
    strcpy (gss->gamename, ogss->gamename);
    for (i = 0; i < 20; i++)
        gss->options[i] = ogss->options[i];
    memcpy (&gss->paluses[0], &ogss->paluses[0], 256);
    memcpy (&gss->defpal[0],  &ogss->defpal[0],  256 * sizeof(color));
    gss->numviews = ogss->numviews;
    gss->numcharacters = ogss->numcharacters;
    gss->playercharacter = ogss->playercharacter;
    gss->totalscore = ogss->totalscore;
    gss->numinvitems = ogss->numinvitems;
    gss->numdialog = ogss->numdialog;
    gss->numdlgmessage = ogss->numdlgmessage;
    gss->numfonts = ogss->numfonts;
    gss->color_depth = ogss->color_depth;
    gss->target_win = ogss->target_win;
    gss->dialog_bullet = ogss->dialog_bullet;
    gss->hotdot = ogss->hotdot;
    gss->hotdotouter = ogss->hotdotouter;
    gss->uniqueid = ogss->uniqueid;
    gss->numgui = ogss->numgui;
    memcpy (&gss->fontflags[0], &ogss->fontflags[0], 10);
    memcpy (&gss->fontoutline[0], &ogss->fontoutline[0], 10);
    memcpy (&gss->spriteflags[0], &ogss->spriteflags[0], 6000);
    memcpy (&gss->invinfo[0], &ogss->invinfo[0], 100 * sizeof(InventoryItemInfo));
    memcpy (&gss->mcurs[0], &ogss->mcurs[0], 10 * sizeof(MouseCursor));
    for (i = 0; i < MAXGLOBALMES; i++)
        gss->messages[i] = ogss->messages[i];
    gss->dict = ogss->dict;
    gss->globalscript = ogss->globalscript;
    gss->chars = NULL; //ogss->chars;
    gss->compiled_script = ogss->compiled_script;
    gss->numcursors = 10;
}

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
