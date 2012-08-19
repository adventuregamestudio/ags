
#include "util/wgt2allg.h"
#include "ac/gamesetupstruct.h"
#include "ac/common.h"
#include "util/string_utils.h"      // fputstring, etc


// Create the missing audioClips data structure for 3.1.x games.
// This is done by going through the data files and adding all music*.*
// and sound*.* files to it.
extern "C" {
    extern int csetlib(const char *namm, const char *passw);
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


void GameSetupStruct::ReadFromFile_Part1(FILE *iii, GAME_STRUCT_READ_DATA &read_data)
{
    read_savegame_info(iii, read_data);
    read_font_flags(iii, read_data);
    read_sprite_flags(iii, read_data);
    read_invinfo(iii, read_data);
    read_cursors(iii, read_data);
    read_interaction_scripts(iii, read_data);
    read_words_dictionary(iii, read_data);
}

void GameSetupStruct::ReadFromFile_Part2(FILE *iii, GAME_STRUCT_READ_DATA &read_data)
{
   read_characters(iii, read_data);
   read_lipsync(iii, read_data);
   read_messages(iii, read_data);
}

void GameSetupStruct::ReadFromFile_Part3(FILE *iii, GAME_STRUCT_READ_DATA &read_data)
{
    read_customprops(iii, read_data);
    read_audio(iii, read_data);
    read_room_names(iii, read_data);
}

//-----------------------------------------------------------------------------
// Reading Part 1

void GameSetupStruct::read_savegame_info(FILE *iii, GAME_STRUCT_READ_DATA &read_data)
{
    if (read_data.filever > 32) // only 3.x
    {
        fread(&guid[0], 1, MAX_GUID_LENGTH, iii);
        fread(&saveGameFileExtension[0], 1, MAX_SG_EXT_LENGTH, iii);
        fread(&saveGameFolderName[0], 1, MAX_SG_FOLDER_LEN, iii);

        if (saveGameFileExtension[0] != 0)
            sprintf(read_data.saveGameSuffix, ".%s", saveGameFileExtension);
        else
            read_data.saveGameSuffix[0] = 0;
    }
}

void GameSetupStruct::read_font_flags(FILE *iii, GAME_STRUCT_READ_DATA &read_data)
{
    fread(&fontflags[0], 1, numfonts, iii);
    fread(&fontoutline[0], 1, numfonts, iii);
}

void GameSetupStruct::read_sprite_flags(FILE*iii, GAME_STRUCT_READ_DATA &read_data)
{
    int numToRead;
    if (read_data.filever < 24)
        numToRead = 6000; // Fixed number of sprites on < 2.56
    else
        numToRead = getw(iii);

    if (numToRead > MAX_SPRITES) {
        quit("Too many sprites; need newer AGS version");
    }
    fread(&spriteflags[0], 1, numToRead, iii);
}

void GameSetupStruct::read_invinfo(FILE*iii, GAME_STRUCT_READ_DATA &read_data)
{
    //#ifdef ALLEGRO_BIG_ENDIAN
    for (int iteratorCount = 0; iteratorCount < numinvitems; ++iteratorCount)
    {
        invinfo[iteratorCount].ReadFromFile(iii);
    }
    //#else
    //    fread(&invinfo[0], sizeof(InventoryItemInfo), numinvitems, iii);
    //#endif
}

void GameSetupStruct::read_cursors(FILE*iii, GAME_STRUCT_READ_DATA &read_data)
{
    if (numcursors > MAX_CURSOR)
        quit("Too many cursors: need newer AGS version");
    //#ifdef ALLEGRO_BIG_ENDIAN
    for (int iteratorCount = 0; iteratorCount < numcursors; ++iteratorCount)
    {
        mcurs[iteratorCount].ReadFromFile(iii);
    }
    //#else
    //    fread(&mcurs[0], sizeof(MouseCursor), numcursors, iii);
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

void GameSetupStruct::read_interaction_scripts(FILE*iii, GAME_STRUCT_READ_DATA &read_data)
{
    numGlobalVars = 0;

    if (read_data.filever > 32) // 3.x
    {
        int bb;

        charScripts = new InteractionScripts*[numcharacters];
        invScripts = new InteractionScripts*[numinvitems];
        for (bb = 0; bb < numcharacters; bb++) {
            charScripts[bb] = new InteractionScripts();
            deserialize_interaction_scripts(iii, charScripts[bb]);
        }
        for (bb = 1; bb < numinvitems; bb++) {
            invScripts[bb] = new InteractionScripts();
            deserialize_interaction_scripts(iii, invScripts[bb]);
        }
    }
    else // 2.x
    {
        int bb;

        intrChar = new NewInteraction*[numcharacters];

        for (bb = 0; bb < numcharacters; bb++) {
            intrChar[bb] = deserialize_new_interaction(iii);
        }
        for (bb = 0; bb < numinvitems; bb++) {
            intrInv[bb] = deserialize_new_interaction(iii);
        }

        numGlobalVars = getw(iii);
        for (bb = 0; bb < numGlobalVars; bb++) {
            globalvars[bb].ReadFromFile(iii);
        }
    }
}

void GameSetupStruct::read_words_dictionary(FILE*iii, GAME_STRUCT_READ_DATA &read_data)
{
    if (dict != NULL) {
        dict = (WordsDictionary*)malloc(sizeof(WordsDictionary));
        read_dictionary (dict, iii);
    }
}

//-----------------------------------------------------------------------------
// Reading Part 2

void GameSetupStruct::read_characters(FILE*iii, GAME_STRUCT_READ_DATA &read_data)
{
    chars=(CharacterInfo*)calloc(1,sizeof(CharacterInfo)*numcharacters+5);
    //#ifdef ALLEGRO_BIG_ENDIAN
    for (int iteratorCount = 0; iteratorCount < numcharacters; ++iteratorCount)
    {
        chars[iteratorCount].ReadFromFile(iii);
    }
    //#else
    //    fread(&chars[0],sizeof(CharacterInfo),numcharacters,iii);  
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

void GameSetupStruct::read_lipsync(FILE*iii, GAME_STRUCT_READ_DATA &read_data)
{
    if (read_data.filever > 19) // > 2.1
        fread(&lipSyncFrameLetters[0][0], MAXLIPSYNCFRAMES, 50, iii);
}

void GameSetupStruct::read_messages(FILE*iii, GAME_STRUCT_READ_DATA &read_data)
{
    for (int ee=0;ee<MAXGLOBALMES;ee++) {
        if (messages[ee]==NULL) continue;
        messages[ee]=(char*)malloc(500);

        if (read_data.filever < 26) // Global messages are not encrypted on < 2.61
        {
            char* nextchar = messages[ee];

            while (1)
            {
                *nextchar = fgetc(iii);
                if (*nextchar == 0)
                    break;
                nextchar++;
            }
        }
        else
            read_string_decrypt(iii, messages[ee]);
    }
}

//-----------------------------------------------------------------------------
// Reading Part 3

void GameSetupStruct::read_customprops(FILE*iii, GAME_STRUCT_READ_DATA &read_data)
{
    if (read_data.filever >= 25) // >= 2.60
    {
        if (propSchema.UnSerialize(iii))
            quit("load room: unable to deserialize prop schema");

        int errors = 0;
        int bb;

        for (bb = 0; bb < numcharacters; bb++)
            errors += charProps[bb].UnSerialize (iii);
        for (bb = 0; bb < numinvitems; bb++)
            errors += invProps[bb].UnSerialize (iii);

        if (errors > 0)
            quit("LoadGame: errors encountered reading custom props");

        for (bb = 0; bb < numviews; bb++)
            fgetstring_limit(viewNames[bb], iii, MAXVIEWNAMELENGTH);

        for (bb = 0; bb < numinvitems; bb++)
            fgetstring_limit(invScriptNames[bb], iii, MAX_SCRIPT_NAME_LEN);

        for (bb = 0; bb < numdialog; bb++)
            fgetstring_limit(dialogScriptNames[bb], iii, MAX_SCRIPT_NAME_LEN);
    }
}

void GameSetupStruct::read_audio(FILE*iii, GAME_STRUCT_READ_DATA &read_data)
{
    int i;
    if (read_data.filever >= 41)
    {
        audioClipTypeCount = getw(iii);

        if (audioClipTypeCount > read_data.max_audio_types)
            quit("LoadGame: too many audio types");

        audioClipTypes = (AudioClipType*)malloc(audioClipTypeCount * sizeof(AudioClipType));
        //fread(&audioClipTypes[0], sizeof(AudioClipType), audioClipTypeCount, iii);
        for (i = 0; i < audioClipTypeCount; ++i)
        {
            audioClipTypes[i].ReadFromFile(iii);
        }

        audioClipCount = getw(iii);
        audioClips = (ScriptAudioClip*)malloc(audioClipCount * sizeof(ScriptAudioClip));
        //fread(&audioClips[0], sizeof(ScriptAudioClip), audioClipCount, iii);
        for (i = 0; i < audioClipCount; ++i)
        {
            audioClips[i].ReadFromFile(iii);
        }
        
        //play.score_sound = getw(iii);
        read_data.score_sound = getw(iii);
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

void GameSetupStruct::read_room_names(FILE*iii, GAME_STRUCT_READ_DATA &read_data)
{
    if ((read_data.filever >= 36) && (options[OPT_DEBUGMODE] != 0))
    {
        char pexbuf[STD_BUFFER_SIZE];

        roomCount = getw(iii);
        roomNumbers = (int*)malloc(roomCount * sizeof(int));
        roomNames = (char**)malloc(roomCount * sizeof(char*));
        for (int bb = 0; bb < roomCount; bb++)
        {
            roomNumbers[bb] = getw(iii);
            fgetstring_limit(pexbuf, iii, sizeof(pexbuf));
            roomNames[bb] = (char*)malloc(strlen(pexbuf) + 1);
            strcpy(roomNames[bb], pexbuf);
        }
    }
    else
    {
        roomCount = 0;
    }
}

void GameSetupStruct::ReadFromSaveGame(FILE *f, char* gswas, ccScript* compsc, CharacterInfo* chwas,
                                       WordsDictionary *olddict, char** mesbk)
{
    int bb;
    // [IKM] No padding here! -- this data was originally read exactly as done here
    //
    //fread(&invinfo[0], sizeof(InventoryItemInfo), numinvitems, f);
    for (bb = 0; bb < numinvitems; bb++)
    {
        invinfo[bb].ReadFromFile(f);
    }
    //fread(&mcurs[0], sizeof(MouseCursor), numcursors, f);
    for (bb = 0; bb < numcursors; bb++)
    {
        mcurs[bb].ReadFromFile(f);
    }

    if (invScripts == NULL)
    {
        for (bb = 0; bb < numinvitems; bb++)
            fread (&intrInv[bb]->timesRun[0], sizeof (int), MAX_NEWINTERACTION_EVENTS, f);
        for (bb = 0; bb < numcharacters; bb++)
            fread (&intrChar[bb]->timesRun[0], sizeof (int), MAX_NEWINTERACTION_EVENTS, f);
    }

    // restore pointer members
    globalscript=gswas;
    compiled_script=compsc;
    chars=chwas;
    dict = olddict;
    for (int vv=0;vv<MAXGLOBALMES;vv++) messages[vv]=mesbk[vv];

    fread(&options[0], sizeof(int), OPT_HIGHESTOPTION+1, f);
    options[OPT_LIPSYNCTEXT] = fgetc(f);

    //fread(&chars[0],sizeof(CharacterInfo),numcharacters,f);
    for (bb = 0; bb < numcharacters; bb++)
    {
        chars[bb].ReadFromFile(f);
    }
}

void GameSetupStruct::WriteForSaveGame(FILE *f)
{
    // [IKM] No padding here! -- this data was originally written exactly as done here
    //
    int bb;

    //fwrite(&invinfo[0], sizeof(InventoryItemInfo), numinvitems, f);
    for (bb = 0; bb < numinvitems; bb++)
    {
        invinfo[bb].WriteToFile(f);
    }
    //fwrite(&mcurs[0], sizeof(MouseCursor), numcursors, f);
    for (bb = 0; bb < numcursors; bb++)
    {
        mcurs[bb].WriteToFile(f);
    }

    if (invScripts == NULL)
    {
      int bb;
      for (bb = 0; bb < numinvitems; bb++)
        fwrite (&intrInv[bb]->timesRun[0], sizeof (int), MAX_NEWINTERACTION_EVENTS, f);
      for (bb = 0; bb < numcharacters; bb++)
        fwrite (&intrChar[bb]->timesRun[0], sizeof (int), MAX_NEWINTERACTION_EVENTS, f); 
    }

    fwrite (&options[0], sizeof(int), OPT_HIGHESTOPTION+1, f);
    fputc (options[OPT_LIPSYNCTEXT], f);

    //fwrite(&chars[0],sizeof(CharacterInfo),numcharacters,f);
    for (bb = 0; bb < numcharacters; bb++)
    {
        chars[bb].WriteToFile(f);
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
