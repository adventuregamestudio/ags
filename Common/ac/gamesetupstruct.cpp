
#include "util/wgt2allg.h"
#include "ac/gamesetupstruct.h"
#include "ac/common.h"
#include "util/string_utils.h"      // fputstring, etc
#include "util/string.h"
#include "util/datastream.h"

using AGS::Common::CDataStream;
using AGS::Common::CString;


// Create the missing audioClips data structure for 3.1.x games.
// This is done by going through the data files and adding all music*.*
// and sound*.* files to it.
extern "C" {
    extern int csetlib(char *namm, char *passw);
    extern int clibGetNumFiles();
    extern char *clibGetFileName(int index);
}

void GameSetupStruct::BuildAudioClipArray()
{
    char temp_name[30];
    int temp_number;
    char temp_extension[10];

    int number_of_files = clibGetNumFiles();
    int i;
    for (i = 0; i < number_of_files; i++)
    {
        if (sscanf(clibGetFileName(i), "%5s%d.%3s", temp_name, &temp_number, temp_extension) == 3)
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


void GameSetupStruct::ReadFromFile_Part1(Common::CDataStream *in, GAME_STRUCT_READ_DATA &read_data)
{
    read_savegame_info(in, read_data);
    read_font_flags(in, read_data);
    read_sprite_flags(in, read_data);
    read_invinfo(in, read_data);
    read_cursors(in, read_data);
    read_interaction_scripts(in, read_data);
    read_words_dictionary(in, read_data);
}

void GameSetupStruct::ReadFromFile_Part2(Common::CDataStream *in, GAME_STRUCT_READ_DATA &read_data)
{
   read_characters(in, read_data);
   read_lipsync(in, read_data);
   read_messages(in, read_data);
}

void GameSetupStruct::ReadFromFile_Part3(Common::CDataStream *in, GAME_STRUCT_READ_DATA &read_data)
{
    read_customprops(in, read_data);
    read_audio(in, read_data);
    read_room_names(in, read_data);
}

//-----------------------------------------------------------------------------
// Reading Part 1

void GameSetupStruct::read_savegame_info(Common::CDataStream *in, GAME_STRUCT_READ_DATA &read_data)
{
    if (read_data.filever > 32) // only 3.x
    {
        in->Read(&guid[0], MAX_GUID_LENGTH);
        in->Read(&saveGameFileExtension[0], MAX_SG_EXT_LENGTH);
        in->Read(&saveGameFolderName[0], MAX_SG_FOLDER_LEN);

        if (saveGameFileExtension[0] != 0)
            sprintf(read_data.saveGameSuffix, ".%s", saveGameFileExtension);
        else
            read_data.saveGameSuffix[0] = 0;
    }
}

void GameSetupStruct::read_font_flags(Common::CDataStream *in, GAME_STRUCT_READ_DATA &read_data)
{
    in->Read(&fontflags[0], numfonts);
    in->Read(&fontoutline[0], numfonts);
}

void GameSetupStruct::read_sprite_flags(Common::CDataStream *in, GAME_STRUCT_READ_DATA &read_data)
{
    int numToRead;
    if (read_data.filever < 24)
        numToRead = 6000; // Fixed number of sprites on < 2.56
    else
        numToRead = in->ReadInt32();

    if (numToRead > MAX_SPRITES) {
        quit("Too many sprites; need newer AGS version");
    }
    in->Read(&spriteflags[0], numToRead);
}

void GameSetupStruct::read_invinfo(Common::CDataStream *in, GAME_STRUCT_READ_DATA &read_data)
{
    //#ifdef ALLEGRO_BIG_ENDIAN
    for (int iteratorCount = 0; iteratorCount < numinvitems; ++iteratorCount)
    {
        invinfo[iteratorCount].ReadFromFile(in);
    }
    //#else
    //    in->ReadArray(&invinfo[0], sizeof(InventoryItemInfo), numinvitems);
    //#endif
}

void GameSetupStruct::read_cursors(Common::CDataStream *in, GAME_STRUCT_READ_DATA &read_data)
{
    if (numcursors > MAX_CURSOR)
        quit("Too many cursors: need newer AGS version");
    //#ifdef ALLEGRO_BIG_ENDIAN
    for (int iteratorCount = 0; iteratorCount < numcursors; ++iteratorCount)
    {
        mcurs[iteratorCount].ReadFromFile(in);
    }
    //#else
    //    in->ReadArray(&mcurs[0], sizeof(MouseCursor), numcursors);
    //#endif

    if (read_data.filever <= 32) // 2.x
    {
        // Change cursor.view from 0 to -1 for non-animating cursors.
        int i;
        for (i = 0; i < numcursors; i++)
        {
            if (mcurs[i].view == 0)
                mcurs[i].view = -1;
        }
    }
}

void GameSetupStruct::read_interaction_scripts(Common::CDataStream *in, GAME_STRUCT_READ_DATA &read_data)
{
    numGlobalVars = 0;

    if (read_data.filever > 32) // 3.x
    {
        int bb;

        charScripts = new InteractionScripts*[numcharacters];
        invScripts = new InteractionScripts*[numinvitems];
        for (bb = 0; bb < numcharacters; bb++) {
            charScripts[bb] = new InteractionScripts();
            deserialize_interaction_scripts(in, charScripts[bb]);
        }
        for (bb = 1; bb < numinvitems; bb++) {
            invScripts[bb] = new InteractionScripts();
            deserialize_interaction_scripts(in, invScripts[bb]);
        }
    }
    else // 2.x
    {
        int bb;

        intrChar = new NewInteraction*[numcharacters];

        for (bb = 0; bb < numcharacters; bb++) {
            intrChar[bb] = deserialize_new_interaction(in);
        }
        for (bb = 0; bb < numinvitems; bb++) {
            intrInv[bb] = deserialize_new_interaction(in);
        }

        numGlobalVars = in->ReadInt32();
        for (bb = 0; bb < numGlobalVars; bb++) {
            globalvars[bb].ReadFromFile(in);
        }
    }
}

void GameSetupStruct::read_words_dictionary(Common::CDataStream *in, GAME_STRUCT_READ_DATA &read_data)
{
    if (dict != NULL) {
        dict = (WordsDictionary*)malloc(sizeof(WordsDictionary));
        read_dictionary (dict, in);
    }
}

//-----------------------------------------------------------------------------
// Reading Part 2

void GameSetupStruct::read_characters(Common::CDataStream *in, GAME_STRUCT_READ_DATA &read_data)
{
    chars=(CharacterInfo*)calloc(1,sizeof(CharacterInfo)*numcharacters+5);
    //#ifdef ALLEGRO_BIG_ENDIAN
    for (int iteratorCount = 0; iteratorCount < numcharacters; ++iteratorCount)
    {
        chars[iteratorCount].ReadFromFile(in);
    }
    //#else
    //    in->ReadArray(&chars[0],sizeof(CharacterInfo),numcharacters,in);  
    //#endif

    //charcache = (CharacterCache*)calloc(1,sizeof(CharacterCache)*numcharacters+5);

    if (read_data.filever <= 32) // fixup charakter script names for 2.x (EGO -> cEgo)
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

    if (read_data.filever <= 37) // fix character walk speed for < 3.1.1
    {
        for (int i = 0; i < numcharacters; i++)
        {
            if (options[OPT_ANTIGLIDE])
                chars[i].flags |= CHF_ANTIGLIDE;
        }
    }
}

void GameSetupStruct::read_lipsync(Common::CDataStream *in, GAME_STRUCT_READ_DATA &read_data)
{
    if (read_data.filever > 19) // > 2.1
        in->ReadArray(&lipSyncFrameLetters[0][0], MAXLIPSYNCFRAMES, 50);
}

void GameSetupStruct::read_messages(Common::CDataStream *in, GAME_STRUCT_READ_DATA &read_data)
{
    for (int ee=0;ee<MAXGLOBALMES;ee++) {
        if (messages[ee]==NULL) continue;
        messages[ee]=(char*)malloc(500);

        if (read_data.filever < 26) // Global messages are not encrypted on < 2.61
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
}

//-----------------------------------------------------------------------------
// Reading Part 3

void GameSetupStruct::read_customprops(Common::CDataStream *in, GAME_STRUCT_READ_DATA &read_data)
{
    if (read_data.filever >= 25) // >= 2.60
    {
        if (propSchema.UnSerialize(in))
            quit("load room: unable to deserialize prop schema");

        int errors = 0;
        int bb;

        for (bb = 0; bb < numcharacters; bb++)
            errors += charProps[bb].UnSerialize (in);
        for (bb = 0; bb < numinvitems; bb++)
            errors += invProps[bb].UnSerialize (in);

        if (errors > 0)
            quit("LoadGame: errors encountered reading custom props");

        for (bb = 0; bb < numviews; bb++)
            fgetstring_limit(viewNames[bb], in, MAXVIEWNAMELENGTH);

        for (bb = 0; bb < numinvitems; bb++)
            fgetstring_limit(invScriptNames[bb], in, MAX_SCRIPT_NAME_LEN);

        for (bb = 0; bb < numdialog; bb++)
            fgetstring_limit(dialogScriptNames[bb], in, MAX_SCRIPT_NAME_LEN);
    }
}

void GameSetupStruct::read_audio(Common::CDataStream *in, GAME_STRUCT_READ_DATA &read_data)
{
    int i;
    if (read_data.filever >= 41)
    {
        audioClipTypeCount = in->ReadInt32();

        if (audioClipTypeCount > read_data.max_audio_types)
            quit("LoadGame: too many audio types");

        audioClipTypes = (AudioClipType*)malloc(audioClipTypeCount * sizeof(AudioClipType));
        //in->ReadArray(&audioClipTypes[0], sizeof(AudioClipType), audioClipTypeCount);
        for (i = 0; i < audioClipTypeCount; ++i)
        {
            audioClipTypes[i].ReadFromFile(in);
        }

        audioClipCount = in->ReadInt32();
        audioClips = (ScriptAudioClip*)malloc(audioClipCount * sizeof(ScriptAudioClip));
        //in->ReadArray(&audioClips[0], sizeof(ScriptAudioClip), audioClipCount);
        for (i = 0; i < audioClipCount; ++i)
        {
            audioClips[i].ReadFromFile(in);
        }
        
        //play.score_sound = in->ReadInt32();
        read_data.score_sound = in->ReadInt32();
    }
    else
    {
        // Create soundClips and audioClipTypes structures.
        audioClipCount = 1000;
        audioClipTypeCount = 4;

        audioClipTypes = (AudioClipType*)malloc(audioClipTypeCount * sizeof(AudioClipType));
        memset(audioClipTypes, 0, audioClipTypeCount * sizeof(AudioClipType));

        audioClips = (ScriptAudioClip*)malloc(audioClipCount * sizeof(ScriptAudioClip));
        memset(audioClips, 0, audioClipCount * sizeof(ScriptAudioClip));

        int i;
        for (i = 0; i < 4; i++)
        {
            audioClipTypes[i].reservedChannels = 1;
            audioClipTypes[i].id = i;
            audioClipTypes[i].volume_reduction_while_speech_playing = 10;
        }
        audioClipTypes[3].reservedChannels = 0;


        audioClipCount = 0;

        if (csetlib("music.vox", "") == 0)
            BuildAudioClipArray();

        csetlib(read_data.game_file_name, "");
        BuildAudioClipArray();

        audioClips = (ScriptAudioClip*)realloc(audioClips, audioClipCount * sizeof(ScriptAudioClip));
    }
}

// Temporarily copied this from acruntim.h;
// it is unknown if this should be defined for all solution, or only runtime
#define STD_BUFFER_SIZE 3000

void GameSetupStruct::read_room_names(CDataStream *in, GAME_STRUCT_READ_DATA &read_data)
{
    if ((read_data.filever >= 36) && (options[OPT_DEBUGMODE] != 0))
    {
        roomCount = in->ReadInt32();
        roomNumbers = (int*)malloc(roomCount * sizeof(int));
        roomNames = (char**)malloc(roomCount * sizeof(char*));
        CString pexbuf;
        for (int bb = 0; bb < roomCount; bb++)
        {
            roomNumbers[bb] = in->ReadInt32();
            pexbuf = in->ReadString(STD_BUFFER_SIZE);
            roomNames[bb] = (char*)malloc(pexbuf.GetLength() + 1);
            strcpy(roomNames[bb], pexbuf);
        }
    }
    else
    {
        roomCount = 0;
    }
}

void GameSetupStruct::ReadFromSaveGame(CDataStream *in, char* gswas, ccScript* compsc, CharacterInfo* chwas,
                                       WordsDictionary *olddict, char** mesbk)
{
    int bb;
    // [IKM] No padding here! -- this data was originally read exactly as done here
    //
    //in->ReadArray(&invinfo[0], sizeof(InventoryItemInfo), numinvitems);
    for (bb = 0; bb < numinvitems; bb++)
    {
        invinfo[bb].ReadFromFile(in);
    }
    //in->ReadArray(&mcurs[0], sizeof(MouseCursor), numcursors);
    for (bb = 0; bb < numcursors; bb++)
    {
        mcurs[bb].ReadFromFile(in);
    }

    if (invScripts == NULL)
    {
        for (bb = 0; bb < numinvitems; bb++)
            in->ReadArrayOfInt32(&intrInv[bb]->timesRun[0], MAX_NEWINTERACTION_EVENTS);
        for (bb = 0; bb < numcharacters; bb++)
            in->ReadArrayOfInt32 (&intrChar[bb]->timesRun[0], MAX_NEWINTERACTION_EVENTS);
    }

    // restore pointer members
    globalscript=gswas;
    compiled_script=compsc;
    chars=chwas;
    dict = olddict;
    for (int vv=0;vv<MAXGLOBALMES;vv++) messages[vv]=mesbk[vv];

    in->ReadArrayOfInt32(&options[0], OPT_HIGHESTOPTION+1);
    options[OPT_LIPSYNCTEXT] = in->ReadInt8();

    //in->ReadArray(&chars[0],sizeof(CharacterInfo),numcharacters,f);
    for (bb = 0; bb < numcharacters; bb++)
    {
        chars[bb].ReadFromFile(in);
    }
}

void GameSetupStruct::WriteForSaveGame(CDataStream *out)
{
    // [IKM] No padding here! -- this data was originally written exactly as done here
    //
    int bb;

    //out->WriteArray(&invinfo[0], sizeof(InventoryItemInfo), numinvitems);
    for (bb = 0; bb < numinvitems; bb++)
    {
        invinfo[bb].WriteToFile(out);
    }
    //out->WriteArray(&mcurs[0], sizeof(MouseCursor), numcursors);
    for (bb = 0; bb < numcursors; bb++)
    {
        mcurs[bb].WriteToFile(out);
    }

    if (invScripts == NULL)
    {
      int bb;
      for (bb = 0; bb < numinvitems; bb++)
        out->WriteArrayOfInt32 (&intrInv[bb]->timesRun[0], MAX_NEWINTERACTION_EVENTS);
      for (bb = 0; bb < numcharacters; bb++)
        out->WriteArrayOfInt32 (&intrChar[bb]->timesRun[0], MAX_NEWINTERACTION_EVENTS); 
    }

    out->WriteArrayOfInt32 (&options[0], OPT_HIGHESTOPTION+1);
    out->WriteInt8 (options[OPT_LIPSYNCTEXT]);

    //out->WriteArray(&chars[0],sizeof(CharacterInfo),numcharacters,f);
    for (bb = 0; bb < numcharacters; bb++)
    {
        chars[bb].WriteToFile(out);
    }
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
