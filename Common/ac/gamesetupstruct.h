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
#include "game/interactions.h"
#include "game/main_game_file.h" // TODO: constants to separate header or split out reading functions
#include "gui/guidefines.h"


using AGS::Common::UInteractionEvents;
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
    std::vector<UInteractionEvents> charScripts;
    std::vector<UInteractionEvents> invScripts;
    // TODO: why we do not use this in the engine instead of
    // loaded_game_file_version?
    int               filever = 0;  // just used by editor
    Common::String    compiled_with; // version of AGS this data was created by
    char              lipSyncFrameLetters[MAXLIPSYNCFRAMES][50] = {{ 0 }};
    AGS::Common::PropertySchema propSchema;
    std::vector<AGS::Common::StringIMap> charProps;
    AGS::Common::StringIMap invProps[MAX_INV];
    std::vector<AGS::Common::StringIMap> audioclipProps;
    std::vector<AGS::Common::StringIMap> dialogProps;
    std::vector<AGS::Common::StringIMap> guiProps;
    std::vector<AGS::Common::StringIMap> guicontrolProps[AGS::Common::kGUIControlTypeNum];
    // NOTE: although the view names are stored in game data, they are never
    // used, nor registered as script exports; numeric IDs are used to
    // reference views instead.
    std::vector<Common::String> viewNames;
    Common::String    invScriptNames[MAX_INV];
    std::vector<Common::String> dialogScriptNames;
    char              guid[MAX_GUID_LENGTH] = { 0 };
    char              saveGameFileExtension[MAX_SG_EXT_LENGTH] = { 0 };
    // NOTE: saveGameFolderName is generally used to create game subdirs in common user directories
    Common::String    saveGameFolderName;
    // Saved room names, known during the game compilation;
    // may be used to learn the total number of registered rooms
    std::map<int, Common::String> roomNames;
    std::vector<ScriptAudioClip> audioClips;
    std::vector<AudioClipType> audioClipTypes;
    // number of accessible game audio channels (the ones under direct user control)
    int               numGameChannels = 0;
    // backward-compatible channel limit that may be exported to script and reserved by audiotypes
    int               numCompatGameChannels = 0;

    // Extended global game properties
    // Character face direction ratio (y/x)
    float             faceDirectionRatio = 1.f;
    
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


    GameSetupStruct() = default;
    GameSetupStruct(GameSetupStruct &&gss) = default;
    ~GameSetupStruct() = default;

    GameSetupStruct &operator =(GameSetupStruct &&gss) = default;

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
    // NOTE: Global messages are cut out, but we still have to check them
    // so long as we keep support of loading an older game data
    void skip_messages(Common::Stream *in, const std::array<int, NUM_LEGACY_GLOBALMES> &load_messages, GameDataVersion data_ver);

    void ReadCharacters(Common::Stream *in);
    void WriteCharacters(Common::Stream *out);
    //------------------------------
    // Part 3
    HGameFileError read_customprops(Common::Stream *in, GameDataVersion data_ver);
    HGameFileError read_audio(Common::Stream *in, GameDataVersion data_ver);
    void read_room_names(Common::Stream *in, GameDataVersion data_ver);

    void ReadAudioClips(Common::Stream *in, size_t count);
    //--------------------------------------------------------------------

    void ReadFromSavegame(Common::Stream *in);
    void WriteForSavegame(Common::Stream *out);
};


#endif // __AGS_CN_AC__GAMESETUPSTRUCT_H
