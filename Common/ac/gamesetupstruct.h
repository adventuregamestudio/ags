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
//
//
//=============================================================================
#ifndef __AGS_CN_AC__GAMESETUPSTRUCT_H
#define __AGS_CN_AC__GAMESETUPSTRUCT_H

#include <vector>
#include "ac/audiocliptype.h"        // AudioClipType
#include "ac/game_version.h"
#include "ac/inventoryiteminfo.h"   // InventoryItemInfo
#include "ac/mousecursor.h"      // MouseCursor
#include "ac/gamesetupstructbase.h"
#include "ac/oldgamesetupstruct.h"
#include "ac/dynobj/scriptaudioclip.h" // ScriptAudioClip
#include "game/customproperties.h"
#include "game/interactions.h"
#include "game/main_game_file.h"

namespace AGS { namespace Common { struct AssetLibInfo; } }

using AGS::Common::Interaction;
using AGS::Common::InteractionScripts;
using AGS::Common::HGameFileError;

// TODO: split GameSetupStruct into struct used to hold loaded game data, and actual runtime object
struct GameSetupStruct: public GameSetupStructBase {
    // These arrays are used only to read data into;
    // font parameters are then put and queried in the fonts module
    unsigned char     fontflags[MAX_FONTS];
    char              fontoutline[MAX_FONTS];
    int               fontvoffset[MAX_FONTS]; // vertical font offset
    int               fontlnspace[MAX_FONTS]; // font's line spacing (0 = default)
    //
    unsigned char     spriteflags[MAX_SPRITES];
    InventoryItemInfo invinfo[MAX_INV];
    MouseCursor       mcurs[MAX_CURSOR];
    Interaction     **intrChar;
    Interaction      *intrInv[MAX_INV];
    InteractionScripts **charScripts;
    InteractionScripts **invScripts;
    // TODO: why we do not use this in the engine instead of
    // loaded_game_file_version?
    int               filever;  // just used by editor
    char              lipSyncFrameLetters[MAXLIPSYNCFRAMES][50];
    AGS::Common::PropertySchema propSchema;
    std::vector<AGS::Common::StringIMap> charProps;
    AGS::Common::StringIMap invProps[MAX_INV];
    // NOTE: although the view names are stored in game data, they are never
    // used, nor registered as script exports; numeric IDs are used to
    // reference views instead.
    std::vector<Common::String> viewNames;
    Common::String    invScriptNames[MAX_INV];
    std::vector<Common::String> dialogScriptNames;
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
    // A clip to play when player gains score in game
    // TODO: find out why OPT_SCORESOUND option cannot be used to store this in >=3.2 games
    int               scoreClipID;

    // Get game's native color depth
    inline int GetColorDepth() const { return color_depth * 8; }


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

    //--------------------------------------------------------------------
    // Do not call these directly
    //------------------------------
    // Part 1
    void read_savegame_info(Common::Stream *in, GameDataVersion data_ver);
    void read_font_flags(Common::Stream *in, GameDataVersion data_ver);
    HGameFileError read_sprite_flags(Common::Stream *in, GameDataVersion data_ver);
    HGameFileError read_cursors(Common::Stream *in, GameDataVersion data_ver);
    void read_interaction_scripts(Common::Stream *in, GameDataVersion data_ver);
    void read_words_dictionary(Common::Stream *in);

    void ReadInvInfo_Aligned(Common::Stream *in);
    void WriteInvInfo_Aligned(Common::Stream *out);
    void ReadMouseCursors_Aligned(Common::Stream *in);
    void WriteMouseCursors_Aligned(Common::Stream *out);
    //------------------------------
    // Part 2
    void read_characters(Common::Stream *in, GameDataVersion data_ver);
    void read_lipsync(Common::Stream *in, GameDataVersion data_ver);
    void read_messages(Common::Stream *in, GameDataVersion data_ver);

    void ReadCharacters_Aligned(Common::Stream *in);
    void WriteCharacters_Aligned(Common::Stream *out);
    //------------------------------
    // Part 3
    HGameFileError read_customprops(Common::Stream *in, GameDataVersion data_ver);
    HGameFileError read_audio(Common::Stream *in, GameDataVersion data_ver);
    void read_room_names(Common::Stream *in, GameDataVersion data_ver);

    void ReadAudioClips_Aligned(Common::Stream *in);
    //--------------------------------------------------------------------

    // Functions for reading and writing appropriate data from/to save game
    void ReadFromSaveGame_v321(Common::Stream *in, char* gswas, ccScript* compsc, CharacterInfo* chwas,
                                   WordsDictionary *olddict, char** mesbk);

    void ReadFromSavegame(Common::PStream in);
    void WriteForSavegame(Common::PStream out);
};

//=============================================================================

// TODO: find out how this function was supposed to be used
void ConvertOldGameStruct (OldGameSetupStruct *ogss, GameSetupStruct *gss);
// Finds an audio clip using legacy convention index
ScriptAudioClip* GetAudioClipForOldStyleNumber(GameSetupStruct &game, bool is_music, int num);

#endif // __AGS_CN_AC__GAMESETUPSTRUCT_H
