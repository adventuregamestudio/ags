// AGS - Project management implementation
#include "project.h"
#include "game_data.h"
#include "script_api_versions.h"

#include "data/agfreader.h"

#include "util/path_helpers.h"

#include "ac/gamestructdefines.h"
#include "ac/spritefile.h"
#include "gui/guidefines.h"

#include "tinyxml2.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>

namespace fs = std::filesystem;

namespace AGSBuild
{

using AGS::Common::NormalizePath;
using AGS::Common::ResolveToAbsolutePath;

namespace {

constexpr int kCharacterInteractionEvents = 9;
constexpr int kInventoryInteractionEvents = 5;

} // namespace

Project::Project() = default;

Project::~Project() = default;

void Project::UpdateProjectDir()
{
    if (!project_path_.empty())
    {
        fs::path p(project_path_);
        project_dir_ = ResolveToAbsolutePath(p.parent_path().string());
    }
    else
    {
        project_dir_.clear();
    }
}

void Project::SetProjectDir(const std::string& dir)
{
    project_dir_ = ResolveToAbsolutePath(dir);
}

// -------------------------------------------------------------------------
// AGF XML helper functions (via shared agfreader ValueParser)
// -------------------------------------------------------------------------

using AgfDocElem = const tinyxml2::XMLElement*;

static const char* AgfText(AgfDocElem parent, const char* name, const char* def = "")
{
    const char* text = AGS::AGF::ValueParser::ReadString(
        const_cast<AGS::AGF::DocElem>(parent), name, def ? def : "");
    return text ? text : "";
}

static int AgfInt(AgfDocElem parent, const char* name, int def = 0)
{
    return AGS::AGF::ValueParser::ReadInt(
        const_cast<AGS::AGF::DocElem>(parent), name, def);
}

static bool AgfBool(AgfDocElem parent, const char* name, bool def = false)
{
    return AGS::AGF::ValueParser::ReadBool(
        const_cast<AGS::AGF::DocElem>(parent), name, def);
}

template<typename ListParser, typename Callback>
static void ForEachAgfList(const tinyxml2::XMLElement* root, Callback cb)
{
    ListParser parser;
    std::vector<AGS::AGF::DocElem> elems;
    parser.GetAll(const_cast<AGS::AGF::DocElem>(root), elems);
    for (AGS::AGF::DocElem elem : elems)
        cb(elem);
}

static void ParseResolution(const char* text, int& w, int& h)
{
    if (!text) return;
    if (sscanf(text, "%d,%d", &w, &h) != 2)
    {
        if (sscanf(text, "%dx%d", &w, &h) != 2)
            w = h = atoi(text);
    }
}

// Load Interactions from an <Interactions> child element
static Interactions LoadInteractions(const tinyxml2::XMLElement* parent_elem, int num_events)
{
    Interactions result;
    result.handler_functions.resize(num_events);
    const tinyxml2::XMLElement* inter = parent_elem->FirstChildElement("Interactions");
    if (!inter) return result;

    result.script_module = AgfText(inter, "ScriptModule", "");

    for (const tinyxml2::XMLElement* ev = inter->FirstChildElement("Event");
         ev != nullptr; ev = ev->NextSiblingElement("Event"))
    {
        int index = 0;
        ev->QueryIntAttribute("Index", &index);
        if (index >= 0 && index < num_events)
        {
            const char* text = ev->GetText();
            if (text && text[0] != '\0')
                result.handler_functions[index] = text;
        }
    }
    return result;
}

// -------------------------------------------------------------------------
// LoadFromAGF — Parse .agf XML project file
// -------------------------------------------------------------------------
bool Project::LoadFromAGF(const std::string& agf_path)
{
    AGS::AGF::AGFReader reader;
    AGS::Common::HError open_err = reader.Open(agf_path.c_str());
    if (!open_err)
    {
        fprintf(stderr, "[Project] Failed to open '%s': %s\n",
                agf_path.c_str(), open_err->FullMessage().GetCStr());
        return false;
    }

    AGS::AGF::DocElem game_node = reader.GetGameRoot();
    if (!game_node)
    {
        fprintf(stderr, "[Project] '%s' has no <Game> element.\n", agf_path.c_str());
        return false;
    }

    game_data_ = std::make_unique<GameData>();

    // Read saved editor version from root element attribute
    if (const tinyxml2::XMLElement* root = game_node->GetDocument()->RootElement())
    {
        const char* editor_ver = root->Attribute("EditorVersion");
        if (editor_ver)
            game_data_->saved_editor_version = editor_ver;
    }

    // --- Settings ---
    const tinyxml2::XMLElement* settings = game_node->FirstChildElement("Settings");
    if (settings)
    {
        game_data_->game_title = AgfText(settings, "GameName", "Untitled");

        // Game identity
        game_data_->unique_id = AgfInt(settings, "UniqueID", 0);
        game_data_->guid_string = AgfText(settings, "GUIDAsString", "");

        const char* res_text = AgfText(settings, "CustomResolution", nullptr);
        if (res_text && strlen(res_text) > 0)
            ParseResolution(res_text, game_data_->resolution_width, game_data_->resolution_height);

        const char* depth_text = AgfText(settings, "ColorDepth", "TrueColor");
        if (depth_text)
        {
            if (strcmp(depth_text, "TrueColor") == 0 || strcmp(depth_text, "32") == 0)
                game_data_->color_depth = 32;
            else if (strcmp(depth_text, "HighColor") == 0 || strcmp(depth_text, "16") == 0)
                game_data_->color_depth = 16;
            else if (strcmp(depth_text, "Palette") == 0 || strcmp(depth_text, "8") == 0)
                game_data_->color_depth = 8;
        }

        game_data_->debug_mode = AgfBool(settings, "DebugMode", false);

        // Extended settings
        game_data_->anti_alias_fonts = AgfBool(settings, "AntiAliasFonts", false);
        game_data_->target_fps = AgfInt(settings, "GameFPS", 40);
        game_data_->description = AgfText(settings, "Description", "");
        game_data_->developer_name = AgfText(settings, "DeveloperName", "");
        game_data_->developer_url = AgfText(settings, "DeveloperURL", "");
        game_data_->max_score = AgfInt(settings, "MaximumScore", 0);
        game_data_->save_screenshots = AgfBool(settings, "SaveScreenshots", false);
        game_data_->pixel_perfect_speed = AgfBool(settings, "AntiGlideMode", true);
        game_data_->pixel_perfect = AgfBool(settings, "PixelPerfect", true);
        game_data_->walk_in_look_mode = AgfBool(settings, "WalkInLookMode", false);
        game_data_->mouse_wheel_enabled = AgfBool(settings, "MouseWheelEnabled", true);
        game_data_->auto_move_in_walk_mode = AgfBool(settings, "AutoMoveInWalkMode", true);
        game_data_->letterbox_mode = AgfBool(settings, "LetterboxMode", false);
        game_data_->backwards_text = AgfBool(settings, "BackwardsText", false);
        game_data_->clip_gui_controls = AgfBool(settings, "ClipGUIControls", true);
        game_data_->scale_char_sprite_offsets = AgfBool(settings, "ScaleCharacterSpriteOffsets", false);
        game_data_->use_old_voice_clip_naming = AgfBool(settings, "UseOldVoiceClipNaming", false);
        game_data_->gui_handle_only_left_mouse = AgfBool(settings, "GUIHandleOnlyLeftMouseButton", false);

        // Split resources
        int split_val = AgfInt(settings, "SplitResources", 0);
        game_data_->split_resources = (split_val > 0);
        game_data_->split_resource_threshold = (split_val > 0) ? split_val : 1024;

        // Dialog settings
        game_data_->dialog_options_gui = AgfInt(settings, "DialogOptionsGUI", 0);
        game_data_->dialog_options_gap = AgfInt(settings, "DialogOptionsGap", 0);
        game_data_->dialog_bullet_sprite = AgfInt(settings, "DialogOptionsBullet", 0);
        const char* num_dlg_opt = AgfText(settings, "NumberDialogOptions", "None");
        if (num_dlg_opt) {
            if (strcmp(num_dlg_opt, "None") == 0) game_data_->number_dialog_options = -1;
            else if (strcmp(num_dlg_opt, "KeyShortcutsOnly") == 0) game_data_->number_dialog_options = 0;
            else if (strcmp(num_dlg_opt, "NoneStated") == 0) game_data_->number_dialog_options = 0;
            else game_data_->number_dialog_options = 1;
        }
        game_data_->dialog_options_backwards = AgfBool(settings, "DialogOptionsBackwards", false);
        game_data_->run_game_loops_in_dialog = AgfBool(settings, "RunGameLoopsWhileDialogOptionsDisplayed", false);
        game_data_->dialog_say_function = AgfText(settings, "DialogScriptSayFunction", "");
        game_data_->dialog_narrate_function = AgfText(settings, "DialogScriptNarrateFunction", "");

        // Sound
        game_data_->play_sound_on_score = (AgfInt(settings, "PlaySoundOnScore", 0) > 0);
        game_data_->score_sound_clip = AgfInt(settings, "PlaySoundOnScore", -1);

        // Text & Speech
        game_data_->always_display_text_as_speech = AgfBool(settings, "AlwaysDisplayTextAsSpeech", false);
        game_data_->text_window_gui = AgfInt(settings, "TextWindowGUI", 0);
        game_data_->thought_gui = AgfInt(settings, "ThoughtGUI", 0);
        game_data_->use_global_speech_anim_delay = AgfBool(settings, "UseGlobalSpeechAnimationDelay", false);
        game_data_->global_speech_anim_delay = AgfInt(settings, "GlobalSpeechAnimationDelay", 5);

        // Speech style
        const char* speech_str = AgfText(settings, "SpeechStyle", "Lucasarts");
        if (speech_str) {
            if (strcmp(speech_str, "Lucasarts") == 0) game_data_->speech_style = kSpeechStyle_LucasArts;
            else if (strcmp(speech_str, "Sierra") == 0) game_data_->speech_style = kSpeechStyle_SierraTransparent;
            else if (strcmp(speech_str, "SierraWithBackground") == 0) game_data_->speech_style = kSpeechStyle_SierraBackground;
            // C# editor WholeScreen=3; kSpeechStyle_QFG4=4 in the engine C++ enum
            else if (strcmp(speech_str, "FullScreen") == 0) game_data_->speech_style = 3;
        }

        // Speech portrait side
        const char* portrait_str = AgfText(settings, "SpeechPortraitSide", "Left");
        if (portrait_str) {
            if (strcmp(portrait_str, "Left") == 0) game_data_->speech_portrait_side = PORTRAIT_LEFT;
            else if (strcmp(portrait_str, "Right") == 0) game_data_->speech_portrait_side = PORTRAIT_RIGHT;
            else if (strcmp(portrait_str, "Alternate") == 0) game_data_->speech_portrait_side = PORTRAIT_ALTERNATE;
            else if (strcmp(portrait_str, "XPosition") == 0) game_data_->speech_portrait_side = PORTRAIT_XPOSITION;
        }

        // GUI/Sprite alpha rendering
        const char* gui_alpha_str = AgfText(settings, "GUIAlphaStyle", "Proper");
        if (gui_alpha_str) {
            if (strcmp(gui_alpha_str, "Classic") == 0) game_data_->gui_alpha_style = kGuiAlphaRender_Legacy;
            else if (strcmp(gui_alpha_str, "AdditiveOpacity") == 0) game_data_->gui_alpha_style = kGuiAlphaRender_AdditiveAlpha;
            else if (strcmp(gui_alpha_str, "Proper") == 0 ||
                     strcmp(gui_alpha_str, "MultiplyTranslucenceSrcBlend") == 0)
                game_data_->gui_alpha_style = kGuiAlphaRender_Proper;
        }
        const char* spr_alpha_str = AgfText(settings, "SpriteAlphaStyle", "Proper");
        if (spr_alpha_str) {
            if (strcmp(spr_alpha_str, "Classic") == 0) game_data_->sprite_alpha_style = kSpriteAlphaRender_Legacy;
            else if (strcmp(spr_alpha_str, "Proper") == 0 ||
                     strcmp(spr_alpha_str, "Improved") == 0)
                game_data_->sprite_alpha_style = kSpriteAlphaRender_Proper;
        }

        // Skip speech style
        // Skip speech style — values match C# SkipSpeechStyle enum / Engine SkipSpeechStyle
        const char* skip_str = AgfText(settings, "SkipSpeech", "MouseOrKeyboardOrTimer");
        if (skip_str) {
            if (strcmp(skip_str, "MouseOrKeyboardOrTimer") == 0) game_data_->skip_speech_style = 0;
            else if (strcmp(skip_str, "KeyboardOnly") == 0) game_data_->skip_speech_style = 1;
            else if (strcmp(skip_str, "TimerOnly") == 0) game_data_->skip_speech_style = 2;
            else if (strcmp(skip_str, "MouseOrKeyboard") == 0) game_data_->skip_speech_style = 3;
            else if (strcmp(skip_str, "MouseOnly") == 0) game_data_->skip_speech_style = 4;
            else if (strcmp(skip_str, "KeyboardOnlyStrict") == 0) game_data_->skip_speech_style = 5;
            else if (strcmp(skip_str, "MouseOnlyStrict") == 0) game_data_->skip_speech_style = 6;
        }

        // Backwards compatibility
        game_data_->enforce_object_scripting = AgfBool(settings, "EnforceObjectBasedScript", true);
        game_data_->enforce_new_strings = AgfBool(settings, "EnforceNewStrings", true);
        game_data_->left_to_right_precedence = AgfBool(settings, "LeftToRightPrecedence", true);
        game_data_->enforce_new_audio = AgfBool(settings, "EnforceNewAudio", true);
        game_data_->use_old_custom_dialog_api = AgfBool(settings, "UseOldCustomDialogOptionsAPI", false);
        game_data_->use_old_keyboard_handling = AgfBool(settings, "UseOldKeyboardHandling", false);

        // Script API versions — store actual kScriptAPI_* enum values
        // (match Common/ac/gamestructdefines.h ScriptAPIVersion enum)
        const char* api_str = AgfText(settings, "ScriptAPIVersion", "Highest");
        if (api_str && strcmp(api_str, "Highest") == 0) {
            game_data_->script_api_version = INT_MAX;
        } else {
            game_data_->script_api_version = INT_MAX; // fallback
            for (int i = 0; i < kScriptAPIVersionCount; i++) {
                if (api_str && strcmp(api_str, kScriptAPIVersions[i].name) == 0) {
                    game_data_->script_api_version = kScriptAPIVersions[i].value;
                    break;
                }
            }
        }
        // Default compat level to v321 for old games without the tag
        // (max backward compat — matches C# SetScriptAPIForOldProject)
        const char* compat_str = AgfText(settings, "ScriptCompatLevel", "v321");
        if (compat_str && strcmp(compat_str, "Highest") == 0) {
            game_data_->script_compat_level = INT_MAX;
        } else {
            game_data_->script_compat_level = 0; // v321
            for (int i = 0; i < kScriptAPIVersionCount; i++) {
                if (compat_str && strcmp(compat_str, kScriptAPIVersions[i].name) == 0) {
                    game_data_->script_compat_level = kScriptAPIVersions[i].value;
                    break;
                }
            }
        }

        // Character behavior
        game_data_->turn_before_walking = AgfBool(settings, "TurnBeforeWalking", true);
        game_data_->turn_before_facing = AgfBool(settings, "TurnBeforeFacing", true);

        // Inventory
        game_data_->inventory_cursors = AgfBool(settings, "InventoryCursors", true);
        game_data_->handle_inv_clicks_in_script = AgfBool(settings, "HandleInvClicksInScript", true);
        game_data_->display_multiple_inv = AgfBool(settings, "DisplayMultipleInventory", false);

        // Saved games
        game_data_->save_game_extension = AgfText(settings, "SaveGameFileExtension", "");
        game_data_->save_game_folder = AgfText(settings, "SaveGameFolderName", "");

        // Compiler
        game_data_->game_file_name = AgfText(settings, "GameFileName", "");
        {
            const char *cdd = AgfText(settings, "CustomDataDir", "");
            game_data_->custom_data_dir = (cdd && cdd[0]) ? cdd : "";
        }

        // Sprite compression
        const char* compress_str = AgfText(settings, "CompressSpritesType", "None");
        game_data_->sprite_file_compression = AGS::Common::kSprCompress_None;
        if (compress_str) {
            if (strcmp(compress_str, "RLE") == 0) game_data_->sprite_file_compression = AGS::Common::kSprCompress_RLE;
            else if (strcmp(compress_str, "LZW") == 0) game_data_->sprite_file_compression = AGS::Common::kSprCompress_LZW;
            else if (strcmp(compress_str, "Deflate") == 0) game_data_->sprite_file_compression = AGS::Common::kSprCompress_Deflate;
        }

        game_data_->optimize_sprite_storage = AgfBool(settings, "OptimizeSpriteStorage", true);

        // Visual
        const char* transition_str = AgfText(settings, "RoomTransition", "FadeOutAndIn");
        game_data_->room_transition = kScrTran_Fade;
        if (transition_str) {
            if (strcmp(transition_str, "FadeOutAndIn") == 0) game_data_->room_transition = kScrTran_Fade;
            else if (strcmp(transition_str, "Instant") == 0) game_data_->room_transition = kScrTran_Instant;
            else if (strcmp(transition_str, "Dissolve") == 0) game_data_->room_transition = kScrTran_Dissolve;
            else if (strcmp(transition_str, "BoxOut") == 0) game_data_->room_transition = kScrTran_Boxout;
            else if (strcmp(transition_str, "Crossfade") == 0) game_data_->room_transition = kScrTran_Crossfade;
        }

        const char* render_str = AgfText(settings, "RenderAtScreenResolution", "UserDefined");
        game_data_->render_at_screen_resolution = kRenderAtScreenRes_UserDefined;
        if (render_str) {
            if (strcmp(render_str, "UserDefined") == 0) game_data_->render_at_screen_resolution = kRenderAtScreenRes_UserDefined;
            else if (strcmp(render_str, "True") == 0) game_data_->render_at_screen_resolution = kRenderAtScreenRes_Enabled;
            else if (strcmp(render_str, "False") == 0) game_data_->render_at_screen_resolution = kRenderAtScreenRes_Disabled;
        }

        const char* iface_str = AgfText(settings, "WhenInterfaceDisabled", "GreyOut");
        game_data_->when_interface_disabled = AGS::Common::kGuiDis_Greyout;
        if (iface_str) {
            if (strcmp(iface_str, "GreyOut") == 0) game_data_->when_interface_disabled = AGS::Common::kGuiDis_Greyout;
            else if (strcmp(iface_str, "GoBlack") == 0) game_data_->when_interface_disabled = AGS::Common::kGuiDis_Blackout;
            else if (strcmp(iface_str, "DisplayNormally") == 0) game_data_->when_interface_disabled = AGS::Common::kGuiDis_Unchanged;
            else if (strcmp(iface_str, "SetGUIDisabled") == 0) game_data_->when_interface_disabled = AGS::Common::kGuiDis_Off;
        }
    }

    // --- Rooms ---
    {
        std::vector<std::pair<int, AGS::Common::String>> room_list;
        AGS::AGF::ReadRoomList(room_list, game_node);
        for (const auto& r : room_list)
        {
            GameData::RoomInfo room;
            room.number = r.first;
            room.description = r.second.GetCStr();
            if (room.number >= 0)
                game_data_->rooms.push_back(room);
        }

        std::sort(game_data_->rooms.begin(), game_data_->rooms.end(),
            [](const GameData::RoomInfo& a, const GameData::RoomInfo& b) {
                return a.number < b.number;
            });
    }

    // --- Characters ---
    ForEachAgfList<AGS::AGF::Characters>(game_node, [this](AgfDocElem ch_elem) {
        GameData::CharacterInfo ch;
        ch.id = AgfInt(ch_elem, "ID", 0);
        ch.script_name = AgfText(ch_elem, "ScriptName", "");
        ch.real_name = AgfText(ch_elem, "RealName", "");
        ch.room = AgfInt(ch_elem, "StartingRoom", 0);
        ch.x = AgfInt(ch_elem, "StartX", 0);
        ch.y = AgfInt(ch_elem, "StartY", 0);
        ch.normal_view = AgfInt(ch_elem, "NormalView", -1);
        ch.speech_view = AgfInt(ch_elem, "SpeechView", -1);
        ch.idle_view = AgfInt(ch_elem, "IdleView", -1);
        ch.thinking_view = AgfInt(ch_elem, "ThinkingView", -1);
        ch.blinking_view = AgfInt(ch_elem, "BlinkingView", -1);
        ch.movement_speed = AgfInt(ch_elem, "MovementSpeed", 3);
        ch.movement_speed_x = AgfInt(ch_elem, "MovementSpeedX", 3);
        ch.movement_speed_y = AgfInt(ch_elem, "MovementSpeedY", 3);
        ch.uniform_movement_speed = AgfBool(ch_elem, "UniformMovementSpeed", true);
        ch.animation_delay = AgfInt(ch_elem, "AnimationDelay", 0);
        ch.solid = AgfBool(ch_elem, "Solid", true);
        ch.turn_before_walking = AgfBool(ch_elem, "TurnBeforeWalking", true);
        ch.turn_when_facing = AgfBool(ch_elem, "TurnWhenFacing", true);
        ch.diagonal_loops = AgfBool(ch_elem, "DiagonalLoops", true);
        ch.adjust_speed_with_scaling = AgfBool(ch_elem, "AdjustSpeedWithScaling", true);
        ch.movement_linked_to_animation = AgfBool(ch_elem, "MovementLinkedToAnimation", true);
        ch.baseline = AgfInt(ch_elem, "Baseline", 0);
        ch.use_room_area_scaling = AgfBool(ch_elem, "UseRoomAreaScaling", true);
        ch.clickable = AgfBool(ch_elem, "Clickable", true);
        ch.transparency = AgfInt(ch_elem, "Transparency", 0);
        ch.speech_animation_delay = AgfInt(ch_elem, "SpeechAnimationDelay", 0);
        ch.speech_color = AgfInt(ch_elem, "SpeechColor", 0);
        ch.use_room_area_lighting = AgfBool(ch_elem, "UseRoomAreaLighting", true);
        ch.scale_volume = AgfBool(ch_elem, "AdjustVolumeWithScaling", false);
        ch.idle_delay = AgfInt(ch_elem, "IdleDelay", 20);
        ch.idle_anim_speed = AgfInt(ch_elem, "IdleAnimationDelay", 5);
        ch.interactions = LoadInteractions(ch_elem, kCharacterInteractionEvents);
        const tinyxml2::XMLElement* props_elem = ch_elem->FirstChildElement("Properties");
        if (props_elem)
        {
            for (const tinyxml2::XMLElement* cp = props_elem->FirstChildElement("CustomProperty");
                 cp; cp = cp->NextSiblingElement("CustomProperty"))
            {
                std::string pname = AgfText(cp, "Name", "");
                std::string pval = AgfText(cp, "Value", "");
                if (!pname.empty())
                    ch.custom_properties[pname] = pval;
            }
        }
        game_data_->characters.push_back(ch);
    });

    // --- Player Character ---
    // <PlayerCharacter> is a direct child of <Game>, containing the character ID
    game_data_->player_character_id = AgfInt(game_node, "PlayerCharacter", 0);
    // Also mark which character is the player
    for (auto& ch : game_data_->characters) {
        ch.is_player = (ch.id == game_data_->player_character_id);
    }

    // --- GUIs ---
    // Helper: convert AGF TextAlignment string to FrameAlignment enum value
    auto ParseTextAlignment = [](const char* str) -> int {
        if (!str || !*str) return 0;
        if (strcmp(str, "TopLeft") == 0) return 1;
        if (strcmp(str, "TopCenter") == 0 || strcmp(str, "TopMiddle") == 0) return 2;
        if (strcmp(str, "TopRight") == 0) return 4;
        if (strcmp(str, "MiddleLeft") == 0) return 8;
        if (strcmp(str, "MiddleCenter") == 0 || strcmp(str, "Centre") == 0 ||
            strcmp(str, "Centred") == 0) return 16;
        if (strcmp(str, "MiddleRight") == 0) return 32;
        if (strcmp(str, "BottomLeft") == 0) return 64;
        if (strcmp(str, "BottomCenter") == 0 || strcmp(str, "BottomMiddle") == 0) return 128;
        if (strcmp(str, "BottomRight") == 0) return 256;
        // Legacy horizontal-only values (used by ListBox)
        if (strcmp(str, "Left") == 0) return 1;   // = TopLeft
        if (strcmp(str, "Right") == 0) return 4;   // = TopRight
        if (strcmp(str, "Center") == 0) return 2;  // = TopCenter
        return 0;
    };
    // Helper: convert AGF ClickAction string to enum value
    auto ParseClickAction = [](const char* str) -> int {
        if (!str || !*str) return 0;
        if (strcmp(str, "None") == 0) return 0;
        if (strcmp(str, "SetMode") == 0) return 1;
        if (strcmp(str, "RunScript") == 0) return 2;
        return 0;
    };

    auto collect_single_gui = [this, &ParseTextAlignment, &ParseClickAction](AgfDocElem gui) {
                const char* tag = gui->Name();
                if (strcmp(tag, "GUIMain") != 0 && strcmp(tag, "GUITextWindow") != 0)
                    return;

                // Detect inner type: <NormalGUI> vs <TextWindowGUI>
                const tinyxml2::XMLElement* normal_props = gui->FirstChildElement("NormalGUI");
                const tinyxml2::XMLElement* tw_props = gui->FirstChildElement("TextWindowGUI");
                bool is_text_window = (tw_props != nullptr && normal_props == nullptr);

                GameData::GUIInfo g;
                g.tag_name = is_text_window ? "GUITextWindow" : "GUIMain";

                if (is_text_window) {
                    // TextWindowGUI: read from <TextWindowGUI> child
                    g.id = AgfInt(tw_props, "ID", 0);
                    g.name = AgfText(tw_props, "Name", "");
                    g.border_color = AgfInt(tw_props, "TextColor", 1);
                    g.bg_color = AgfInt(tw_props, "BackgroundColor", 0);
                    g.bg_image = AgfInt(tw_props, "BackgroundImage", 0);
                    g.padding = AgfInt(tw_props, "Padding", 3);
                    // Hardcoded values for text window GUIs
                    g.x = 0;
                    g.y = 0;
                    g.width = 200;
                    g.height = 100;
                    g.visible = false;
                    g.clickable = false;
                    g.popup_style = 2; // kGUI_PopupModal
                    g.z_order = -1;
                    g.transparency = 0;
                } else if (normal_props) {
                    // NormalGUI: read from <NormalGUI> child
                    const tinyxml2::XMLElement* props = normal_props;
                    g.id = AgfInt(props, "ID", 0);
                    g.name = AgfText(props, "Name", "");
                    g.x = AgfInt(props, "Left", 0);
                    g.y = AgfInt(props, "Top", 0);
                    g.width = AgfInt(props, "Width", 100);
                    g.height = AgfInt(props, "Height", 100);
                    g.bg_color = AgfInt(props, "BackgroundColor", 0);
                    g.border_color = AgfInt(props, "BorderColor", 15);
                    g.bg_image = AgfInt(props, "BackgroundImage", 0);
                    g.transparency = AgfInt(props, "Transparency", 0);
                    g.z_order = AgfInt(props, "ZOrder", 0);
                    g.clickable = AgfBool(props, "Clickable", true);
                    g.on_click = AgfText(props, "OnClick", "");
                    g.script_module = AgfText(props, "ScriptModule", "GlobalScript.asc");
                    g.popup_at_mouse_y = AgfInt(props, "PopupYPos", -1);

                    // Try new format <PopupStyle> + <Visible> first
                    const char* popup_str = AgfText(props, "PopupStyle", nullptr);
                    if (popup_str) {
                        if (strcmp(popup_str, "Normal") == 0) g.popup_style = 0;
                        else if (strcmp(popup_str, "MouseYPos") == 0) g.popup_style = 1;
                        else if (strcmp(popup_str, "PopupModal") == 0) g.popup_style = 2;
                        else if (strcmp(popup_str, "Persistent") == 0) g.popup_style = 3;
                        else g.popup_style = 0;
                        g.visible = AgfBool(props, "Visible", true);
                    } else {
                        // Fall back to old format <Visibility>
                        const char* vis_str = AgfText(props, "Visibility", "Normal");
                        if (strcmp(vis_str, "Normal") == 0) { g.popup_style = 0; g.visible = true; }
                        else if (strcmp(vis_str, "MouseYPos") == 0 || strcmp(vis_str, "PopupYPos") == 0) { g.popup_style = 1; g.visible = false; }
                        else if (strcmp(vis_str, "ScriptOnly") == 0) { g.popup_style = 2; g.visible = false; }
                        else if (strcmp(vis_str, "PopupModal") == 0) { g.popup_style = 2; g.visible = false; }
                        else if (strcmp(vis_str, "Persistent") == 0) { g.popup_style = 3; g.visible = true; }
                        else { g.popup_style = 0; g.visible = true; }
                    }
                } else {
                    // Fallback: read from outer element
                    g.id = AgfInt(gui, "ID", 0);
                    g.name = AgfText(gui, "Name", "");
                    g.width = 200;
                    g.height = 100;
                    g.visible = false;
                    g.clickable = false;
                    g.popup_style = 2;
                    g.z_order = -1;
                    g.transparency = 0;
                }
                // Parse controls
                const tinyxml2::XMLElement* controls_node = gui->FirstChildElement("Controls");
                if (controls_node) {
                    for (const tinyxml2::XMLElement* ctrl = controls_node->FirstChildElement();
                         ctrl; ctrl = ctrl->NextSiblingElement()) {
                        GameData::GUIInfo::ControlInfo ci;
                        ci.type_tag = ctrl->Name();
                        ci.id = AgfInt(ctrl, "ID", 0);
                        ci.name = AgfText(ctrl, "Name", "");
                        ci.x = AgfInt(ctrl, "Left", 0);
                        ci.y = AgfInt(ctrl, "Top", 0);
                        ci.width = AgfInt(ctrl, "Width", 60);
                        ci.height = AgfInt(ctrl, "Height", 20);
                        ci.text = AgfText(ctrl, "Text", "");
                        ci.image = AgfInt(ctrl, "Image", -1);
                        ci.font = AgfInt(ctrl, "Font", 0);
                        // Common control flags (defaults match C# GUIControl defaults)
                        ci.enabled = AgfBool(ctrl, "Enabled", true);
                        ci.ctrl_visible = AgfBool(ctrl, "Visible", true);
                        ci.ctrl_clickable = AgfBool(ctrl, "Clickable", true);
                        ci.translated = AgfBool(ctrl, "Translated", true);
                        ci.show_border = AgfBool(ctrl, "ShowBorder", false);
                        // SolidBackground defaults depend on type (set after type detection below)
                        // Read event handler and type-specific properties
                        if (ci.type_tag == "GUIButton" || ci.type_tag == "GUITextWindowEdge") {
                            ci.event_handler = AgfText(ctrl, "OnClick", "");
                            ci.mouseover_image = AgfInt(ctrl, "MouseoverImage", 0);
                            ci.pushed_image = AgfInt(ctrl, "PushedImage", 0);
                            ci.text_color = AgfInt(ctrl, "TextColor", 0);
                            ci.click_action = ParseClickAction(AgfText(ctrl, "ClickAction", "None"));
                            ci.new_mode_number = AgfInt(ctrl, "NewModeNumber", 0);
                            ci.text_alignment = ParseTextAlignment(AgfText(ctrl, "TextAlignment", "TopMiddle"));
                            ci.clip_image = AgfBool(ctrl, "ClipImage", false);
                            ci.wrap_text = AgfBool(ctrl, "WrapText", false);
                            ci.solid_background = AgfBool(ctrl, "SolidBackground", true);
                        } else if (ci.type_tag == "GUILabel") {
                            ci.text_color = AgfInt(ctrl, "TextColor", 0);
                            ci.text_alignment = ParseTextAlignment(AgfText(ctrl, "TextAlignment", "TopMiddle"));
                            ci.solid_background = AgfBool(ctrl, "SolidBackground", false);
                        } else if (ci.type_tag == "GUISlider") {
                            ci.event_handler = AgfText(ctrl, "OnChange", "");
                            ci.min_value = AgfInt(ctrl, "MinValue", 0);
                            ci.max_value = AgfInt(ctrl, "MaxValue", 10);
                            ci.slider_value = AgfInt(ctrl, "Value", 0);
                            ci.handle_image = AgfInt(ctrl, "HandleImage", 0);
                            ci.handle_offset = AgfInt(ctrl, "HandleOffset", 0);
                            ci.bg_image = AgfInt(ctrl, "BackgroundImage", 0);
                            ci.solid_background = AgfBool(ctrl, "SolidBackground", true);
                        } else if (ci.type_tag == "GUIInventory") {
                            ci.inv_char_id = AgfInt(ctrl, "CharacterID", -1);
                            ci.item_width = AgfInt(ctrl, "ItemWidth", 40);
                            ci.item_height = AgfInt(ctrl, "ItemHeight", 22);
                            ci.solid_background = AgfBool(ctrl, "SolidBackground", false);
                        } else if (ci.type_tag == "GUITextBox") {
                            ci.event_handler = AgfText(ctrl, "OnActivate", "");
                            ci.text_color = AgfInt(ctrl, "TextColor", 0);
                            ci.solid_background = AgfBool(ctrl, "SolidBackground", false);
                        } else if (ci.type_tag == "GUIListBox") {
                            ci.event_handler = AgfText(ctrl, "OnSelectionChanged", "");
                            ci.text_color = AgfInt(ctrl, "TextColor", 0);
                            ci.selected_text_color = AgfInt(ctrl, "SelectedTextColor", 0);
                            ci.selected_bg_color = AgfInt(ctrl, "SelectedBackgroundColor", 0);
                            ci.text_alignment = ParseTextAlignment(AgfText(ctrl, "TextAlignment", "Left"));
                            ci.show_border = AgfBool(ctrl, "ShowBorder", true);
                            ci.show_scroll_arrows = AgfBool(ctrl, "ShowScrollArrows", true);
                            ci.solid_background = AgfBool(ctrl, "SolidBackground", false);
                        } else {
                            ci.solid_background = AgfBool(ctrl, "SolidBackground", false);
                        }
                        g.controls.push_back(ci);
                    }
                }
                game_data_->guis.push_back(g);
        };

    ForEachAgfList<AGS::AGF::GUIsAll>(game_node, collect_single_gui);

    // --- Inventory Items ---
    ForEachAgfList<AGS::AGF::Inventory>(game_node, [this](AgfDocElem inv_elem) {
            GameData::InventoryItemInfo inv;
            inv.id = AgfInt(inv_elem, "ID", 0);
            inv.script_name = AgfText(inv_elem, "Name", "");
            inv.description = AgfText(inv_elem, "Description", "");
            inv.image = AgfInt(inv_elem, "Image", 0);
            inv.cursor_image = AgfInt(inv_elem, "CursorImage", 0);
            inv.start_with = AgfBool(inv_elem, "PlayerStartsWithItem", false);
            // Event handlers
            inv.interactions = LoadInteractions(inv_elem, kInventoryInteractionEvents);
            // Custom properties
            const tinyxml2::XMLElement* props_elem = inv_elem->FirstChildElement("Properties");
            if (props_elem)
            {
                for (const tinyxml2::XMLElement* cp = props_elem->FirstChildElement("CustomProperty");
                     cp; cp = cp->NextSiblingElement("CustomProperty"))
                {
                    std::string pname = AgfText(cp, "Name", "");
                    std::string pval = AgfText(cp, "Value", "");
                    if (!pname.empty())
                        inv.custom_properties[pname] = pval;
                }
            }
            game_data_->inventory_items.push_back(inv);
    });

    // --- Dialogs ---
    ForEachAgfList<AGS::AGF::Dialogs>(game_node, [this](AgfDocElem dlg_elem) {
                    GameData::DialogInfo dlg;
                    dlg.id = AgfInt(dlg_elem, "ID", 0);
                    dlg.name = AgfText(dlg_elem, "Name", "");
                    dlg.script_name = AgfText(dlg_elem, "ScriptName", "");
                    // Fallback: older AGF format uses Name as script name
                    if (dlg.script_name.empty())
                        dlg.script_name = dlg.name;
                    dlg.script = AgfText(dlg_elem, "Script", "");
                    dlg.show_text_parser = AgfBool(dlg_elem, "ShowTextParser", false);
                    dlg.option_count = 0;
                    const tinyxml2::XMLElement* opts = dlg_elem->FirstChildElement("DialogOptions");
                    if (opts)
                    {
                        for (const tinyxml2::XMLElement* opt = opts->FirstChildElement("DialogOption");
                             opt; opt = opt->NextSiblingElement("DialogOption"))
                        {
                            GameData::DialogInfo::DialogOption dopt;
                            dopt.text = AgfText(opt, "Text", "");
                            dopt.show = AgfBool(opt, "Show", true);
                            dopt.say = AgfBool(opt, "Say", true);
                            dlg.options.push_back(dopt);
                        }
                    }
                    dlg.option_count = (int)dlg.options.size();
                    game_data_->dialogs.push_back(dlg);
    });

    // --- Cursors (flat list) ---
    const tinyxml2::XMLElement* cursors_node = game_node->FirstChildElement("Cursors");
    if (cursors_node)
    {
        for (const tinyxml2::XMLElement* cur = cursors_node->FirstChildElement("MouseCursor");
             cur; cur = cur->NextSiblingElement("MouseCursor"))
        {
            GameData::CursorInfo c;
            c.id = AgfInt(cur, "ID", 0);
            c.name = AgfText(cur, "Name", "");
            c.image = AgfInt(cur, "Image", 0);
            c.hotspot_x = AgfInt(cur, "HotspotX", 0);
            c.hotspot_y = AgfInt(cur, "HotspotY", 0);
            c.animate = AgfBool(cur, "Animate", false);
            c.view = AgfInt(cur, "View", 0);
            c.process_click = AgfBool(cur, "StandardMode", false);
            game_data_->cursors.push_back(c);
        }
    }

    // --- Fonts (flat list) ---
    const tinyxml2::XMLElement* fonts_node = game_node->FirstChildElement("Fonts");
    if (fonts_node)
    {
        for (const tinyxml2::XMLElement* fnt = fonts_node->FirstChildElement("Font");
             fnt; fnt = fnt->NextSiblingElement("Font"))
        {
            GameData::FontInfo f;
            f.id = AgfInt(fnt, "ID", 0);
            f.name = AgfText(fnt, "Name", "");
            f.size = AgfInt(fnt, "PointSize", 10);
            f.source_file = AgfText(fnt, "SourceFilename", "");
            f.line_spacing = AgfInt(fnt, "LineSpacing", 0);
            f.size_multiplier = AgfInt(fnt, "SizeMultiplier", 1);
            const char* outline = AgfText(fnt, "OutlineStyle", "None");
            if (strcmp(outline, "None") == 0) f.outline_type = 0;
            else if (strcmp(outline, "Automatic") == 0) f.outline_type = 1;
            else if (strcmp(outline, "UseOutlineFont") == 0) f.outline_type = 2;
            f.outline_font = AgfInt(fnt, "OutlineFont", -1);
            game_data_->fonts.push_back(f);
        }
    }

    // --- Views ---
    ForEachAgfList<AGS::AGF::Views>(game_node, [this](AgfDocElem view_elem) {
                    GameData::ViewInfo v;
                    v.id = AgfInt(view_elem, "ID", 0);
                    v.name = AgfText(view_elem, "Name", "");
                    v.loop_count = 0;
                    const tinyxml2::XMLElement* loops = view_elem->FirstChildElement("Loops");
                    if (loops)
                    {
                        for (const tinyxml2::XMLElement* loop_elem = loops->FirstChildElement("Loop");
                             loop_elem; loop_elem = loop_elem->NextSiblingElement("Loop"))
                        {
                            GameData::LoopData loop;
                            loop.run_next_loop = AgfBool(loop_elem, "RunNextLoop", false);
                            const tinyxml2::XMLElement* frames = loop_elem->FirstChildElement("Frames");
                            if (frames)
                            {
                                for (const tinyxml2::XMLElement* fr = frames->FirstChildElement("ViewFrame");
                                     fr; fr = fr->NextSiblingElement("ViewFrame"))
                                {
                                    GameData::FrameData frame;
                                    frame.sprite_id = AgfInt(fr, "Image", 0);
                                    frame.x_offset = AgfInt(fr, "OffsetX", 0);
                                    frame.y_offset = AgfInt(fr, "OffsetY", 0);
                                    frame.delay = AgfInt(fr, "Delay", 0);
                                    frame.flipped = AgfBool(fr, "Flipped", false);
                                    frame.sound = AgfInt(fr, "Sound", -1);
                                    loop.frames.push_back(frame);
                                }
                            }
                            v.loops.push_back(loop);
                            v.loop_count++;
                        }
                    }
                    game_data_->views.push_back(v);
    });

    // --- Scripts ---
    ForEachAgfList<AGS::AGF::ScriptModules>(game_node, [this](AgfDocElem sah_elem) {
                    GameData::ScriptModule mod;
                    const tinyxml2::XMLElement* hdr_wrap = sah_elem->FirstChildElement("ScriptAndHeader_Header");
                    if (hdr_wrap) {
                        const tinyxml2::XMLElement* hdr_script = hdr_wrap->FirstChildElement("Script");
                        if (hdr_script) {
                            mod.header_file = AgfText(hdr_script, "FileName", "");
                            mod.name = AgfText(hdr_script, "Name", "");
                        }
                    }
                    const tinyxml2::XMLElement* scr_wrap = sah_elem->FirstChildElement("ScriptAndHeader_Script");
                    if (scr_wrap) {
                        const tinyxml2::XMLElement* scr_script = scr_wrap->FirstChildElement("Script");
                        if (scr_script) {
                            mod.script_file = AgfText(scr_script, "FileName", "");
                            if (mod.name.empty())
                                mod.name = AgfText(scr_script, "Name", "");
                        }
                    }
                    if (!mod.script_file.empty() || !mod.header_file.empty())
                    {
                        // Derive name from filename if Name element was empty
                        if (mod.name.empty())
                        {
                            const std::string& ref = !mod.header_file.empty() ? mod.header_file : mod.script_file;
                            // Strip extension (.ash or .asc) to get module name
                            auto dot = ref.rfind('.');
                            mod.name = (dot != std::string::npos) ? ref.substr(0, dot) : ref;
                        }
                        game_data_->script_modules.push_back(mod);
                    }
    });

    // --- Audio Clips ---
    ForEachAgfList<AGS::AGF::AudioClips>(game_node, [this](AgfDocElem clip_elem) {
        GameData::AudioClipInfo clip;
        clip.id = AgfInt(clip_elem, "ID", 0);
        clip.fixed_index = AgfInt(clip_elem, "Index", 0);
        clip.name = AgfText(clip_elem, "ScriptName", "");
        if (clip.name.empty())
            clip.name = AgfText(clip_elem, "Name", "");
        clip.filename = NormalizePath(AgfText(clip_elem, "FileName", ""));
        clip.source_filename = NormalizePath(AgfText(clip_elem, "SourceFileName", ""));
        clip.type = AgfInt(clip_elem, "Type", 0);
        clip.default_volume = AgfInt(clip_elem, "DefaultVolume", -1);
        clip.default_priority = AgfInt(clip_elem, "DefaultPriority", 0);
        clip.default_repeat = AgfInt(clip_elem, "DefaultRepeat", 0);
        clip.bundling_type = AgfInt(clip_elem, "BundlingType", 0);
        clip.file_type = AgfInt(clip_elem, "FileType", 0);
        game_data_->audio_clips.push_back(clip);
    });

    // --- Audio Clip Types ---
    const tinyxml2::XMLElement* audio_types_node = game_node->FirstChildElement("AudioClipTypes");
    if (audio_types_node)
    {
        for (const tinyxml2::XMLElement* ct = audio_types_node->FirstChildElement("AudioClipType");
             ct; ct = ct->NextSiblingElement("AudioClipType"))
        {
            GameData::AudioClipTypeInfo ctype;
            ctype.id = AgfInt(ct, "TypeID", 0);
            ctype.name = AgfText(ct, "Name", "");
            ctype.max_channels = AgfInt(ct, "MaxChannels", 0);
            ctype.volume_reduction_while_speech = AgfInt(ct, "VolumeReductionWhileSpeechPlaying", 0);
            std::string crossfade = AgfText(ct, "CrossfadeClips", "No");
            if (crossfade == "Slow") ctype.crossfade_speed = 1;
            else if (crossfade == "Fast") ctype.crossfade_speed = 2;
            else ctype.crossfade_speed = 0;
            ctype.backwards_compat_type = AgfBool(ct, "BackwardsCompatibilityType", false);
            game_data_->audio_clip_types.push_back(ctype);
        }
    }

    // --- Translations (registered in Game.agf only) ---
    game_data_->translations.clear();
    const tinyxml2::XMLElement *trans_root = game_node->FirstChildElement("Translations");
    if (trans_root)
    {
        for (const tinyxml2::XMLElement *tr = trans_root->FirstChildElement("Translation");
             tr; tr = tr->NextSiblingElement("Translation"))
        {
            GameData::TranslationInfo info;
            info.name = AgfText(tr, "Name", "");
            if (!info.name.empty())
                game_data_->translations.push_back(info);
        }
    }

    // --- Global Variables ---
    ForEachAgfList<AGS::AGF::GlobalVariables>(game_node, [this](AgfDocElem gv_elem) {
        GameData::GlobalVariableInfo gv;
        gv.name = AgfText(gv_elem, "Name", "");
        gv.type_name = AgfText(gv_elem, "Type", "int");
        gv.default_value = AgfText(gv_elem, "DefaultValue", "");
        gv.array_type = AgfInt(gv_elem, "ArrayType", 0);
        gv.array_size = AgfInt(gv_elem, "ArraySize", 0);
        if (!gv.name.empty())
            game_data_->global_variables.push_back(gv);
    });

    // --- Lip Sync ---
    const tinyxml2::XMLElement* ls_node = game_node->FirstChildElement("LipSync");
    if (ls_node)
    {
        std::string type_str = AgfText(ls_node, "Type", "None");
        if (type_str == "Text")
            game_data_->lip_sync_type = 1;
        else if (type_str == "PamelaVoiceFiles")
            game_data_->lip_sync_type = 2;
        else
            game_data_->lip_sync_type = 0;

        game_data_->default_lipsync_frame = AgfInt(ls_node, "DefaultFrame", 0);

        const tinyxml2::XMLElement* frames_node = ls_node->FirstChildElement("Frames");
        if (frames_node)
        {
            int idx = 0;
            for (const tinyxml2::XMLElement* cf = frames_node->FirstChildElement("CharsForFrame");
                 cf && idx < GameData::kMaxLipSyncFrames;
                 cf = cf->NextSiblingElement("CharsForFrame"), idx++)
            {
                const char* text = cf->GetText();
                game_data_->lip_sync_chars_per_frame[idx] = text ? text : "";
            }
        }
    }

    // --- Custom Property Definitions ---
    const tinyxml2::XMLElement* propdef_node = game_node->FirstChildElement("PropertyDefinitions");
    if (propdef_node)
    {
        for (const tinyxml2::XMLElement* item = propdef_node->FirstChildElement("CustomPropertySchemaItem");
             item; item = item->NextSiblingElement("CustomPropertySchemaItem"))
        {
            GameData::CustomPropertySchemaInfo prop;
            prop.name = AgfText(item, "Name", "");
            prop.description = AgfText(item, "Description", "");
            prop.default_value = AgfText(item, "DefaultValue", "");
            // Type: "Boolean"->0, "Number"->1, "Text"->2
            const char* type_str = AgfText(item, "Type", "Boolean");
            if (strcmp(type_str, "Number") == 0)
                prop.type = 1;
            else if (strcmp(type_str, "Text") == 0 || strcmp(type_str, "String") == 0)
                prop.type = 2;
            else
                prop.type = 0; // Boolean
            prop.applies_to_characters = AgfBool(item, "AppliesToCharacters", false);
            prop.applies_to_hotspots = AgfBool(item, "AppliesToHotspots", false);
            prop.applies_to_objects = AgfBool(item, "AppliesToObjects", false);
            prop.applies_to_rooms = AgfBool(item, "AppliesToRooms", false);
            prop.applies_to_inv_items = AgfBool(item, "AppliesToInvItems", false);
            prop.translated = AgfBool(item, "Translated", false);
            if (!prop.name.empty())
                game_data_->custom_property_schemas.push_back(prop);
        }
    }
    fprintf(stderr, "  Custom property schemas: %d\n",
            (int)game_data_->custom_property_schemas.size());

    // --- Palette ---
    const tinyxml2::XMLElement* palette_node = game_node->FirstChildElement("Palette");
    if (palette_node)
    {
        for (const tinyxml2::XMLElement* pe = palette_node->FirstChildElement("PaletteEntry");
             pe; pe = pe->NextSiblingElement("PaletteEntry"))
        {
            GameData::PaletteEntry entry;
            entry.r = AgfInt(pe, "Red", 0);
            entry.g = AgfInt(pe, "Green", 0);
            entry.b = AgfInt(pe, "Blue", 0);
            const char* usage = AgfText(pe, "ColourType", "Background");
            if (strcmp(usage, "Gamewide") == 0)
                entry.colour_type = 0;
            else if (strcmp(usage, "Locked") == 0)
                entry.colour_type = 1;
            else
                entry.colour_type = 2; // Background
            game_data_->palette.push_back(entry);
        }
    }
    if (game_data_->palette.empty())
    {
        for (int i = 0; i < 256; i++)
        {
            GameData::PaletteEntry entry;
            entry.r = (i * 4) & 0xFF;
            entry.g = (i * 2) & 0xFF;
            entry.b = (i * 6) & 0xFF;
            entry.colour_type = (i < 42) ? 0 : 2; // Gamewide for first 42, Background for rest
            game_data_->palette.push_back(entry);
        }
    }

    // --- Global Messages (legacy AGS 2.x) ---
    const tinyxml2::XMLElement* gm_node = game_node->FirstChildElement("GlobalMessages");
    if (gm_node)
    {
        int gm_count = 0;
        for (const tinyxml2::XMLElement* msg = gm_node->FirstChildElement("Message");
             msg; msg = msg->NextSiblingElement("Message"))
        {
            int id = 0;
            if (msg->QueryIntAttribute("ID", &id) == tinyxml2::XML_SUCCESS)
            {
                int idx = id - GameData::GLOBAL_MESSAGE_ID_START;
                if (idx >= 0 && idx < GameData::NUM_GLOBAL_MESSAGES)
                {
                    const char* text = msg->GetText();
                    game_data_->global_messages[idx] = text ? text : "";
                    if (!game_data_->global_messages[idx].empty())
                        gm_count++;
                }
            }
        }
        if (gm_count > 0)
            fprintf(stderr, "  Global messages: %d\n", gm_count);
    }

    // --- Sprites (folder tree + source metadata) ---
    // Build a temporary map of sprite source info keyed by slot ID.
    // The actual sprite dimensions come from the binary sprite file (acsprset.spr)
    // but source metadata (import paths, offsets, etc.) is only in the AGF XML.
    {
        std::map<int, GameData::SpriteSourceInfo> source_map;
        std::map<int, std::string> resolution_map;
        std::map<int, bool> alpha_map;
        std::map<int, int> colours_locked_map;

        // Lambda to parse a single <Sprite> element attributes + <Source> child
        auto parse_sprite = [&](AgfDocElem spr_elem) {
            int slot = 0;
            spr_elem->QueryIntAttribute("Slot", &slot);

            // Parse attributes
            const char* res = spr_elem->Attribute("Resolution");
            if (res) resolution_map[slot] = res;

            bool alpha = false;
            if (spr_elem->QueryBoolAttribute("AlphaChannel", &alpha) == tinyxml2::XML_SUCCESS)
                alpha_map[slot] = alpha;

            int locked = -1;
            if (spr_elem->QueryIntAttribute("ColoursLockedToRoom", &locked) == tinyxml2::XML_SUCCESS)
                colours_locked_map[slot] = locked;

            // Parse <Source> child element
            const tinyxml2::XMLElement* src = spr_elem->FirstChildElement("Source");
            if (src) {
                GameData::SpriteSourceInfo info;
                info.source_file = NormalizePath(AgfText(src, "FileName", ""));
                info.offset_x = AgfInt(src, "OffsetX", 0);
                info.offset_y = AgfInt(src, "OffsetY", 0);
                info.import_width = AgfInt(src, "ImportWidth", 0);
                info.import_height = AgfInt(src, "ImportHeight", 0);
                info.import_as_tile = AgfBool(src, "ImportAsTile", false);
                info.frame = AgfInt(src, "Frame", 0);
                source_map[slot] = std::move(info);
            }
        };

        ForEachAgfList<AGS::AGF::Sprites>(game_node, parse_sprite);

        // Store source metadata count for logging
        int source_count = (int)source_map.size();

        // Apply parsed metadata to existing SpriteInfo entries (or create them)
        // Note: sprites vector may already be populated from binary sprite file,
        // or may be populated later in InitSubsystems.
        // We store the maps and apply them after the sprites vector is ready.
        // For now, store in a temporary map on game_data_ via the sprites vector.
        for (auto& spr : game_data_->sprites) {
            auto it_src = source_map.find(spr.id);
            if (it_src != source_map.end()) {
                spr.source = std::move(it_src->second);
                source_map.erase(it_src);
            }
            auto it_res = resolution_map.find(spr.id);
            if (it_res != resolution_map.end()) {
                spr.resolution = it_res->second;
                resolution_map.erase(it_res);
            }
            auto it_alpha = alpha_map.find(spr.id);
            if (it_alpha != alpha_map.end()) {
                spr.alpha_channel = it_alpha->second;
                alpha_map.erase(it_alpha);
            }
            auto it_lock = colours_locked_map.find(spr.id);
            if (it_lock != colours_locked_map.end()) {
                spr.colours_locked_to_room = it_lock->second;
                colours_locked_map.erase(it_lock);
            }
        }

        // Store remaining (not yet matched) source metadata in a temporary map
        // for InitSubsystems to pick up when it populates sprites from the binary file.
        game_data_->pending_sprite_source_ = std::move(source_map);
        game_data_->pending_sprite_resolution_ = std::move(resolution_map);
        game_data_->pending_sprite_alpha_ = std::move(alpha_map);
        game_data_->pending_sprite_colours_locked_ = std::move(colours_locked_map);

        if (source_count > 0)
            fprintf(stderr, "  Sprite source metadata: %d entries\n", source_count);
    }

    // Sort all ID-indexed arrays by their ID field.
    // The DTA binary format expects entities at array[i] to have ID==i,
    // because the engine assigns IDs based on array position.
    // AGF stores items in folder-traversal order, which may differ from ID order.
    std::sort(game_data_->characters.begin(), game_data_->characters.end(),
        [](const GameData::CharacterInfo& a, const GameData::CharacterInfo& b) {
            return a.id < b.id;
        });
    std::sort(game_data_->views.begin(), game_data_->views.end(),
        [](const GameData::ViewInfo& a, const GameData::ViewInfo& b) {
            return a.id < b.id;
        });
    std::sort(game_data_->guis.begin(), game_data_->guis.end(),
        [](const GameData::GUIInfo& a, const GameData::GUIInfo& b) {
            return a.id < b.id;
        });
    std::sort(game_data_->inventory_items.begin(), game_data_->inventory_items.end(),
        [](const GameData::InventoryItemInfo& a, const GameData::InventoryItemInfo& b) {
            return a.id < b.id;
        });
    std::sort(game_data_->dialogs.begin(), game_data_->dialogs.end(),
        [](const GameData::DialogInfo& a, const GameData::DialogInfo& b) {
            return a.id < b.id;
        });
    std::sort(game_data_->audio_clips.begin(), game_data_->audio_clips.end(),
        [](const GameData::AudioClipInfo& a, const GameData::AudioClipInfo& b) {
            return a.id < b.id;
        });
    std::sort(game_data_->cursors.begin(), game_data_->cursors.end(),
        [](const GameData::CursorInfo& a, const GameData::CursorInfo& b) {
            return a.id < b.id;
        });
    std::sort(game_data_->fonts.begin(), game_data_->fonts.end(),
        [](const GameData::FontInfo& a, const GameData::FontInfo& b) {
            return a.id < b.id;
        });

    fprintf(stderr, "[Project] Loaded AGF project '%s':\n", agf_path.c_str());
    fprintf(stderr, "  Title: %s\n", game_data_->game_title.c_str());
    fprintf(stderr, "  Resolution: %dx%d, %d-bit\n",
            game_data_->resolution_width, game_data_->resolution_height, game_data_->color_depth);
    fprintf(stderr, "  Characters: %d, Views: %d, GUIs: %d, Dialogs: %d\n",
            (int)game_data_->characters.size(), (int)game_data_->views.size(),
            (int)game_data_->guis.size(), (int)game_data_->dialogs.size());
    fprintf(stderr, "  Rooms: %d, Scripts: %d, Fonts: %d, Cursors: %d\n",
            (int)game_data_->rooms.size(), (int)game_data_->script_modules.size(),
            (int)game_data_->fonts.size(), (int)game_data_->cursors.size());
    fprintf(stderr, "  Inventory: %d, Audio: %d\n",
            (int)game_data_->inventory_items.size(), (int)game_data_->audio_clips.size());

    return true;
}


bool Project::OpenProject(const std::string& path)
{
    CloseProject();

    project_path_ = ResolveToAbsolutePath(path);
    UpdateProjectDir();

    std::string ext = fs::path(project_path_).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    if (ext != ".agf")
    {
        fprintf(stderr, "[Project] Only .agf project files are supported (got '%s').\n", ext.c_str());
        return false;
    }

    if (!LoadFromAGF(project_path_))
    {
        fprintf(stderr, "[Project] Failed to load AGF project '%s'.\n", project_path_.c_str());
        return false;
    }

    loaded_ = true;
    return true;
}

void Project::CloseProject()
{
    loaded_ = false;
    project_path_.clear();
    project_dir_.clear();
    game_data_.reset();
}

} // namespace AGSBuild
