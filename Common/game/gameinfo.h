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
//
// GameInfo, a class of static game data
//
//=============================================================================
#ifndef __AGS_CN_GAME__AGSGAME_H
#define __AGS_CN_GAME__AGSGAME_H

#include "ac/audiocliptype.h"
#include "ac/characterinfo.h"
#include "ac/gamestructdefines.h"
#include "ac/inventoryiteminfo.h"
#include "ac/mousecursor.h"
#include "ac/dynobj/scriptaudioclip.h"
#include "game/customproperties.h"
#include "util/array.h"
#include "util/string.h"
#include "util/wgt2allg.h"

struct ccScript;
struct InteractionScripts;
struct NewInteraction;
struct WordsDictionary;

namespace AGS
{
namespace Common
{

class GameInfo
{
public:
    GameInfo();
    ~GameInfo();

    void ReadBaseFromFile(Stream *in);
    void WriteBaseToFile(Stream *out);

    // [IKM] Game struct loading code is moved here from Engine's load_game_file
    // function; for now it is not supposed to be called by Editor; although it
    // is possible that eventually will be.
    //
    // Since reading game data is made in a bit inconvenient way I had to
    // a) divide process into three functions (there's some extra stuff
    // being read between them;
    // b) use a helper struct to pass some arguments
    //
    // This should be changed in future
    struct GAME_STRUCT_READ_DATA
    {
        // in
        int  filever;
        int  max_audio_types;
        String game_file_name;

        // out
        char *saveGameSuffix;
        int  score_sound;
    };

    void BuildAudioClipArray();

    void ReadExtFromFile_Part1(Stream *in, GAME_STRUCT_READ_DATA &read_data);
    void ReadExtFromFile_Part2(Stream *in, GAME_STRUCT_READ_DATA &read_data);
    void ReadExtFromFile_Part3(Stream *in, GAME_STRUCT_READ_DATA &read_data);

    // Functions for reading and writing appropriate data from/to save game
    void ReadFromSavedGame_v321(Stream *in);
    void WriteForSavedGame_v321(Stream *out);
    void ReadFromSavedGame(Stream *in);
    void WriteForSavedGame(Stream *out);

    // TODO: temporarily made public
    void ReadInvInfo_Aligned(Stream *in);
    void ReadMouseCursors_Aligned(Stream *in);
    void ReadCharacters_Aligned(Stream *in);
    void WriteInvInfo_Aligned(Stream *out);
    void WriteMouseCursors_Aligned(Stream *out);
    void WriteCharacters_Aligned(Stream *out);

private:
    //------------------------------
    // Part 1
    void read_savegame_info(Stream *in, GAME_STRUCT_READ_DATA &read_data);
    void read_font_flags(Stream *in, GAME_STRUCT_READ_DATA &read_data);
    void read_sprite_flags(Stream *in, GAME_STRUCT_READ_DATA &read_data);
    void read_cursors(Stream *in, GAME_STRUCT_READ_DATA &read_data);
    void read_interaction_scripts(Stream *in, GAME_STRUCT_READ_DATA &read_data);
    void read_words_dictionary(Stream *in, GAME_STRUCT_READ_DATA &read_data);
    
    //------------------------------
    // Part 2
    void read_characters(Stream *in, GAME_STRUCT_READ_DATA &read_data);
    void read_lipsync(Stream *in, GAME_STRUCT_READ_DATA &read_data);
    void read_messages(Stream *in, GAME_STRUCT_READ_DATA &read_data);

    //------------------------------
    // Part 3
    void read_customprops(Stream *in, GAME_STRUCT_READ_DATA &read_data);
    void read_audio(Stream *in, GAME_STRUCT_READ_DATA &read_data);
    void read_room_names(Stream *in, GAME_STRUCT_READ_DATA &read_data);

    void ReadAudioClips_Aligned(Stream *in);
    //--------------------------------------------------------------------

// TODO: all members are currently public; hide them later
public:
    //-----------------------------------------------------
    // Former members of GameSetupStructBase
    //-----------------------------------------------------
    String          GameName;
    int32_t         Options[100];
    uint8_t         PaletteUses[256];
    color           DefaultPalette[256];
    int32_t         PlayerCharacterIndex; // TODO: use character pointer instead
    int32_t         TotalScore; // max game score points
    int32_t         ColorDepth; // in bytes per pixel
    int32_t         DialogBulletSprIndex;
    uint16_t        InvItemHotDotColor; // inventory item cursor hotspot dot
    uint16_t        InvItemHotDotOuterColor;
    int32_t         UniqueId; // random key identifying the game
    // TODO: redo this into something less unclear and hard-coded
    int32_t         DefaultResolution; // 0=undefined, 1=320x200, 2=320x240, 3=640x400 etc
    int32_t         DefaultLipSyncFrame; // used for unknown chars
    int32_t         InvItemHotDotSprIndex;
    ObjectArray<String> GlobalMessages;
    WordsDictionary *Dictionary; // for the text parser
    Array<CharacterInfo> Characters;

    // Currently used in AGS.Native only:
    ccScript        *CompiledScript;

    // TODO: remove these, after refactoring the game load process
    int32_t         ViewCount;
    int32_t         CharacterCount;
    int16_t         InvItemCount;
    int32_t         DialogCount;
    int32_t         DialogMessageCount;
    int32_t         FontCount;
    int32_t         GuiCount;
    int32_t         MouseCursorCount;
    int32_t         MessageToLoad[MAXGLOBALMES];
    bool            LoadDictionary;
    int32_t         RoomCount;
    bool            LoadCompiledScript;

    //-----------------------------------------------------
    // Former members of GameSetupStruct
    //-----------------------------------------------------
    Array<uint8_t>              FontFlags;
    Array<int8_t>               FontOutline;
    Array<uint8_t>              SpriteFlags;
    ObjectArray<InventoryItemInfo> InventoryItems;
    ObjectArray<MouseCursor>    MouseCursors;
    Array<NewInteraction*>      CharacterInteractions;
    Array<NewInteraction*>      InvItemInteractions;
    Array<InteractionScripts*>  CharacterInteractionScripts;
    Array<InteractionScripts*>  InvItemInteractionScripts;
    int32_t                     FileVersion; // just used by editor
    Array<char[50]>             LipSyncFrameLetters;
    CustomPropertySchema        PropertySchema;
    ObjectArray<CustomProperties> CharacterProperties;
    ObjectArray<CustomProperties> InvItemProperties;
    ObjectArray<String>         ViewNames;
    ObjectArray<String>         InventoryScriptNames;
    ObjectArray<String>         DialogScriptNames;
    String                      Guid;
    String                      SavedGameFileExtension;
    String                      SavedGameFolderName;
    Array<int32_t>              RoomNumbers;
    ObjectArray<String>         RoomNames;
    Array<ScriptAudioClip>      AudioClips;
    Array<AudioClipType>        AudioClipTypes;

    // TODO: remove this later, after audio clips rebuild procedure is
    // rewritten in a better way
    int32_t                     AudioClipTypeCount;
    int32_t                     AudioClipCount;
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_GAME__AGSGAME_H
