
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_CN_AC__GAMESETUPSTRUCT_H
#define __AGS_CN_AC__GAMESETUPSTRUCT_H

#include "ac/audiocliptype.h"        // AudioClipType
#include "ac/customproperties.h" // CustomProperties, CustomPropertySchema
#include "ac/interaction.h"      // NewInteraction
#include "ac/inventoryiteminfo.h"   // InventoryItemInfo
#include "ac/mousecursor.h"      // MouseCursor
#include "ac/gamesetupstructbase.h"
#include "ac/oldgamesetupstruct.h"
#include "ac/dynobj/scriptaudioclip.h" // ScriptAudioClip

// [IKM] Do not change the order of variables in this struct!
// Until the serialization and script referencing methods are improved
// game execution will depend on actual object addresses in memory
//
struct GameSetupStruct: public GameSetupStructBase {
    unsigned char     fontflags[MAX_FONTS];
    char              fontoutline[MAX_FONTS];
    unsigned char     spriteflags[MAX_SPRITES];
    InventoryItemInfo invinfo[MAX_INV];
    MouseCursor       mcurs[MAX_CURSOR];
    NewInteraction   **intrChar;
    NewInteraction   *intrInv[MAX_INV];
    InteractionScripts **charScripts;
    InteractionScripts **invScripts;
    int               filever;  // just used by editor
    char              lipSyncFrameLetters[MAXLIPSYNCFRAMES][50];
    CustomPropertySchema propSchema;
    CustomProperties  *charProps, invProps[MAX_INV];
    char              **viewNames;
    char              invScriptNames[MAX_INV][MAX_SCRIPT_NAME_LEN];
    char              dialogScriptNames[MAX_DIALOG][MAX_SCRIPT_NAME_LEN];
    char              guid[MAX_GUID_LENGTH];
    char              saveGameFileExtension[MAX_SG_EXT_LENGTH];
    char              saveGameFolderName[MAX_SG_FOLDER_LEN];
    int               roomCount;
    int              *roomNumbers;
    char            **roomNames;
    int               audioClipCount;
    ScriptAudioClip  *audioClips;
    int               audioClipTypeCount;
    AudioClipType    *audioClipTypes;



    // [IKM] Game struct loading code is moved here from Engine's load_game_file
    // function; for now it is not supposed to be called by Editor; although it
    // is possible that eventually will be.
    //
    // Since reading game data is made in a bit inconvenient way I had to
    // a) divide process into three functions (there's some extra stuff
    // being read between them;
    // b) use a helper struct to pass some arguments
    //
    // I also had to move BuildAudioClipArray from the engine and make it
    // GameSetupStruct member.
    // Anyway, I believe that read/write/init functions should be in the
    // class regardless it is shared by both engine and editor or not.
    struct GAME_STRUCT_READ_DATA
    {
        // in
        int  filever;
        int  max_audio_types;
        char *game_file_name;

        // out
        char *saveGameSuffix;
        int  score_sound;
    };

    void BuildAudioClipArray();

    void ReadFromFile_Part1(Common::DataStream *in, GAME_STRUCT_READ_DATA &read_data);
    void ReadFromFile_Part2(Common::DataStream *in, GAME_STRUCT_READ_DATA &read_data);
    void ReadFromFile_Part3(Common::DataStream *in, GAME_STRUCT_READ_DATA &read_data);
    //--------------------------------------------------------------------
    // Do not call these directly
    //------------------------------
    // Part 1
    void read_savegame_info(Common::DataStream *in, GAME_STRUCT_READ_DATA &read_data);
    void read_font_flags(Common::DataStream *in, GAME_STRUCT_READ_DATA &read_data);
    void read_sprite_flags(Common::DataStream *in, GAME_STRUCT_READ_DATA &read_data);
    void read_invinfo(Common::DataStream *in, GAME_STRUCT_READ_DATA &read_data);
    void read_cursors(Common::DataStream *in, GAME_STRUCT_READ_DATA &read_data);
    void read_interaction_scripts(Common::DataStream *in, GAME_STRUCT_READ_DATA &read_data);
    void read_words_dictionary(Common::DataStream *in, GAME_STRUCT_READ_DATA &read_data);
    //------------------------------
    // Part 2
    void read_characters(Common::DataStream *in, GAME_STRUCT_READ_DATA &read_data);
    void read_lipsync(Common::DataStream *in, GAME_STRUCT_READ_DATA &read_data);
    void read_messages(Common::DataStream *in, GAME_STRUCT_READ_DATA &read_data);
    //------------------------------
    // Part 3
    void read_customprops(Common::DataStream *in, GAME_STRUCT_READ_DATA &read_data);
    void read_audio(Common::DataStream *in, GAME_STRUCT_READ_DATA &read_data);
    void read_room_names(Common::DataStream *in, GAME_STRUCT_READ_DATA &read_data);
    //--------------------------------------------------------------------

    // Functions for reading and writing appropriate data from/to save game
    void ReadFromSaveGame(Common::DataStream *in, char* gswas, ccScript* compsc, CharacterInfo* chwas,
                                   WordsDictionary *olddict, char** mesbk);
    void WriteForSaveGame(Common::DataStream *out);
};

//=============================================================================

void ConvertOldGameStruct (OldGameSetupStruct *ogss, GameSetupStruct *gss);

#endif // __AGS_CN_AC__GAMESETUPSTRUCT_H
