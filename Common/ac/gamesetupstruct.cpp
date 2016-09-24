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


// Create the missing audioClips data structure for 3.1.x games.
// This is done by going through the data files and adding all music*.*
// and sound*.* files to it.
void GameSetupStruct::BuildAudioClipArray(const AssetLibInfo &lib)
{
    char temp_name[30];
    int temp_number;
    char temp_extension[10];

    size_t number_of_files = lib.AssetInfos.size();
    for (size_t i = 0; i < number_of_files; ++i)
    {
        if (sscanf(lib.AssetInfos[i].FileName, "%5s%d.%3s", temp_name, &temp_number, temp_extension) == 3)
        {
            if (stricmp(temp_name, "music") == 0)
            {
                sprintf(audioClips[audioClipCount].scriptName, "aMusic%d", temp_number);
                sprintf(audioClips[audioClipCount].fileName, "music%d.%s", temp_number, temp_extension);
                audioClips[audioClipCount].bundlingType = (stricmp(temp_extension, "mid") == 0) ? AUCL_BUNDLE_EXE : AUCL_BUNDLE_VOX;
                audioClips[audioClipCount].type = 2;
                audioClips[audioClipCount].defaultRepeat = 1;
            }
            else if (stricmp(temp_name, "sound") == 0)
            {
                sprintf(audioClips[audioClipCount].scriptName, "aSound%d", temp_number);
                sprintf(audioClips[audioClipCount].fileName, "sound%d.%s", temp_number, temp_extension);
                audioClips[audioClipCount].bundlingType = AUCL_BUNDLE_EXE;
                audioClips[audioClipCount].type = 3;
            }
            else
            {
                continue;
            }

            audioClips[audioClipCount].defaultVolume = 100;
            audioClips[audioClipCount].defaultPriority = 50;
            audioClips[audioClipCount].id = audioClipCount;

            if (stricmp(temp_extension, "mp3") == 0)
                audioClips[audioClipCount].fileType = eAudioFileMP3;
            else if (stricmp(temp_extension, "wav") == 0)
                audioClips[audioClipCount].fileType = eAudioFileWAV;
            else if (stricmp(temp_extension, "voc") == 0)
                audioClips[audioClipCount].fileType = eAudioFileVOC;
            else if (stricmp(temp_extension, "mid") == 0)
                audioClips[audioClipCount].fileType = eAudioFileMIDI;
            else if ((stricmp(temp_extension, "mod") == 0) || (stricmp(temp_extension, "xm") == 0)
                || (stricmp(temp_extension, "s3m") == 0) || (stricmp(temp_extension, "it") == 0))
                audioClips[audioClipCount].fileType = eAudioFileMOD;
            else if (stricmp(temp_extension, "ogg") == 0)
                audioClips[audioClipCount].fileType = eAudioFileOGG;

            audioClipCount++;
        }
    }
}


MainGameFileError GameSetupStruct::ReadFromFile_Part1(Common::Stream *in, GameDataVersion data_ver)
{
    read_savegame_info(in, data_ver);
    read_font_flags(in);
    MainGameFileError err = read_sprite_flags(in, data_ver);
    if (err != kMGFErr_NoError)
        return err;
    ReadInvInfo_Aligned(in);
    err = read_cursors(in, data_ver);
    if (err != kMGFErr_NoError)
        return err;
    read_interaction_scripts(in, data_ver);
    read_words_dictionary(in);
    return kMGFErr_NoError;
}

MainGameFileError GameSetupStruct::ReadFromFile_Part2(Common::Stream *in, GameDataVersion data_ver)
{
   read_characters(in, data_ver);
   read_lipsync(in, data_ver);
   read_messages(in, data_ver);
   return kMGFErr_NoError;
}

MainGameFileError GameSetupStruct::ReadFromFile_Part3(Common::Stream *in, GameDataVersion data_ver)
{
    MainGameFileError err = read_customprops(in, data_ver);
    if (err != kMGFErr_NoError)
        return err;
    err = read_audio(in, data_ver);
    if (err != kMGFErr_NoError)
        return err;
    read_room_names(in, data_ver);
    return kMGFErr_NoError;
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

void GameSetupStruct::read_font_flags(Common::Stream *in)
{
    in->Read(&fontflags[0], numfonts);
    in->Read(&fontoutline[0], numfonts);
}

MainGameFileError GameSetupStruct::read_sprite_flags(Common::Stream *in, GameDataVersion data_ver)
{
    int numToRead;
    if (data_ver < kGameVersion_256)
        numToRead = 6000; // Fixed number of sprites on < 2.56
    else
        numToRead = in->ReadInt32();

    if (numToRead > MAX_SPRITES)
        return kMGFErr_TooManySprites;
    in->Read(&spriteflags[0], numToRead);
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

    if (data_ver <= kGameVersion_272) // 2.x
    {
        // Change cursor.view from 0 to -1 for non-animating cursors.
        int i;
        for (i = 0; i < numcursors; i++)
        {
            if (mcurs[i].view == 0)
                mcurs[i].view = -1;
        }
    }
    return kMGFErr_NoError;
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

    //charcache = (CharacterCache*)calloc(1,sizeof(CharacterCache)*numcharacters+5);

    if (data_ver <= kGameVersion_272) // fixup charakter script names for 2.x (EGO -> cEgo)
    {
        char tempbuffer[200];
        for (int i = 0; i < numcharacters; i++)
        {
            memset(tempbuffer, 0, 200);
            tempbuffer[0] = 'c';
            tempbuffer[1] = chars[i].scrname[0];
            strcat(&tempbuffer[2], strlwr(&chars[i].scrname[1]));
            strcpy(chars[i].scrname, tempbuffer);
        }
    }

    if (data_ver <= kGameVersion_310) // fix character walk speed for < 3.1.1
    {
        for (int i = 0; i < numcharacters; i++)
        {
            if (options[OPT_ANTIGLIDE])
                chars[i].flags |= CHF_ANTIGLIDE;
        }
    }

    // Characters can always walk through each other on < 2.54
    if (data_ver < kGameVersion_254)
    {
        for (int i = 0; i < numcharacters; i++)
        {
            chars[i].flags |= CHF_NOBLOCKING;
        }
    }
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

MainGameFileError GameSetupStruct::read_customprops(Common::Stream *in, GameDataVersion data_ver)
{
    if (data_ver >= kGameVersion_260) // >= 2.60
    {
        if (Properties::ReadSchema(propSchema, in) != kPropertyErr_NoError)
            return kMGFErr_InvalidPropertySchema;

        int errors = 0;
        int bb;

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
            return kMGFErr_InvalidPropertyValues;

        for (bb = 0; bb < numviews; bb++)
            fgetstring_limit(viewNames[bb], in, MAXVIEWNAMELENGTH);

        for (bb = 0; bb < numinvitems; bb++)
            fgetstring_limit(invScriptNames[bb], in, MAX_SCRIPT_NAME_LEN);

        for (bb = 0; bb < numdialog; bb++)
            fgetstring_limit(dialogScriptNames[bb], in, MAX_SCRIPT_NAME_LEN);
    }
    return kMGFErr_NoError;
}

MainGameFileError GameSetupStruct::read_audio(Common::Stream *in, GameDataVersion data_ver)
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
    else
    {
        // An explanation of building audio clips array for pre-3.2 games.
        //
        // When AGS version 3.2 was released, it contained new audio system.
        // In the nutshell, prior to 3.2 audio files had to be manually put
        // to game project directory and their IDs were taken out of filenames.
        // Since 3.2 this information is stored inside the game data.
        // To make the modern engine compatible with pre-3.2 games, we have
        // to scan game data packages for audio files, and enumerate them
        // ourselves, then add this information to game struct.
        //
        // Some things below can be classified as "dirty hack" and should be
        // fixed in the future.


        // Create soundClips and audioClipTypes structures.
        //
        // TODO: 1000 is a maximal number of clip entries this code is
        // supporting, but it is not clear whether that limit is covering
        // all possibilities. In any way, this array should be replaced
        // with vector.
        audioClipCount = 1000;
        audioClipTypeCount = 4;

        audioClipTypes = (AudioClipType*)malloc(audioClipTypeCount * sizeof(AudioClipType));
        memset(audioClipTypes, 0, audioClipTypeCount * sizeof(AudioClipType));

        audioClips = (ScriptAudioClip*)malloc(audioClipCount * sizeof(ScriptAudioClip));
        memset(audioClips, 0, audioClipCount * sizeof(ScriptAudioClip));

        // TODO: find out what is 4
        for (int i = 0; i < 4; i++)
        {
            audioClipTypes[i].reservedChannels = 1;
            audioClipTypes[i].id = i;
            audioClipTypes[i].volume_reduction_while_speech_playing = 10;
        }
        audioClipTypes[3].reservedChannels = 0;


        audioClipCount = 0;

        // Read audio clip names from "music.vox", then from main library
        // TODO: it's absolutely wrong that this code has to know about
        // "music.vox"; there might be better ways to handle this.
        AssetLibInfo music_lib;
        if (AssetManager::ReadDataFileTOC("music.vox", music_lib) == kAssetNoError)
            BuildAudioClipArray(music_lib);
        const AssetLibInfo *game_lib = AssetManager::GetLibraryTOC();
        if (game_lib)
            BuildAudioClipArray(*game_lib);

        audioClips = (ScriptAudioClip*)realloc(audioClips, audioClipCount * sizeof(ScriptAudioClip));
        scoreClipID = -1;
    }
    return kMGFErr_NoError;
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

void GameSetupStruct::WriteForSaveGame_v321(Stream *out)
{
    WriteInvInfo_Aligned(out);
    WriteMouseCursors_Aligned(out);

    if (invScripts == NULL)
    {
      int bb;
      for (bb = 0; bb < numinvitems; bb++)
        intrInv[bb]->WriteTimesRunToSavedgame(out);
      for (bb = 0; bb < numcharacters; bb++)
        intrChar[bb]->WriteTimesRunToSavedgame(out);
    }

    out->WriteArrayOfInt32 (&options[0], OPT_HIGHESTOPTION_321 + 1);
    out->WriteInt8 (options[OPT_LIPSYNCTEXT]);

    WriteCharacters_Aligned(out);
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
