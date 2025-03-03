//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include <vector>
#include "ac/character.h"
#include "ac/dialog.h"
#include "ac/display.h"
#include "ac/draw.h"
#include "ac/file.h"
#include "ac/game.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/gui.h"
#include "ac/lipsync.h"
#include "ac/movelist.h"
#include "ac/spritecache.h"
#include "ac/view.h"
#include "ac/dynobj/all_dynamicclasses.h"
#include "ac/dynobj/all_scriptclasses.h"
#include "ac/dynobj/dynobj_manager.h"
#include "core/assetmanager.h"
#include "debug/debug_log.h"
#include "debug/out.h"
#include "font/agsfontrenderer.h"
#include "font/fonts.h"
#include "game/game_init.h"
#include "gfx/bitmap.h"
#include "gfx/ddb.h"
#include "gui/guilabel.h"
#include "gui/guiinv.h"
#include "media/audio/audio_system.h"
#include "platform/base/agsplatformdriver.h"
#include "plugin/plugin_engine.h"
#include "script/cc_common.h"
#include "script/exports.h"
#include "script/script.h"
#include "script/script_runtime.h"
#include "util/memory_compat.h"
#include "util/string_compat.h"
#include "util/string_utils.h"

using namespace Common;
using namespace Engine;

extern ScriptSystem scsystem;
extern std::vector<ViewStruct> views;
extern SpriteCache spriteset;

extern CCCharacter ccDynamicCharacter;
extern CCHotspot   ccDynamicHotspot;
extern CCRegion    ccDynamicRegion;
extern CCWalkableArea ccDynamicWalkarea;
extern CCWalkbehind ccDynamicWalkbehind;
extern CCInventory ccDynamicInv;
extern CCGUI       ccDynamicGUI;
extern CCTextWindowGUI ccDynamicTextWindowGUI;
extern CCObject    ccDynamicObject;
extern CCDialog    ccDynamicDialog;
extern CCAudioChannel ccDynamicAudio;
extern CCAudioClip ccDynamicAudioClip;

extern ScriptObject scrObj[MAX_ROOM_OBJECTS];
extern std::vector<ScriptGUI> scrGui;
extern ScriptHotspot scrHotspot[MAX_ROOM_HOTSPOTS];
extern ScriptRegion scrRegion[MAX_ROOM_REGIONS];
extern ScriptWalkableArea scrWalkarea[MAX_WALK_AREAS];
extern ScriptWalkbehind scrWalkbehind[MAX_WALK_BEHINDS];
extern ScriptInvItem scrInv[MAX_INV];
extern ScriptAudioChannel scrAudioChannel[MAX_GAME_CHANNELS];

extern ScriptDialogOptionsRendering ccDialogOptionsRendering;
extern ScriptDrawingSurface* dialogOptionsRenderingSurface;

// Lipsync
extern std::vector<SpeechLipSyncLine> splipsync;
extern int numLipLines, curLipLine, curLipLinePhoneme;

extern AGSCCStaticObject GlobalStaticManager;
CCStaticArray StaticHandlesArray;
// Static arrays of managed handles for individual game objects
std::vector<int> StaticCharacterArray;
std::vector<int> StaticObjectArray;
std::vector<int> StaticGUIArray;
std::vector<int> StaticHotspotArray;
std::vector<int> StaticRegionArray;
std::vector<int> StaticWalkareaArray;
std::vector<int> StaticWalkbehindArray;
std::vector<int> StaticInventoryArray;
std::vector<int> StaticDialogArray;
std::vector<int> StaticAudioClipArray;


namespace AGS
{
namespace Engine
{

String GetGameInitErrorText(GameInitErrorType err)
{
    switch (err)
    {
    case kGameInitErr_NoError:
        return "No error.";
    case kGameInitErr_NoFonts:
        return "No fonts specified to be used in this game.";
    case kGameInitErr_TooManyAudioTypes:
        return "Too many audio types for this engine to handle.";
    case kGameInitErr_EntityInitFail:
        return "Failed to initialize game entities.";
    case kGameInitErr_PluginNameInvalid:
        return "Plugin name is invalid.";
    case kGameInitErr_NoGlobalScript:
        return "No global script in game.";
    case kGameInitErr_ScriptLinkFailed:
        return "Script link failed.";
    }
    return "Unknown error.";
}

// Initializes audio channels and clips and registers them in the script system
void InitAndRegisterAudioObjects(GameSetupStruct &game)
{
    for (int i = 0; i < game.numCompatGameChannels; ++i)
    {
        scrAudioChannel[i].id = i;
        ccRegisterPersistentObject(&scrAudioChannel[i], &ccDynamicAudio); // add internal ref
    }

    StaticAudioClipArray.resize(game.audioClips.size());
    for (size_t i = 0; i < game.audioClips.size(); ++i)
    {
        // Note that as of 3.5.0 data format the clip IDs are still restricted
        // to actual item index in array, so we don't make any difference
        // between game versions, for now.
        game.audioClips[i].id = i;
        int handle = ccRegisterPersistentObject(&game.audioClips[i], &ccDynamicAudioClip); // add internal ref
        StaticAudioClipArray[i] = handle;
        ccAddExternalScriptObjectHandle(game.audioClips[i].scriptName, &StaticAudioClipArray[i]);
    }
}

// Initializes characters and registers them in the script system
void InitAndRegisterCharacters(GameSetupStruct &game, const LoadedGameEntities &ents)
{
    // ensure at least 1 element, we must register buffer
    StaticCharacterArray.resize(std::max(1, game.numcharacters));

    for (int i = 0; i < game.numcharacters; ++i)
    {
        game.chars[i].walking = 0;
        game.chars[i].animating = 0;
        game.chars[i].pic_xoffs = 0;
        game.chars[i].pic_yoffs = 0;
        game.chars[i].blinkinterval = 140;
        game.chars[i].blinktimer = game.chars[i].blinkinterval;
        game.chars[i].index_id = i;
        game.chars[i].blocking_width = 0;
        game.chars[i].blocking_height = 0;
        game.chars[i].prevroom = -1;
        game.chars[i].loop = 0;
        game.chars[i].frame = 0;
        game.chars[i].walkwait = -1;
        // register and save handle
        int handle = ccRegisterPersistentObject(&game.chars[i], &ccDynamicCharacter);
        StaticCharacterArray[i] = handle;

        // export the character's script object
        ccAddExternalScriptObjectHandle(game.chars[i].scrname, &StaticCharacterArray[i]);
    }

    // extra character properties (because the characters are split into 2 structs now)
    if (ents.CharEx.size() > 0)
    {
        for (int i = 0; i < game.numcharacters; ++i)
        {
            charextra[i].blend_mode = ents.CharEx[i].BlendMode;
        }
    }
}

// Initializes dialog and registers them in the script system
void InitAndRegisterDialogs(const GameSetupStruct &game)
{
    scrDialog.resize(game.numdialog);
    // ensure at least 1 element, we must register buffer
    StaticDialogArray.resize(std::max(1, game.numdialog));

    for (int i = 0; i < game.numdialog; ++i)
    {
        scrDialog[i].id = i;
        // register and save handle
        int handle = ccRegisterPersistentObject(&scrDialog[i], &ccDynamicDialog);
        StaticDialogArray[i] = handle;

        if (!game.dialogScriptNames[i].IsEmpty())
            ccAddExternalScriptObjectHandle(game.dialogScriptNames[i], &StaticDialogArray[i]);
    }
}

// Initializes dialog options rendering objects and registers them in the script system
void InitAndRegisterDialogOptions()
{
    ccRegisterPersistentObject(&ccDialogOptionsRendering, &ccDialogOptionsRendering); // add internal ref

    dialogOptionsRenderingSurface = new ScriptDrawingSurface();
    dialogOptionsRenderingSurface->isLinkedBitmapOnly = true;
    ccRegisterPersistentObject(dialogOptionsRenderingSurface, dialogOptionsRenderingSurface); // add internal ref
}

// Initializes gui and registers them in the script system
HError InitAndRegisterGUI(const GameSetupStruct &game)
{
    scrGui.resize(game.numgui);
    // ensure at least 1 element, we must register buffer
    StaticGUIArray.resize(std::max(1, game.numgui));

    for (int i = 0; i < game.numgui; ++i)
    {
        scrGui[i].id = -1;
    }

    GUIRefCollection guictrl_refs(guibuts, guiinv, guilabels, guilist, guislider, guitext);
    for (int i = 0; i < game.numgui; ++i)
    {
        // link controls to their parent guis
        HError err = guis[i].RebuildArray(guictrl_refs);
        if (!err)
            return err;
        scrGui[i].id = i;

        // register and save handle
        IScriptObject *mgr = guis[i].IsTextWindow() ? &ccDynamicTextWindowGUI : &ccDynamicGUI;
        int handle = ccRegisterPersistentObject(&scrGui[i], mgr);
        StaticGUIArray[i] = handle;

        // export the gui script object
        ccAddExternalScriptObjectHandle(guis[i].Name, &StaticGUIArray[i]);
    }
    // export all the GUI's controls
    export_all_gui_controls();

    return HError::None();
}

// Initializes inventory items and registers them in the script system
void InitAndRegisterInvItems(const GameSetupStruct &game)
{
    StaticInventoryArray.resize(MAX_INV);

    for (int i = 0; i < MAX_INV; ++i)
    {
        scrInv[i].id = i;
        // register and save handle
        int handle = ccRegisterPersistentObject(&scrInv[i], &ccDynamicInv);
        StaticInventoryArray[i] = handle;

        if (!game.invScriptNames[i].IsEmpty())
            ccAddExternalScriptObjectHandle(game.invScriptNames[i], &StaticInventoryArray[i]);
    }
}

// Register room script entity of a given type
template <typename TScriptType>
void RegisterRoomEntities(std::vector<int> &static_arr, TScriptType scr_objs[], size_t count, AGSCCDynamicObject &mgr)
{
    static_arr.resize(count);
    for (size_t i = 0; i < count; ++i)
    {
        scr_objs[i].id = i;
        // register and save handle
        int handle = ccRegisterPersistentObject(&scr_objs[i], &mgr);
        static_arr[i] = handle;
    }
}

// Initializes room entities and registers them in the script system
void InitAndRegisterRoomEntities()
{
    RegisterRoomEntities(StaticHotspotArray, scrHotspot, MAX_ROOM_HOTSPOTS, ccDynamicHotspot);
    RegisterRoomEntities(StaticObjectArray, scrObj, MAX_ROOM_OBJECTS, ccDynamicObject);
    RegisterRoomEntities(StaticRegionArray, scrRegion, MAX_ROOM_REGIONS, ccDynamicRegion);
    RegisterRoomEntities(StaticWalkareaArray, scrWalkarea, MAX_WALK_AREAS, ccDynamicWalkarea);
    RegisterRoomEntities(StaticWalkbehindArray, scrWalkbehind, MAX_WALK_BEHINDS, ccDynamicWalkbehind);
}

// Registers static entity arrays in the script system
void RegisterStaticArrays(GameSetupStruct &game)
{
    StaticHandlesArray.Create(sizeof(int32_t));

    // Game element arrays
    ccAddExternalScriptObject("character", &StaticCharacterArray[0], &StaticHandlesArray);
    ccAddExternalScriptObject("dialog", &StaticDialogArray[0], &StaticHandlesArray);
    ccAddExternalScriptObject("gui", &StaticGUIArray[0], &StaticHandlesArray);
    ccAddExternalScriptObject("inventory", &StaticInventoryArray[0], &StaticHandlesArray);
    // Room element arrays
    ccAddExternalScriptObject("hotspot", &StaticHotspotArray[0], &StaticHandlesArray);
    ccAddExternalScriptObject("object", &StaticObjectArray[0], &StaticHandlesArray);
    ccAddExternalScriptObject("region", &StaticRegionArray[0], &StaticHandlesArray);
    ccAddExternalScriptObject("walkarea", &StaticWalkareaArray[0], &StaticHandlesArray);
    ccAddExternalScriptObject("walkbehind", &StaticWalkbehindArray[0], &StaticHandlesArray);
}

// Initializes various game entities and registers them in the script system
HError InitAndRegisterGameEntities(const LoadedGameEntities &ents)
{
    GameSetupStruct &game = ents.Game;
    InitAndRegisterAudioObjects(game);
    InitAndRegisterCharacters(game, ents);
    InitAndRegisterDialogs(game);
    InitAndRegisterDialogOptions();
    HError err = InitAndRegisterGUI(game);
    if (!err)
        return err;
    InitAndRegisterInvItems(game);

    InitAndRegisterRoomEntities();

    RegisterStaticArrays(game);

    setup_player_character(game.playercharacter);
    ccAddExternalScriptObjectHandle("player", &_sc_PlayerCharPtr);
    return HError::None();
}

void LoadFonts(GameSetupStruct &game, GameDataVersion data_ver)
{
    for (int i = 0; i < game.numfonts; ++i) 
    {
        if (!game.fonts[i].Filename.IsEmpty())
        {
            load_font_size(i, game.fonts[i]);
        }
        else
        {
            Debug::Printf(kDbgMsg_Warn, "Font %d does not have any source filename assigned, won't be loaded on startup.", i);
        }
    }

    // Additional fixups - after all the fonts are registered
    for (int i = 0; i < game.numfonts; ++i)
    {
        if (!is_bitmap_font(i))
        {
            // Check for the LucasFan font since it comes with an outline font that
            // is drawn incorrectly with Freetype versions > 2.1.3.
            // A simple workaround is to disable outline fonts for it and use
            // automatic outline drawing.
            const int outline_font = get_font_outline(i);
            if (outline_font < 0) continue;
            const char *name = get_font_name(i);
            const char *outline_name = get_font_name(outline_font);
            if ((ags_stricmp(name, "LucasFan-Font") == 0) && (ags_stricmp(outline_name, "Arcade") == 0))
                set_font_outline(i, FONT_OUTLINE_AUTO);
        }
    }
}

void LoadLipsyncData()
{
    auto speechsync = AssetMgr->OpenAsset("syncdata.dat", "voice");
    if (!speechsync)
        return;
    // this game has voice lip sync
    int lipsync_fmt = speechsync->ReadInt32();
    if (lipsync_fmt != 4)
    {
        Debug::Printf(kDbgMsg_Info, "Unknown speech lip sync format (%d).\nLip sync disabled.", lipsync_fmt);
    }
    else {
        numLipLines = speechsync->ReadInt32();
        splipsync.resize(numLipLines);
        for (int ee = 0; ee < numLipLines; ee++)
        {
            splipsync[ee].numPhonemes = speechsync->ReadInt16();
            speechsync->Read(splipsync[ee].filename, 14);
            splipsync[ee].endtimeoffs.resize(splipsync[ee].numPhonemes);
            speechsync->ReadArrayOfInt32(splipsync[ee].endtimeoffs.data(), splipsync[ee].numPhonemes);
            splipsync[ee].frame.resize(splipsync[ee].numPhonemes);
            speechsync->ReadArrayOfInt16(splipsync[ee].frame.data(), splipsync[ee].numPhonemes);
        }
    }
    Debug::Printf(kDbgMsg_Info, "Lipsync data found and loaded");
}

void InitGameResolution(GameSetupStruct &game, GameDataVersion data_ver)
{
    const Size game_size = game.GetGameRes();
    Debug::Printf(kDbgMsg_Info, "Game native resolution: %d x %d (%d bit)", game_size.Width, game_size.Height, game.color_depth * 8);

    // Assign general game viewports
    Rect viewport = RectWH(game_size);
    play.SetMainViewport(viewport);
    play.SetUIViewport(viewport);
    
    // Assign ScriptSystem's resolution variables
    scsystem.width = game.GetGameRes().Width;
    scsystem.height = game.GetGameRes().Height;
    scsystem.coldepth = game.GetColorDepth();
    scsystem.viewport_width = play.GetMainViewport().GetWidth();
    scsystem.viewport_height = play.GetMainViewport().GetHeight();
}

HGameInitError InitGameState(const LoadedGameEntities &ents, GameDataVersion data_ver)
{
    GameSetupStruct &game = ents.Game;
    const ScriptAPIVersion base_api = (ScriptAPIVersion)game.options[OPT_BASESCRIPTAPI];
    const ScriptAPIVersion compat_api = (ScriptAPIVersion)game.options[OPT_SCRIPTCOMPATLEV];
    const char *base_api_name = GetScriptAPIName(base_api);
    const char *compat_api_name = GetScriptAPIName(compat_api);
    Debug::Printf(kDbgMsg_Info, "Requested script API: %s (%d), compat level: %s (%d)",
                base_api >= 0 && base_api <= kScriptAPI_Current ? base_api_name : "unknown", base_api,
                compat_api >= 0 && compat_api <= kScriptAPI_Current ? compat_api_name : "unknown", compat_api);
    // If the game was compiled using unsupported version of the script API,
    // we warn about potential incompatibilities but proceed further.
    if (game.options[OPT_BASESCRIPTAPI] > kScriptAPI_Current)
        platform->DisplayAlert("Warning: this game requests a higher version of AGS script API, it may not run correctly or run at all.");

    //
    // 1. Check that the loaded data is valid and compatible with the current
    // engine capabilities.
    //
    if (game.numfonts == 0)
        return new GameInitError(kGameInitErr_NoFonts);
    if (game.audioClipTypes.size() > MAX_AUDIO_TYPES)
        return new GameInitError(kGameInitErr_TooManyAudioTypes, String::FromFormat("Required: %zu, max: %zu", game.audioClipTypes.size(), (size_t)MAX_AUDIO_TYPES));

    //
    // 3. Allocate and init game objects
    //
    spriteset.EnlargeTo(ents.SpriteCount - 1);
    charextra.resize(game.numcharacters);
    // NOTE: movelists have 1 extra slot 0 which assumes a role of "undefined" list
    mls.resize(game.numcharacters + MAX_ROOM_OBJECTS + 1);
    guis = std::move(ents.Guis);
    guibuts = std::move(ents.GuiControls.Buttons);
    guiinv = std::move(ents.GuiControls.InvWindows);
    guilabels = std::move(ents.GuiControls.Labels);
    guilist = std::move(ents.GuiControls.ListBoxes);
    guislider = std::move(ents.GuiControls.Sliders);
    guitext = std::move(ents.GuiControls.TextBoxes);
    GUI::Context.GameColorDepth = game.GetColorDepth();
    GUI::Context.Spriteset = &spriteset;
    GUIRefCollection guictrl_refs(guibuts, guiinv, guilabels, guilist, guislider, guitext);
    GUI::RebuildGUI(guis, guictrl_refs);
    views = std::move(ents.Views);
    play.audioclipProps.resize(game.audioClips.size());
    play.charProps.resize(game.numcharacters);
    play.dialogProps.resize(game.numdialog);
    play.guiProps.resize(game.numgui);
    play.guicontrolProps[kGUIButton].resize(guibuts.size());
    play.guicontrolProps[kGUILabel].resize(guilabels.size());
    play.guicontrolProps[kGUIInvWindow].resize(guiinv.size());
    play.guicontrolProps[kGUISlider].resize(guislider.size());
    play.guicontrolProps[kGUITextBox].resize(guitext.size());
    play.guicontrolProps[kGUIListBox].resize(guilist.size());
    dialog = std::move(ents.Dialogs);
    // Set number of game channels corresponding to the loaded game version
    game.numGameChannels = MAX_GAME_CHANNELS;
    game.numCompatGameChannels = MAX_GAME_CHANNELS;
    HError err = InitAndRegisterGameEntities(ents);
    if (!err)
        return new GameInitError(kGameInitErr_EntityInitFail, err);
    LoadFonts(game, data_ver);
    LoadLipsyncData();

    //
    // 4. Initialize certain runtime variables
    //
    // TODO: merge this with engine_init_game_settings()
    game_paused = 0;  // reset the game paused flag
    ifacepopped = -1;

    String svg_suffix;
    if (game.saveGameFileExtension[0] != 0)
        svg_suffix.Format(".%s", game.saveGameFileExtension);
    set_save_game_suffix(svg_suffix);

    play.fade_effect = game.options[OPT_FADETYPE];
    play.std_gui_textheight = get_font_height_outlined(0) + 1;
    play.enable_antialiasing = usetup.AntialiasSprites;

    //
    // 5. Initialize runtime state of certain game objects
    //
    InitGameResolution(game, data_ver);
    prepare_gui_runtime(true /* startup */);
    calculate_reserved_channel_count();
    // Default viewport and camera, draw data, etc, should be created when resolution is set
    play.CreatePrimaryViewportAndCamera();
    init_game_drawdata();

    //
    // 6. Register engine API exports
    // NOTE: we must do this before plugin start, because some plugins may
    // require access to script API at initialization time.
    //
    InitScriptExec();
    setup_script_exports(base_api, compat_api);

    //
    // 7. Start up plugins
    //
    pl_register_plugins(ents.PluginInfos, !usetup.Override.NoPlugins);
    pl_startup_plugins();

    //
    // 8. Create script modules
    // NOTE: we must do this after plugins, because some plugins may export
    // script symbols too.
    //
    if (!ents.GlobalScript)
        return new GameInitError(kGameInitErr_NoGlobalScript);
    gamescript = std::move(RuntimeScript::Create(ents.GlobalScript.get(), "G"));
    dialogScriptsScript= std::move(RuntimeScript::Create(ents.DialogScript.get(), "D"));
    numScriptModules = ents.ScriptModules.size();
    for (size_t i = 0; i < ents.ScriptModules.size(); ++i)
        scriptModules.push_back(std::shared_ptr<RuntimeScript>(RuntimeScript::Create(ents.ScriptModules[i].get(), "M")));
    AllocScriptModules();
    if (!LinkGlobalScripts())
        return new GameInitError(kGameInitErr_ScriptLinkFailed, cc_get_error().ErrorString);

    // Apply accessibility options, must be done last, because some
    // may override startup game settings.
    ApplyAccessibilityOptions();

    return HGameInitError::None();
}

void ApplyAccessibilityOptions()
{
    if (usetup.Access.SpeechSkipStyle != kSkipSpeechNone)
    {
        play.speech_skip_style = user_to_internal_skip_speech(usetup.Access.SpeechSkipStyle);
    }
    if (usetup.Access.TextSkipStyle != kSkipSpeechNone)
    {
        play.skip_display = usetup.Access.TextSkipStyle;
    }
    if (usetup.Access.TextReadSpeed > 0)
    {
        play.text_speed = usetup.Access.TextReadSpeed;
        play.text_min_display_time_ms = Math::Clamp((int)(1000 * (15.f / usetup.Access.TextReadSpeed)), 1000, 3000);
    }
}

} // namespace Engine
} // namespace AGS
