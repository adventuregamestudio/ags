
#include <stdio.h>
#include "ac/characterinfo.h"
#include "ac/common.h"
#include "ac/interaction.h"
#include "ac/wordsdictionary.h"
#include "core/assetmanager.h"
#include "game/gameinfo.h"
#include "util/alignedstream.h"
#include "util/string_utils.h"

using namespace AGS;
using namespace AGS::Common;

GameInfo::GameInfo()
{
    memset(PaletteUses, 0, sizeof(PaletteUses));
    memset(DefaultPalette, 0, sizeof(DefaultPalette));
    PlayerCharacterIndex    = 0;
    TotalScore              = 0;
    ColorDepth              = 0;
    DialogBulletSprIndex    = 0;
    InvItemHotDotColor      = 0;
    InvItemHotDotOuterColor = 0;
    UniqueId                = 0;
    DefaultResolution       = 0;
    DefaultLipSyncFrame     = 0;
    InvItemHotDotSprIndex   = 0;
    Dictionary              = NULL;
    CompiledScript          = NULL;

    ViewCount               = 0;
    CharacterCount          = 0;
    InvItemCount            = 0;
    DialogCount             = 0;
    DialogMessageCount      = 0;
    FontCount               = 0;
    GuiCount                = 0;
    MouseCursorCount        = 0;
    LoadDictionary          = false;
    RoomCount               = 0;
    LoadCompiledScript      = false;
}

GameInfo::~GameInfo()
{
}

//-----------------------------------------------------------------------------
//
// Base part
//
//-----------------------------------------------------------------------------

void GameInfo::ReadBaseFromFile(Stream *in)
{
    GameName.ReadCount(in, 50);
    in->ReadArrayOfInt32(Options, 100);
    in->Read(PaletteUses, 256);
    // colors are an array of bytes
    in->Read(DefaultPalette, sizeof(color)*256);
    ViewCount               = in->ReadInt32();
    CharacterCount          = in->ReadInt32();
    PlayerCharacterIndex    = in->ReadInt32();
    TotalScore              = in->ReadInt32();
    InvItemCount            = in->ReadInt16();
    DialogCount             = in->ReadInt32();
    DialogMessageCount      = in->ReadInt32();
    FontCount               = in->ReadInt32();
    ColorDepth              = in->ReadInt32();
    in->ReadInt32(); // skip target_win
    DialogBulletSprIndex    = in->ReadInt32();
    InvItemHotDotColor      = in->ReadInt16();
    InvItemHotDotOuterColor = in->ReadInt16();
    UniqueId                = in->ReadInt32();
    GuiCount                = in->ReadInt32();
    MouseCursorCount        = in->ReadInt32();
    DefaultResolution       = in->ReadInt32();
    DefaultLipSyncFrame     = in->ReadInt32();
    InvItemHotDotSprIndex   = in->ReadInt32();
    int32_t reserved[17];
    in->ReadArrayOfInt32(reserved, 17); // skip unused data
    // read the final ptrs so we know to load dictionary, scripts etc
    in->ReadArrayOfInt32(MessageToLoad, MAXGLOBALMES);

    LoadDictionary = in->ReadInt32() != 0; // dict
    // skip unused data
    in->ReadInt32(); // globalscript
    in->ReadInt32(); // characters pointer
    LoadCompiledScript = in->ReadInt32() != 0; // compiled_script
}

void GameInfo::WriteBaseToFile(Stream *out)
{
    GameName.WriteCount(out, 50);
    out->WriteArrayOfInt32(Options, 100);
    out->Write(PaletteUses, 256);
    // colors are an array of bytes
    out->Write(&DefaultPalette[0], sizeof(color)*256);
    out->WriteInt32(ViewCount);
    out->WriteInt32(CharacterCount);
    out->WriteInt32(PlayerCharacterIndex);
    out->WriteInt32(TotalScore);
    out->WriteInt16(InvItemCount);
    out->WriteInt32(DialogCount);
    out->WriteInt32(DialogMessageCount);
    out->WriteInt32(FontCount);
    out->WriteInt32(ColorDepth);
    out->WriteInt32(0); // target_win
    out->WriteInt32(DialogBulletSprIndex);
    out->WriteInt16(InvItemHotDotColor);
    out->WriteInt16(InvItemHotDotOuterColor);
    out->WriteInt32(UniqueId);
    out->WriteInt32(GuiCount);
    out->WriteInt32(MouseCursorCount);
    out->WriteInt32(DefaultResolution);
    out->WriteInt32(DefaultLipSyncFrame);
    out->WriteInt32(InvItemHotDotSprIndex);
    int32_t reserved[17];
    out->WriteArrayOfInt32(reserved, 17);
    // write the final ptrs so we know to load dictionary, scripts etc
    for (int i = 0; i < MAXGLOBALMES; ++i)
    {
        out->WriteInt32(GlobalMessages[i].IsEmpty() ? 0 : 1);
    }
    out->WriteInt32(Dictionary ? 1 : 0);
    out->WriteInt32(0); // globalscript
    out->WriteInt32(0); // characters pointer
    out->WriteInt32(LoadCompiledScript ? 1 : 0); // compiled_script
}

//-----------------------------------------------------------------------------
//
// Extended part
//
//-----------------------------------------------------------------------------

// Create the missing AudioClips data structure for 3.1.x games.
// This is done by going through the data files and adding all music*.*
// and sound*.* files to it.
void GameInfo::BuildAudioClipArray()
{
    // TODO: before BuildAudioClipArray() is called, AudioClipCount
    // is set to 0; the function may be called more than once in a sequence,
    // and AudioClipCount retains its value between calls.
    // This has to be rewritten in a more clean and explicit way.
    // For now one should take into consideration, that AudioClips array is
    // already constructed at this time, and it is larger, than AudioClipCount
    // (about 1000 elements).

    char temp_name[30];
    int temp_number;
    char temp_extension[10];

    int number_of_files = Common::AssetManager::GetAssetCount();
    int i;
    for (i = 0; i < number_of_files; i++)
    {
        if (sscanf(Common::AssetManager::GetAssetFileByIndex(i), "%5s%d.%3s", temp_name, &temp_number, temp_extension) == 3)
        {
            if (stricmp(temp_name, "music") == 0)
            {
                sprintf(AudioClips[AudioClipCount].scriptName, "aMusic%d", temp_number);
                sprintf(AudioClips[AudioClipCount].fileName, "music%d.%s", temp_number, temp_extension);
                AudioClips[AudioClipCount].bundlingType = (stricmp(temp_extension, "mid") == 0) ? AUCL_BUNDLE_EXE : AUCL_BUNDLE_VOX;
                AudioClips[AudioClipCount].type = 2;
                AudioClips[AudioClipCount].defaultRepeat = 1;
            }
            else if (stricmp(temp_name, "sound") == 0)
            {
                sprintf(AudioClips[AudioClipCount].scriptName, "aSound%d", temp_number);
                sprintf(AudioClips[AudioClipCount].fileName, "sound%d.%s", temp_number, temp_extension);
                AudioClips[AudioClipCount].bundlingType = AUCL_BUNDLE_EXE;
                AudioClips[AudioClipCount].type = 3;
            }
            else
            {
                continue;
            }

            AudioClips[AudioClipCount].defaultVolume = 100;
            AudioClips[AudioClipCount].defaultPriority = 50;
            AudioClips[AudioClipCount].id = AudioClipCount;

            if (stricmp(temp_extension, "mp3") == 0)
                AudioClips[AudioClipCount].fileType = eAudioFileMP3;
            else if (stricmp(temp_extension, "wav") == 0)
                AudioClips[AudioClipCount].fileType = eAudioFileWAV;
            else if (stricmp(temp_extension, "voc") == 0)
                AudioClips[AudioClipCount].fileType = eAudioFileVOC;
            else if (stricmp(temp_extension, "mid") == 0)
                AudioClips[AudioClipCount].fileType = eAudioFileMIDI;
            else if ((stricmp(temp_extension, "mod") == 0) || (stricmp(temp_extension, "xm") == 0)
                || (stricmp(temp_extension, "s3m") == 0) || (stricmp(temp_extension, "it") == 0))
                AudioClips[AudioClipCount].fileType = eAudioFileMOD;
            else if (stricmp(temp_extension, "ogg") == 0)
                AudioClips[AudioClipCount].fileType = eAudioFileOGG;

            AudioClipCount++;
        }
    }
}


void GameInfo::ReadExtFromFile_Part1(Common::Stream *in, GAME_STRUCT_READ_DATA &read_data)
{
    read_savegame_info(in, read_data);
    read_font_flags(in, read_data);
    read_sprite_flags(in, read_data);
    ReadInvInfo_Aligned(in);
    read_cursors(in, read_data);
    read_interaction_scripts(in, read_data);
    read_words_dictionary(in, read_data);
}

void GameInfo::ReadExtFromFile_Part2(Common::Stream *in, GAME_STRUCT_READ_DATA &read_data)
{
   read_characters(in, read_data);
   read_lipsync(in, read_data);
   read_messages(in, read_data);
}

void GameInfo::ReadExtFromFile_Part3(Common::Stream *in, GAME_STRUCT_READ_DATA &read_data)
{
    read_customprops(in, read_data);
    read_audio(in, read_data);
    read_room_names(in, read_data);
}

//-----------------------------------------------------------------------------
// Reading Part 1

void GameInfo::read_savegame_info(Common::Stream *in, GAME_STRUCT_READ_DATA &read_data)
{
    if (read_data.filever > kGameVersion_272) // only 3.x
    {
        Guid.ReadCount(in, MAX_GUID_LENGTH);
        SavedGameFileExtension.ReadCount(in, MAX_SG_EXT_LENGTH);
        SavedGameFolderName.ReadCount(in, MAX_SG_FOLDER_LEN);

        if (SavedGameFileExtension.IsEmpty())
            read_data.saveGameSuffix[0] = 0;
        else
            sprintf(read_data.saveGameSuffix, ".%s", SavedGameFileExtension.GetCStr());            
    }
}

void GameInfo::read_font_flags(Common::Stream *in, GAME_STRUCT_READ_DATA &read_data)
{
    FontFlags.ReadRaw(in, FontCount);
    FontOutline.ReadRaw(in, FontCount);
}

void GameInfo::read_sprite_flags(Common::Stream *in, GAME_STRUCT_READ_DATA &read_data)
{
    int numToRead;
    if (read_data.filever < kGameVersion_256)
        numToRead = 6000; // Fixed number of sprites on < 2.56
    else
        numToRead = in->ReadInt32();

    if (numToRead > MAX_SPRITES) {
        quit("Too many sprites; need newer AGS version");
    }
    SpriteFlags.ReadRaw(in, numToRead);
}

void GameInfo::ReadInvInfo_Aligned(Stream *in)
{
    AlignedStream align_s(in, Common::kAligned_Read);
    InventoryItems.New(InvItemCount);
    for (int iteratorCount = 0; iteratorCount < InvItemCount; ++iteratorCount)
    {
        InventoryItems[iteratorCount].ReadFromFile_v321(&align_s);
        align_s.Reset();
    }
}

void GameInfo::WriteInvInfo_Aligned(Stream *out)
{
    AlignedStream align_s(out, Common::kAligned_Write);
    for (int iteratorCount = 0; iteratorCount < InvItemCount; ++iteratorCount)
    {
        InventoryItems[iteratorCount].WriteToFile_v321(&align_s);
        align_s.Reset();
    }
}

void GameInfo::read_cursors(Common::Stream *in, GAME_STRUCT_READ_DATA &read_data)
{
    if (MouseCursorCount > MAX_CURSOR)
        quit("Too many cursors: need newer AGS version");

    ReadMouseCursors_Aligned(in);

    if (read_data.filever <= kGameVersion_272) // 2.x
    {
        // Change cursor.view from 0 to -1 for non-animating cursors.
        int i;
        for (i = 0; i < MouseCursorCount; i++)
        {
            if (MouseCursors[i].view == 0)
                MouseCursors[i].view = -1;
        }
    }
}

void GameInfo::read_interaction_scripts(Common::Stream *in, GAME_STRUCT_READ_DATA &read_data)
{
    numGlobalVars = 0;

    if (read_data.filever > kGameVersion_272) // 3.x
    {
        int bb;

        CharacterInteractionScripts.New(CharacterCount);
        InvItemInteractionScripts.New(InvItemCount);
        for (bb = 0; bb < CharacterCount; bb++) {
            CharacterInteractionScripts[bb] = new InteractionScripts();
            deserialize_interaction_scripts(in, CharacterInteractionScripts[bb]);
        }
        for (bb = 1; bb < InvItemCount; bb++) {
            InvItemInteractionScripts[bb] = new InteractionScripts();
            deserialize_interaction_scripts(in, InvItemInteractionScripts[bb]);
        }
    }
    else // 2.x
    {
        int bb;

        CharacterInteractions.New(CharacterCount);
        InvItemInteractions.New(InvItemCount);

        for (bb = 0; bb < CharacterCount; bb++) {
            CharacterInteractions[bb] = deserialize_new_interaction(in);
        }
        for (bb = 0; bb < InvItemCount; bb++) {
            InvItemInteractions[bb] = deserialize_new_interaction(in);
        }

        numGlobalVars = in->ReadInt32();
        for (bb = 0; bb < numGlobalVars; bb++) {
            globalvars[bb].ReadFromFile(in);
        }
    }
}

void GameInfo::read_words_dictionary(Common::Stream *in, GAME_STRUCT_READ_DATA &read_data)
{
    if (LoadDictionary) {
        Dictionary = (WordsDictionary*)malloc(sizeof(WordsDictionary));
        read_dictionary (Dictionary, in);
    }
}

void GameInfo::ReadMouseCursors_Aligned(Stream *in)
{
    AlignedStream align_s(in, Common::kAligned_Read);
    MouseCursors.New(MouseCursorCount);
    for (int iteratorCount = 0; iteratorCount < MouseCursorCount; ++iteratorCount)
    {
        MouseCursors[iteratorCount].ReadFromFile(&align_s);
        align_s.Reset();
    }
}

void GameInfo::WriteMouseCursors_Aligned(Stream *out)
{
    AlignedStream align_s(out, Common::kAligned_Write);
    for (int iteratorCount = 0; iteratorCount < MouseCursorCount; ++iteratorCount)
    {
        MouseCursors[iteratorCount].WriteToFile(&align_s);
        align_s.Reset();
    }
}

//-----------------------------------------------------------------------------
// Reading Part 2

void GameInfo::read_characters(Common::Stream *in, GAME_STRUCT_READ_DATA &read_data)
{
    Characters.New(CharacterCount + 5); // CHECKME: what is this +5 for? sort of safety thing??

    ReadCharacters_Aligned(in);

    //charcache = (CharacterCache*)calloc(1,sizeof(CharacterCache)*CharacterCount+5);

    if (read_data.filever <= kGameVersion_272) // fixup charakter script names for 2.x (EGO -> cEgo)
    {
        char tempbuffer[200];
        for (int i = 0; i < CharacterCount; i++)
        {
            memset(tempbuffer, 0, sizeof(tempbuffer));
            tempbuffer[0] = 'c';
            tempbuffer[1] = Characters[i].scrname[0];
            strcat(&tempbuffer[2], strlwr(&Characters[i].scrname[1]));
            strcpy(Characters[i].scrname, tempbuffer);
        }
    }

    if (read_data.filever <= kGameVersion_300) // fix character walk speed for < 3.1.1
    {
        for (int i = 0; i < CharacterCount; i++)
        {
            if (Options[OPT_ANTIGLIDE])
                Characters[i].flags |= CHF_ANTIGLIDE;
        }
    }

    // Characters can always walk through each other on < 2.54
    if (read_data.filever < kGameVersion_254)
    {
        for (int i = 0; i < CharacterCount; i++)
        {
            Characters[i].flags |= CHF_NOBLOCKING;
        }
    }
}

void GameInfo::read_lipsync(Common::Stream *in, GAME_STRUCT_READ_DATA &read_data)
{
    if (read_data.filever >= kGameVersion_254) // lip syncing was introduced in 2.54
    {
        LipSyncFrameLetters.ReadRaw(in, MAXLIPSYNCFRAMES);
    }
}

void GameInfo::read_messages(Common::Stream *in, GAME_STRUCT_READ_DATA &read_data)
{
    char msg_buffer[500];
    GlobalMessages.New(MAXGLOBALMES);
    for (int ee=0;ee<MAXGLOBALMES;ee++) {
        if (!MessageToLoad[ee])
        {
            continue;
        }

        if (read_data.filever < kGameVersion_261) // Global GlobalMessages are not encrypted on < 2.61
        {
            // FIXME: this is a common algorythm (use String::Read?)
            char* nextchar = msg_buffer;
            while (1)
            {
                *nextchar = in->ReadInt8();
                if (*nextchar == 0)
                    break;
                nextchar++;
            }
            GlobalMessages[ee] = msg_buffer;
        }
        else
        {
            read_string_decrypt(in, msg_buffer);
            GlobalMessages[ee] = msg_buffer;
        }
    }
}

void GameInfo::ReadCharacters_Aligned(Stream *in)
{
    AlignedStream align_s(in, Common::kAligned_Read);
    for (int iteratorCount = 0; iteratorCount < CharacterCount; ++iteratorCount)
    {
        Characters[iteratorCount].ReadFromFile_v321(&align_s);
        align_s.Reset();
    }
}

void GameInfo::WriteCharacters_Aligned(Stream *out)
{
    AlignedStream align_s(out, Common::kAligned_Write);
    for (int iteratorCount = 0; iteratorCount < CharacterCount; ++iteratorCount)
    {
        Characters[iteratorCount].WriteToFile_v321(&align_s);
        align_s.Reset();
    }
}

//-----------------------------------------------------------------------------
// Reading Part 3

void GameInfo::read_customprops(Common::Stream *in, GAME_STRUCT_READ_DATA &read_data)
{
    if (read_data.filever >= kGameVersion_260) // >= 2.60
    {
        if (PropertySchema.UnSerialize(in))
            quit("load room: unable to deserialize prop schema");

        CharacterProperties.New(CharacterCount);
        InvItemProperties.New(InvItemCount);

        int errors = 0;
        int bb;

        for (bb = 0; bb < CharacterCount; bb++)
            errors += CharacterProperties[bb].UnSerialize (in);
        for (bb = 0; bb < InvItemCount; bb++)
            errors += InvItemProperties[bb].UnSerialize (in);

        if (errors > 0)
            quit("LoadGame: errors encountered reading custom props");

        ViewNames.New(ViewCount);
        InventoryScriptNames.New(InvItemCount);
        DialogScriptNames.New(DialogCount);

        for (bb = 0; bb < ViewCount; bb++)
            ViewNames[bb].Read(in, MAXVIEWNAMELENGTH);

        for (bb = 0; bb < InvItemCount; bb++)
            InventoryScriptNames[bb].Read(in, MAX_SCRIPT_NAME_LEN);

        for (bb = 0; bb < DialogCount; bb++)
            DialogScriptNames[bb].Read(in, MAX_SCRIPT_NAME_LEN);
    }
}

void GameInfo::read_audio(Common::Stream *in, GAME_STRUCT_READ_DATA &read_data)
{
    int i;
    if (read_data.filever >= kGameVersion_320)
    {
        AudioClipTypeCount = in->ReadInt32();

        if (AudioClipTypeCount > read_data.max_audio_types)
            quit("LoadGame: too many audio types");

        AudioClipTypes.New(AudioClipTypeCount);
        for (i = 0; i < AudioClipTypeCount; ++i)
        {
            AudioClipTypes[i].ReadFromFile(in);
        }

        AudioClipCount = in->ReadInt32();
        AudioClips.New(AudioClipCount);
        ReadAudioClips_Aligned(in);
        
        //play.ScoreSoundIndex = in->ReadInt32();
        read_data.score_sound = in->ReadInt32();
    }
    else
    {
        // Create soundClips and AudioClipTypes structures.
        // TODO: refactor loading to deal without allocating 1000 clips right away
        AudioClipCount = 1000;
        AudioClipTypeCount = 4;
        AudioClipTypes.New(AudioClipTypeCount);
        AudioClips.New(AudioClipCount);

        int i;
        for (i = 0; i < 4; i++)
        {
            AudioClipTypes[i].reservedChannels = 1;
            AudioClipTypes[i].id = i;
            AudioClipTypes[i].volume_reduction_while_speech_playing = 10;
        }
        AudioClipTypes[3].reservedChannels = 0;

        AudioClipCount = 0;
        if (Common::AssetManager::SetDataFile("music.vox") == Common::kAssetNoError)
            BuildAudioClipArray();

        Common::AssetManager::SetDataFile(read_data.game_file_name);
        BuildAudioClipArray();

        AudioClips.SetLength(AudioClipCount);
    }
}

// Temporarily copied this from acruntim.h;
// it is unknown if this should be defined for all solution, or only runtime
#define STD_BUFFER_SIZE 3000

void GameInfo::read_room_names(Stream *in, GAME_STRUCT_READ_DATA &read_data)
{
    if ((read_data.filever >= kGameVersion_pre300) && (Options[OPT_DEBUGMODE] != 0))
    {
        RoomCount = in->ReadInt32();
        RoomNumbers.New(RoomCount);
        RoomNames.New(RoomCount);
        String pexbuf;
        for (int bb = 0; bb < RoomCount; bb++)
        {
            RoomNumbers[bb] = in->ReadInt32();
            pexbuf.Read(in, STD_BUFFER_SIZE);
            // We do not use assignment operator here, because that
            // would add a reference to buffer, and we don't want
            // to have 3000 chars-long name for every room.
            RoomNames[bb].SetString(pexbuf);
        }
    }
    else
    {
        RoomCount = 0;
    }
}

void GameInfo::ReadAudioClips_Aligned(Common::Stream *in)
{
    AlignedStream align_s(in, Common::kAligned_Read);
    for (int i = 0; i < AudioClipCount; ++i)
    {
        AudioClips[i].ReadFromFile(&align_s);
        align_s.Reset();
    }
}

void GameInfo::ReadFromSavedGame_v321(Stream *in)
{
    int bb;

    ReadInvInfo_Aligned(in);
    ReadMouseCursors_Aligned(in);

    if (InvItemInteractionScripts.IsEmpty())
    {
        for (bb = 0; bb < InvItemCount; bb++)
            in->ReadArrayOfInt32(&InvItemInteractions[bb]->timesRun[0], MAX_NEWINTERACTION_EVENTS);
        for (bb = 0; bb < CharacterCount; bb++)
            in->ReadArrayOfInt32 (&CharacterInteractions[bb]->timesRun[0], MAX_NEWINTERACTION_EVENTS);
    }

    in->ReadArrayOfInt32(&Options[0], OPT_HIGHESTOPTION+1);
    Options[OPT_LIPSYNCTEXT] = in->ReadByte();

    ReadCharacters_Aligned(in);
}

void GameInfo::WriteForSavedGame_v321(Stream *out)
{
    WriteInvInfo_Aligned(out);
    WriteMouseCursors_Aligned(out);

    if (InvItemInteractionScripts.IsEmpty())
    {
      int bb;
      for (bb = 0; bb < InvItemCount; bb++)
        out->WriteArrayOfInt32 (&InvItemInteractions[bb]->timesRun[0], MAX_NEWINTERACTION_EVENTS);
      for (bb = 0; bb < CharacterCount; bb++)
        out->WriteArrayOfInt32 (&CharacterInteractions[bb]->timesRun[0], MAX_NEWINTERACTION_EVENTS); 
    }

    out->WriteArrayOfInt32 (&Options[0], OPT_HIGHESTOPTION+1);
    out->WriteInt8 (Options[OPT_LIPSYNCTEXT]);

    WriteCharacters_Aligned(out);
}

void GameInfo::ReadFromSavedGame(Stream *in)
{
    in->ReadArrayOfInt32(Options, OPT_HIGHESTOPTION + 1);
    Options[OPT_LIPSYNCTEXT] = in->ReadInt8();
    in->Read(PaletteUses, 256);
    PlayerCharacterIndex     = in->ReadInt32();
    DialogBulletSprIndex     = in->ReadInt32();
    InvItemHotDotColor       = in->ReadInt16();
    InvItemHotDotOuterColor  = in->ReadInt16();
    DefaultLipSyncFrame      = in->ReadInt32();
    InvItemHotDotSprIndex    = in->ReadInt32();
}

void GameInfo::WriteForSavedGame(Stream *out)
{
    out->WriteArrayOfInt32(Options, OPT_HIGHESTOPTION + 1);
    out->WriteInt8(Options[OPT_LIPSYNCTEXT]);
    out->Write(PaletteUses, 256);
    out->WriteInt32(PlayerCharacterIndex);
    out->WriteInt32(DialogBulletSprIndex);
    out->WriteInt16(InvItemHotDotColor);
    out->WriteInt16(InvItemHotDotOuterColor);
    out->WriteInt32(DefaultLipSyncFrame);
    out->WriteInt32(InvItemHotDotSprIndex);
}
