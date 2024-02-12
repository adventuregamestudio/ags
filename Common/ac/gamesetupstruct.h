//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// GameSetupStruct is a contemporary main game data. 
//
//=============================================================================
#ifndef __AGS_CN_AC__GAMESETUPSTRUCT_H
#define __AGS_CN_AC__GAMESETUPSTRUCT_H

#include <vector>
#include "ac/audiocliptype.h"
#include "ac/characterinfo.h" // TODO: constants to separate header
#include "ac/gamesetupstructbase.h"
#include "ac/inventoryiteminfo.h"
#include "ac/mousecursor.h"
#include "ac/dynobj/scriptaudioclip.h"
#include "game/customproperties.h"
#include "game/main_game_file.h" // TODO: constants to separate header or split out reading functions

namespace AGS
{
    namespace Common
    {
        struct AssetLibInfo;
        struct Interaction;
        struct InteractionScripts;
        typedef std::shared_ptr<Interaction> PInteraction;
        typedef std::shared_ptr<InteractionScripts> PInteractionScripts;
    }
}

using AGS::Common::PInteraction;
using AGS::Common::PInteractionScripts;
using AGS::Common::HGameFileError;


// TODO: split GameSetupStruct into struct used to hold loaded game data, and actual runtime object
struct GameSetupStruct : public GameSetupStructBase
{
    // This array is used only to read data into;
    // font parameters are then put and queried in the fonts module
    // TODO: split into installation params (used only when reading) and runtime params
    std::vector<FontInfo> fonts;
    InventoryItemInfo invinfo[MAX_INV]{};
    std::vector<MouseCursor> mcurs;
    std::vector<PInteraction> intrChar;
    PInteraction intrInv[MAX_INV];
    std::vector<PInteractionScripts> charScripts;
    std::vector<PInteractionScripts> invScripts;
    // TODO: why we do not use this in the engine instead of
    // loaded_game_file_version?
    int               filever;  // just used by editor
    Common::String    compiled_with; // version of AGS this data was created by
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
    // NOTE: saveGameFolderName is generally used to create game subdirs in common user directories
    Common::String    saveGameFolderName;
    int               roomCount;
    std::vector<int>  roomNumbers;
    std::vector<Common::String> roomNames;
    std::vector<ScriptAudioClip> audioClips;
    std::vector<AudioClipType> audioClipTypes;
    // A clip to play when player gains score in game
    // NOTE: this stores an internal audio clip index, which may or not correspond
    // to the OPT_SCORESOUND, and also depends on whether the game was upgraded from <3.2 data.
    int               scoreClipID;
    // number of accessible game audio channels (the ones under direct user control)
    int               numGameChannels = 0;
    // backward-compatible channel limit that may be exported to script and reserved by audiotypes
    int               numCompatGameChannels = 0;
    
    // TODO: I converted original array of sprite infos to vector here, because
    // statistically in most games sprites go in long continious sequences with minimal
    // gaps, and standard hash-map will have relatively big memory overhead compared.
    // Of course vector will not behave very well if user has created e.g. only
    // sprite #1 and sprite #1000000. For that reason I decided to still limit static
    // sprite count to some reasonable number for the time being. Dynamic sprite IDs are
    // added in sequence, so there won't be any issue with these.
    // There could be other collection types, more optimal for this case. For example,
    // we could use a kind of hash map containing fixed-sized arrays, where size of
    // array is calculated based on key spread factor.
    std::vector<SpriteInfo> SpriteInfos;

    // Get game's native color depth (bits per pixel)
    inline int GetColorDepth() const { return color_depth * 8; }


    GameSetupStruct();
    GameSetupStruct(GameSetupStruct &&gss) = default;
    ~GameSetupStruct();

    GameSetupStruct &operator =(GameSetupStruct &&gss) = default;

    void Free();

    // [IKM] Game struct loading code is moved here from Engine's load_game_file
    // function; for now it is not supposed to be called by Editor; although it
    // is possible that eventually will be.
    //
    // Since reading game data is made in a bit inconvenient way I had to
    // a) divide process into three functions (there's some extra stuff
    // being read between them;
    // b) use a helper struct to pass some arguments
    //

    //--------------------------------------------------------------------
    // Do not call these directly
    //------------------------------
    // Part 1
    void read_savegame_info(Common::Stream *in, GameDataVersion data_ver);
    void read_font_infos(Common::Stream *in, GameDataVersion data_ver);
    HGameFileError read_cursors(Common::Stream *in);
    void read_interaction_scripts(Common::Stream *in, GameDataVersion data_ver);
    void read_words_dictionary(Common::Stream *in);

    void ReadInvInfo(Common::Stream *in);
    void WriteInvInfo(Common::Stream *out);
    void ReadMouseCursors(Common::Stream *in);
    void WriteMouseCursors(Common::Stream *out);
    //------------------------------
    // Part 2
    void read_characters(Common::Stream *in);
    void read_lipsync(Common::Stream *in, GameDataVersion data_ver);
    void read_messages(Common::Stream *in, const std::array<int, MAXGLOBALMES> &load_messages, GameDataVersion data_ver);

    void ReadCharacters(Common::Stream *in);
    void WriteCharacters(Common::Stream *out);
    //------------------------------
    // Part 3
    HGameFileError read_customprops(Common::Stream *in, GameDataVersion data_ver);
    HGameFileError read_audio(Common::Stream *in, GameDataVersion data_ver);
    void read_room_names(Common::Stream *in, GameDataVersion data_ver);

    void ReadAudioClips(Common::Stream *in, size_t count);
    //--------------------------------------------------------------------

    // Functions for reading and writing appropriate data from/to save game
    void ReadFromSavegame(Common::Stream *in);
    void WriteForSavegame(Common::Stream *out);
};

//=============================================================================
#if defined (OBSOLETE)
struct OldGameSetupStruct;
void ConvertOldGameStruct (OldGameSetupStruct *ogss, GameSetupStruct *gss);
#endif // OBSOLETE

// Finds an audio clip using legacy convention index
ScriptAudioClip* GetAudioClipForOldStyleNumber(GameSetupStruct &game, bool is_music, int num);

#endif // __AGS_CN_AC__GAMESETUPSTRUCT_H
