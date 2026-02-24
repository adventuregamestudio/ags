// AGS Editor ImGui - Project management implementation
#include "project.h"
#include "game_data.h"
#include "sprite_loader.h"
#include "room_loader.h"
#include "script_api_data.h"
#include "old_game_importer.h"
#include "core/path_utils.h"
#include "compiler/compiler_bridge.h"

// AGS Common library headers for loading game data
#include "ac/gamesetupstruct.h"
#include "ac/spritefile.h"
#include "ac/view.h"
#include "ac/dialogtopic.h"
#include "gui/guidefines.h"
#include "game/main_game_file.h"
#include "game/room_file.h"
#include "util/file.h"
#include "util/string.h"

// TinyXML2 for parsing .agf XML project files
#include "tinyxml2.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <unordered_map>

namespace fs = std::filesystem;

namespace AGSEditor
{

// ---- RecentFiles ----

static const char* kRecentFileName = "recent_games.txt";

std::string RecentFiles::GetConfigPath() const
{
    std::string config_dir;
#ifdef _WIN32
    const char* appdata = getenv("APPDATA");
    if (appdata)
        config_dir = std::string(appdata) + "/AGSEditor";
#else
    const char* home = getenv("HOME");
    if (home)
        config_dir = std::string(home) + "/.config/ags_editor";
#endif
    if (config_dir.empty())
        config_dir = ".";

    // Create directory if it doesn't exist
    try { fs::create_directories(config_dir); } catch (...) {}

    return config_dir + "/" + kRecentFileName;
}

void RecentFiles::Load()
{
    files_.clear();
    std::string path = GetConfigPath();
    std::ifstream in(path);
    if (!in.is_open())
        return;

    std::string line;
    while (std::getline(in, line))
    {
        if (!line.empty() && files_.size() < kMaxRecentFiles)
            files_.push_back(line);
    }
}

void RecentFiles::Save() const
{
    std::string path = GetConfigPath();
    std::ofstream out(path);
    if (!out.is_open())
        return;

    for (const auto& f : files_)
        out << f << "\n";
}

void RecentFiles::AddFile(const std::string& path)
{
    // Remove duplicates
    files_.erase(std::remove(files_.begin(), files_.end(), path), files_.end());
    // Insert at front
    files_.insert(files_.begin(), path);
    // Trim to max size
    if ((int)files_.size() > kMaxRecentFiles)
        files_.resize(kMaxRecentFiles);
    Save();
}

void RecentFiles::Clear()
{
    files_.clear();
    Save();
}

// ---- Project ----

Project::Project()
{
    recent_files_.Load();
}

Project::~Project() = default;

void Project::UpdateProjectDir()
{
    if (!project_path_.empty())
    {
        fs::path p(project_path_);
        project_dir_ = p.parent_path().string();
    }
    else
    {
        project_dir_.clear();
    }
}

bool Project::CreateDefaultGameData(const std::string& title, int width, int height, int color_depth)
{
    game_data_ = std::make_unique<GameData>();
    game_data_->game_title = title;
    game_data_->resolution_width = width;
    game_data_->resolution_height = height;
    game_data_->color_depth = color_depth;

    // Create default game entities
    // Default character (player)
    GameData::CharacterInfo player;
    player.id = 0;
    player.script_name = "cEgo";
    player.real_name = "Ego";
    player.room = 1;
    player.x = 160;
    player.y = 120;
    game_data_->characters.push_back(player);

    // Default room
    GameData::RoomInfo room;
    room.number = 1;
    room.description = "Room 1";
    game_data_->rooms.push_back(room);

    // Default cursors
    const char* cursor_names[] = { "Walk to", "Look at", "Interact", "Talk to", "Use inventory", "Pick up", "Wait", "Arrow" };
    for (int i = 0; i < 8; i++)
    {
        GameData::CursorInfo cursor;
        cursor.id = i;
        cursor.name = cursor_names[i];
        cursor.image = 0;
        cursor.hotspot_x = 0;
        cursor.hotspot_y = 0;
        cursor.animate = false;
        cursor.view = 0;
        cursor.process_click = (i <= 3 || i == 5);
        game_data_->cursors.push_back(cursor);
    }

    // Default script module
    GameData::ScriptModule global_script;
    global_script.name = "GlobalScript";
    global_script.header_file = "GlobalScript.ash";
    global_script.script_file = "GlobalScript.asc";
    game_data_->script_modules.push_back(global_script);

    // Default fonts
    GameData::FontInfo font;
    font.id = 0;
    font.name = "Default Font";
    font.size = 10;
    game_data_->fonts.push_back(font);

    GameData::FontInfo speech_font;
    speech_font.id = 1;
    speech_font.name = "Speech Font";
    speech_font.size = 10;
    game_data_->fonts.push_back(speech_font);

    // Default palette (standard VGA palette start)
    for (int i = 0; i < 256; i++)
    {
        GameData::PaletteEntry entry;
        entry.r = (i * 4) & 0xFF;
        entry.g = (i * 2) & 0xFF;
        entry.b = (i * 6) & 0xFF;
        entry.colour_type = (i < 42) ? 0 : 2;
        game_data_->palette.push_back(entry);
    }

    return true;
}

bool Project::NewProject(const std::string& path, const std::string& title,
                         int width, int height, int color_depth)
{
    CloseProject();

    project_path_ = path;
    game_title_ = title;
    UpdateProjectDir();

    // Create the project directory if it doesn't exist
    try
    {
        fs::create_directories(project_dir_);
    }
    catch (const std::exception& e)
    {
        fprintf(stderr, "Failed to create project directory: %s\n", e.what());
        return false;
    }

    if (!CreateDefaultGameData(title, width, height, color_depth))
        return false;

    loaded_ = true;
    dirty_ = true;

    // Initialize subsystems
    sprite_loader_ = std::make_unique<SpriteLoader>();
    room_loader_ = std::make_unique<RoomLoader>();
    room_loader_->SetGameDir(project_dir_);
    room_loader_->SetGameIsHires(width > 320);
    room_loader_->SetSpriteLoader(sprite_loader_.get());
    script_api_data_ = std::make_unique<ScriptAPIData>();

    recent_files_.AddFile(project_path_);

    return true;
}

void Project::InitSubsystems(const std::string& game_dir)
{
    sprite_loader_ = std::make_unique<SpriteLoader>();
    if (sprite_loader_->Open(game_dir))
    {
        fprintf(stderr, "[Project] Sprite loader opened with %d sprites.\n",
                sprite_loader_->GetSpriteCount());
        if (game_data_)
        {
            // If sprites list is empty (loaded from AGF, not old game import),
            // populate it from the sprite file so WriteSpriteFlags knows the count
            if (game_data_->sprites.empty())
            {
                const auto& all = sprite_loader_->GetAllMetrics();
                for (const auto& m : all)
                {
                    if (m.exists)
                    {
                        GameData::SpriteInfo s;
                        s.id = m.id;
                        s.width = m.width;
                        s.height = m.height;
                        s.color_depth = m.color_depth * 8;
                        // Apply pending source metadata from AGF XML parsing
                        auto it = game_data_->pending_sprite_source_.find(s.id);
                        if (it != game_data_->pending_sprite_source_.end()) {
                            s.source = std::move(it->second);
                            game_data_->pending_sprite_source_.erase(it);
                        }
                        auto it_res = game_data_->pending_sprite_resolution_.find(s.id);
                        if (it_res != game_data_->pending_sprite_resolution_.end()) {
                            s.resolution = it_res->second;
                            game_data_->pending_sprite_resolution_.erase(it_res);
                        }
                        auto it_alpha = game_data_->pending_sprite_alpha_.find(s.id);
                        if (it_alpha != game_data_->pending_sprite_alpha_.end()) {
                            s.alpha_channel = it_alpha->second;
                            game_data_->pending_sprite_alpha_.erase(it_alpha);
                        }
                        auto it_lock = game_data_->pending_sprite_colours_locked_.find(s.id);
                        if (it_lock != game_data_->pending_sprite_colours_locked_.end()) {
                            s.colours_locked_to_room = it_lock->second;
                            game_data_->pending_sprite_colours_locked_.erase(it_lock);
                        }
                        game_data_->sprites.push_back(s);
                    }
                }
            }
            else
            {
                for (auto& spr : game_data_->sprites)
                {
                    auto* m = sprite_loader_->GetMetrics(spr.id);
                    if (m && m->exists)
                    {
                        spr.width = m->width;
                        spr.height = m->height;
                        spr.color_depth = m->color_depth * 8;
                    }
                }
            }
        }
    }

    room_loader_ = std::make_unique<RoomLoader>();
    room_loader_->SetGameDir(game_dir);
    room_loader_->SetSpriteLoader(sprite_loader_.get());
    if (game_data_)
        room_loader_->SetGameIsHires(game_data_->resolution_width > 320);

    script_api_data_ = std::make_unique<ScriptAPIData>();
}

// -------------------------------------------------------------------------
// XML helper functions for AGF parsing
// -------------------------------------------------------------------------

static const char* XmlChildText(const tinyxml2::XMLElement* parent, const char* name, const char* def = "")
{
    if (!parent) return def;
    const tinyxml2::XMLElement* child = parent->FirstChildElement(name);
    if (!child) return def;
    const char* text = child->GetText();
    return text ? text : def;
}

static int XmlChildInt(const tinyxml2::XMLElement* parent, const char* name, int def = 0)
{
    if (!parent) return def;
    const tinyxml2::XMLElement* child = parent->FirstChildElement(name);
    if (!child) return def;
    const char* text = child->GetText();
    if (!text) return def;
    return atoi(text);
}

static bool XmlChildBool(const tinyxml2::XMLElement* parent, const char* name, bool def = false)
{
    const char* text = XmlChildText(parent, name, nullptr);
    if (!text) return def;
    return (strcmp(text, "True") == 0 || strcmp(text, "true") == 0 || strcmp(text, "1") == 0);
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

    result.script_module = XmlChildText(inter, "ScriptModule", "");

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

// Save Interactions as an <Interactions> child element
static void SaveInteractions(tinyxml2::XMLDocument& doc,
                             tinyxml2::XMLElement* parent_elem,
                             const Interactions& interactions)
{
    tinyxml2::XMLElement* inter = doc.NewElement("Interactions");
    parent_elem->InsertEndChild(inter);

    if (!interactions.script_module.empty())
    {
        tinyxml2::XMLElement* sm = doc.NewElement("ScriptModule");
        sm->SetText(interactions.script_module.c_str());
        inter->InsertEndChild(sm);
    }

    for (int i = 0; i < (int)interactions.handler_functions.size(); ++i)
    {
        tinyxml2::XMLElement* ev = doc.NewElement("Event");
        ev->SetAttribute("Index", i);
        if (!interactions.handler_functions[i].empty())
            ev->SetText(interactions.handler_functions[i].c_str());
        inter->InsertEndChild(ev);
    }
}

// Recursively collect items from an AGF folder structure.
// Matches C# AllItemsFlat: subfolders first, then items at current level.
template<typename Callback>
static void CollectFolderItems(const tinyxml2::XMLElement* folder_elem,
                               const char* items_list_tag,
                               const char* item_tag,
                               Callback cb)
{
    if (!folder_elem) return;

    // Process subfolders first (matches C# BaseFolderCollection.AllItemsFlat)
    const tinyxml2::XMLElement* subfolders = folder_elem->FirstChildElement("SubFolders");
    if (subfolders)
    {
        const char* folder_tag = folder_elem->Name();
        for (const tinyxml2::XMLElement* sub = subfolders->FirstChildElement(folder_tag);
             sub; sub = sub->NextSiblingElement(folder_tag))
        {
            CollectFolderItems(sub, items_list_tag, item_tag, cb);
        }
    }

    // Then items at this level
    const tinyxml2::XMLElement* items = folder_elem->FirstChildElement(items_list_tag);
    if (items)
    {
        for (const tinyxml2::XMLElement* item = items->FirstChildElement(item_tag);
             item; item = item->NextSiblingElement(item_tag))
        {
            cb(item);
        }
    }
}

// Variant that also builds a FolderInfo tree alongside flat collection.
// The IdExtractor must return the item's integer ID from the XML element.
template<typename Callback, typename IdExtractor>
static void CollectFolderItemsWithTree(const tinyxml2::XMLElement* folder_elem,
                                        const char* items_list_tag,
                                        const char* item_tag,
                                        FolderInfo& folder_out,
                                        Callback cb,
                                        IdExtractor id_fn)
{
    if (!folder_elem) return;

    const char* name = folder_elem->Attribute("Name");
    if (name) folder_out.name = name;

    // Process subfolders first
    const tinyxml2::XMLElement* subfolders = folder_elem->FirstChildElement("SubFolders");
    if (subfolders)
    {
        const char* folder_tag = folder_elem->Name();
        for (const tinyxml2::XMLElement* sub = subfolders->FirstChildElement(folder_tag);
             sub; sub = sub->NextSiblingElement(folder_tag))
        {
            FolderInfo child;
            CollectFolderItemsWithTree(sub, items_list_tag, item_tag, child, cb, id_fn);
            folder_out.subfolders.push_back(std::move(child));
        }
    }

    // Then items at this level
    const tinyxml2::XMLElement* items = folder_elem->FirstChildElement(items_list_tag);
    if (items)
    {
        for (const tinyxml2::XMLElement* item = items->FirstChildElement(item_tag);
             item; item = item->NextSiblingElement(item_tag))
        {
            int id = id_fn(item);
            folder_out.item_ids.push_back(id);
            cb(item);
        }
    }
}

// Recursively write a FolderInfo tree to XML.
// folder_tag: tag name for folder elements (e.g., "CharacterFolder")
// items_list_tag: tag name for items container (e.g., "Characters")
// write_item: callback(tinyxml2::XMLDocument&, tinyxml2::XMLElement* parent, int item_id)
template<typename WriteItemFn>
static void WriteFolderTree(tinyxml2::XMLDocument& doc,
                            tinyxml2::XMLElement* parent_folder,
                            const FolderInfo& folder,
                            const char* folder_tag,
                            const char* items_list_tag,
                            WriteItemFn write_item)
{
    parent_folder->SetAttribute("Name", folder.name.c_str());

    // SubFolders
    tinyxml2::XMLElement* sub_folders = doc.NewElement("SubFolders");
    parent_folder->InsertEndChild(sub_folders);
    for (const auto& child : folder.subfolders)
    {
        tinyxml2::XMLElement* child_folder = doc.NewElement(folder_tag);
        sub_folders->InsertEndChild(child_folder);
        WriteFolderTree(doc, child_folder, child, folder_tag, items_list_tag, write_item);
    }

    // Items at this level
    tinyxml2::XMLElement* items_list = doc.NewElement(items_list_tag);
    parent_folder->InsertEndChild(items_list);
    for (int id : folder.item_ids)
    {
        write_item(doc, items_list, id);
    }
}

// -------------------------------------------------------------------------
// LoadFromAGF — Parse .agf XML project file
// -------------------------------------------------------------------------
bool Project::LoadFromAGF(const std::string& agf_path)
{
    tinyxml2::XMLDocument doc;
    tinyxml2::XMLError xml_err = doc.LoadFile(agf_path.c_str());
    if (xml_err != tinyxml2::XML_SUCCESS)
    {
        fprintf(stderr, "[Project] Failed to parse XML '%s': %s\n",
                agf_path.c_str(), doc.ErrorStr());
        return false;
    }

    const tinyxml2::XMLElement* root = doc.RootElement();
    if (!root || strcmp(root->Name(), "AGSEditorDocument") != 0)
    {
        fprintf(stderr, "[Project] '%s' is not a valid AGS project file.\n", agf_path.c_str());
        return false;
    }

    const tinyxml2::XMLElement* game_node = root->FirstChildElement("Game");
    if (!game_node)
    {
        fprintf(stderr, "[Project] '%s' has no <Game> element.\n", agf_path.c_str());
        return false;
    }

    game_data_ = std::make_unique<GameData>();
    std::string game_dir = fs::path(agf_path).parent_path().string();

    // Read saved editor version from root element attribute
    const char* editor_ver = root->Attribute("EditorVersion");
    if (editor_ver)
        game_data_->saved_editor_version = editor_ver;

    // --- Settings ---
    const tinyxml2::XMLElement* settings = game_node->FirstChildElement("Settings");
    if (settings)
    {
        game_data_->game_title = XmlChildText(settings, "GameName", "Untitled");
        game_title_ = game_data_->game_title;

        // Game identity
        game_data_->unique_id = XmlChildInt(settings, "UniqueID", 0);
        game_data_->guid_string = XmlChildText(settings, "GUIDAsString", "");

        const char* res_text = XmlChildText(settings, "CustomResolution", nullptr);
        if (res_text && strlen(res_text) > 0)
            ParseResolution(res_text, game_data_->resolution_width, game_data_->resolution_height);

        const char* depth_text = XmlChildText(settings, "ColorDepth", "TrueColor");
        if (depth_text)
        {
            if (strcmp(depth_text, "TrueColor") == 0 || strcmp(depth_text, "32") == 0)
                game_data_->color_depth = 32;
            else if (strcmp(depth_text, "HighColor") == 0 || strcmp(depth_text, "16") == 0)
                game_data_->color_depth = 16;
            else if (strcmp(depth_text, "Palette") == 0 || strcmp(depth_text, "8") == 0)
                game_data_->color_depth = 8;
        }

        game_data_->debug_mode = XmlChildBool(settings, "DebugMode", false);

        // Extended settings
        game_data_->anti_alias_fonts = XmlChildBool(settings, "AntiAliasFonts", false);
        game_data_->target_fps = XmlChildInt(settings, "GameFPS", 40);
        game_data_->description = XmlChildText(settings, "Description", "");
        game_data_->developer_name = XmlChildText(settings, "DeveloperName", "");
        game_data_->developer_url = XmlChildText(settings, "DeveloperURL", "");
        game_data_->max_score = XmlChildInt(settings, "MaximumScore", 0);
        game_data_->save_screenshots = XmlChildBool(settings, "SaveScreenshots", false);
        game_data_->pixel_perfect_speed = XmlChildBool(settings, "AntiGlideMode", true);
        game_data_->pixel_perfect = XmlChildBool(settings, "PixelPerfect", true);
        game_data_->walk_in_look_mode = XmlChildBool(settings, "WalkInLookMode", false);
        game_data_->mouse_wheel_enabled = XmlChildBool(settings, "MouseWheelEnabled", true);
        game_data_->auto_move_in_walk_mode = XmlChildBool(settings, "AutoMoveInWalkMode", true);
        game_data_->letterbox_mode = XmlChildBool(settings, "LetterboxMode", false);
        game_data_->backwards_text = XmlChildBool(settings, "BackwardsText", false);
        game_data_->clip_gui_controls = XmlChildBool(settings, "ClipGUIControls", true);
        game_data_->scale_char_sprite_offsets = XmlChildBool(settings, "ScaleCharacterSpriteOffsets", false);
        game_data_->use_old_voice_clip_naming = XmlChildBool(settings, "UseOldVoiceClipNaming", false);
        game_data_->gui_handle_only_left_mouse = XmlChildBool(settings, "GUIHandleOnlyLeftMouseButton", false);

        // Split resources
        int split_val = XmlChildInt(settings, "SplitResources", 0);
        game_data_->split_resources = (split_val > 0);
        game_data_->split_resource_threshold = (split_val > 0) ? split_val : 1024;

        // Dialog settings
        game_data_->dialog_options_gui = XmlChildInt(settings, "DialogOptionsGUI", 0);
        game_data_->dialog_options_gap = XmlChildInt(settings, "DialogOptionsGap", 0);
        game_data_->dialog_bullet_sprite = XmlChildInt(settings, "DialogOptionsBullet", 0);
        const char* num_dlg_opt = XmlChildText(settings, "NumberDialogOptions", "None");
        if (num_dlg_opt) {
            if (strcmp(num_dlg_opt, "None") == 0) game_data_->number_dialog_options = -1;
            else if (strcmp(num_dlg_opt, "KeyShortcutsOnly") == 0) game_data_->number_dialog_options = 0;
            else if (strcmp(num_dlg_opt, "NoneStated") == 0) game_data_->number_dialog_options = 0;
            else game_data_->number_dialog_options = 1;
        }
        game_data_->dialog_options_backwards = XmlChildBool(settings, "DialogOptionsBackwards", false);
        game_data_->run_game_loops_in_dialog = XmlChildBool(settings, "RunGameLoopsWhileDialogOptionsDisplayed", false);
        game_data_->dialog_say_function = XmlChildText(settings, "DialogScriptSayFunction", "");
        game_data_->dialog_narrate_function = XmlChildText(settings, "DialogScriptNarrateFunction", "");

        // Sound
        game_data_->play_sound_on_score = (XmlChildInt(settings, "PlaySoundOnScore", 0) > 0);
        game_data_->score_sound_clip = XmlChildInt(settings, "PlaySoundOnScore", -1);

        // Text & Speech
        game_data_->always_display_text_as_speech = XmlChildBool(settings, "AlwaysDisplayTextAsSpeech", false);
        game_data_->text_window_gui = XmlChildInt(settings, "TextWindowGUI", 0);
        game_data_->thought_gui = XmlChildInt(settings, "ThoughtGUI", 0);
        game_data_->use_global_speech_anim_delay = XmlChildBool(settings, "UseGlobalSpeechAnimationDelay", false);
        game_data_->global_speech_anim_delay = XmlChildInt(settings, "GlobalSpeechAnimationDelay", 5);

        // Speech style
        const char* speech_str = XmlChildText(settings, "SpeechStyle", "Lucasarts");
        if (speech_str) {
            if (strcmp(speech_str, "Lucasarts") == 0) game_data_->speech_style = kSpeechStyle_LucasArts;
            else if (strcmp(speech_str, "Sierra") == 0) game_data_->speech_style = kSpeechStyle_SierraTransparent;
            else if (strcmp(speech_str, "SierraWithBackground") == 0) game_data_->speech_style = kSpeechStyle_SierraBackground;
            // C# editor WholeScreen=3; kSpeechStyle_QFG4=4 in the engine C++ enum
            else if (strcmp(speech_str, "FullScreen") == 0) game_data_->speech_style = 3;
        }

        // Speech portrait side
        const char* portrait_str = XmlChildText(settings, "SpeechPortraitSide", "Left");
        if (portrait_str) {
            if (strcmp(portrait_str, "Left") == 0) game_data_->speech_portrait_side = PORTRAIT_LEFT;
            else if (strcmp(portrait_str, "Right") == 0) game_data_->speech_portrait_side = PORTRAIT_RIGHT;
            else if (strcmp(portrait_str, "Alternate") == 0) game_data_->speech_portrait_side = PORTRAIT_ALTERNATE;
            else if (strcmp(portrait_str, "XPosition") == 0) game_data_->speech_portrait_side = PORTRAIT_XPOSITION;
        }

        // GUI/Sprite alpha rendering
        const char* gui_alpha_str = XmlChildText(settings, "GUIAlphaStyle", "Proper");
        if (gui_alpha_str) {
            if (strcmp(gui_alpha_str, "Classic") == 0) game_data_->gui_alpha_style = kGuiAlphaRender_Legacy;
            else if (strcmp(gui_alpha_str, "AdditiveOpacity") == 0) game_data_->gui_alpha_style = kGuiAlphaRender_AdditiveAlpha;
            else if (strcmp(gui_alpha_str, "Proper") == 0) game_data_->gui_alpha_style = kGuiAlphaRender_Proper;
        }
        const char* spr_alpha_str = XmlChildText(settings, "SpriteAlphaStyle", "Proper");
        if (spr_alpha_str) {
            if (strcmp(spr_alpha_str, "Classic") == 0) game_data_->sprite_alpha_style = kSpriteAlphaRender_Legacy;
            else if (strcmp(spr_alpha_str, "Proper") == 0) game_data_->sprite_alpha_style = kSpriteAlphaRender_Proper;
        }

        // Skip speech style
        // Skip speech style — values match C# SkipSpeechStyle enum / Engine SkipSpeechStyle
        const char* skip_str = XmlChildText(settings, "SkipSpeech", "MouseOrKeyboardOrTimer");
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
        game_data_->enforce_object_scripting = XmlChildBool(settings, "EnforceObjectBasedScript", true);
        game_data_->enforce_new_strings = XmlChildBool(settings, "EnforceNewStrings", true);
        game_data_->left_to_right_precedence = XmlChildBool(settings, "LeftToRightPrecedence", true);
        game_data_->enforce_new_audio = XmlChildBool(settings, "EnforceNewAudio", true);
        game_data_->use_old_custom_dialog_api = XmlChildBool(settings, "UseOldCustomDialogOptionsAPI", false);
        game_data_->use_old_keyboard_handling = XmlChildBool(settings, "UseOldKeyboardHandling", false);

        // Script API versions — store actual kScriptAPI_* enum values
        // (match Common/ac/gamestructdefines.h ScriptAPIVersion enum)
        const char* api_str = XmlChildText(settings, "ScriptAPIVersion", "Highest");
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
        const char* compat_str = XmlChildText(settings, "ScriptCompatLevel", "v321");
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
        game_data_->turn_before_walking = XmlChildBool(settings, "TurnBeforeWalking", true);
        game_data_->turn_before_facing = XmlChildBool(settings, "TurnBeforeFacing", true);

        // Inventory
        game_data_->inventory_cursors = XmlChildBool(settings, "InventoryCursors", true);
        game_data_->handle_inv_clicks_in_script = XmlChildBool(settings, "HandleInvClicksInScript", true);
        game_data_->display_multiple_inv = XmlChildBool(settings, "DisplayMultipleInventory", false);

        // Saved games
        game_data_->save_game_extension = XmlChildText(settings, "SaveGameFileExtension", "");
        game_data_->save_game_folder = XmlChildText(settings, "SaveGameFolderName", "");

        // Compiler
        game_data_->game_file_name = XmlChildText(settings, "GameFileName", "");

        // Sprite compression
        const char* compress_str = XmlChildText(settings, "CompressSpritesType", "None");
        game_data_->sprite_file_compression = AGS::Common::kSprCompress_None;
        if (compress_str) {
            if (strcmp(compress_str, "RLE") == 0) game_data_->sprite_file_compression = AGS::Common::kSprCompress_RLE;
            else if (strcmp(compress_str, "LZW") == 0) game_data_->sprite_file_compression = AGS::Common::kSprCompress_LZW;
            else if (strcmp(compress_str, "Deflate") == 0) game_data_->sprite_file_compression = AGS::Common::kSprCompress_Deflate;
        }

        game_data_->default_room_mask_resolution = XmlChildInt(settings, "DefaultRoomMaskResolution", 1);

        // Visual
        const char* transition_str = XmlChildText(settings, "RoomTransition", "FadeOutAndIn");
        game_data_->room_transition = kScrTran_Fade;
        if (transition_str) {
            if (strcmp(transition_str, "FadeOutAndIn") == 0) game_data_->room_transition = kScrTran_Fade;
            else if (strcmp(transition_str, "Instant") == 0) game_data_->room_transition = kScrTran_Instant;
            else if (strcmp(transition_str, "Dissolve") == 0) game_data_->room_transition = kScrTran_Dissolve;
            else if (strcmp(transition_str, "BoxOut") == 0) game_data_->room_transition = kScrTran_Boxout;
            else if (strcmp(transition_str, "Crossfade") == 0) game_data_->room_transition = kScrTran_Crossfade;
        }

        const char* render_str = XmlChildText(settings, "RenderAtScreenResolution", "UserDefined");
        game_data_->render_at_screen_resolution = kRenderAtScreenRes_UserDefined;
        if (render_str) {
            if (strcmp(render_str, "UserDefined") == 0) game_data_->render_at_screen_resolution = kRenderAtScreenRes_UserDefined;
            else if (strcmp(render_str, "True") == 0) game_data_->render_at_screen_resolution = kRenderAtScreenRes_Enabled;
            else if (strcmp(render_str, "False") == 0) game_data_->render_at_screen_resolution = kRenderAtScreenRes_Disabled;
        }

        const char* iface_str = XmlChildText(settings, "WhenInterfaceDisabled", "GreyOut");
        game_data_->when_interface_disabled = AGS::Common::kGuiDis_Greyout;
        if (iface_str) {
            if (strcmp(iface_str, "GreyOut") == 0) game_data_->when_interface_disabled = AGS::Common::kGuiDis_Greyout;
            else if (strcmp(iface_str, "GoBlack") == 0) game_data_->when_interface_disabled = AGS::Common::kGuiDis_Blackout;
            else if (strcmp(iface_str, "DisplayNormally") == 0) game_data_->when_interface_disabled = AGS::Common::kGuiDis_Unchanged;
            else if (strcmp(iface_str, "SetGUIDisabled") == 0) game_data_->when_interface_disabled = AGS::Common::kGuiDis_Off;
        }
    }

    // --- Runtime Setup (Default Setup) ---
    const tinyxml2::XMLElement* rt_setup = game_node->FirstChildElement("RuntimeSetup");
    if (rt_setup)
    {
        auto& rs = game_data_->runtime_setup;

        // Graphics
        const char* gfx_drv = XmlChildText(rt_setup, "GraphicsDriver", "D3D9");
        if (gfx_drv) {
            // Engine uses string IDs; C# editor enum: Software=0, D3D9=1, OpenGL=2
            if (strcmp(gfx_drv, "Software") == 0) rs.graphics_driver = 0;
            else if (strcmp(gfx_drv, "D3D9") == 0) rs.graphics_driver = 1;
            else if (strcmp(gfx_drv, "OpenGL") == 0) rs.graphics_driver = 2;
        }
        rs.windowed = XmlChildBool(rt_setup, "Windowed", false);
        rs.fullscreen_desktop = XmlChildBool(rt_setup, "FullscreenDesktop", true);

        const char* fs_scale = XmlChildText(rt_setup, "FullscreenGameScaling", "ProportionalStretch");
        if (fs_scale) {
            // C# editor enum GameScaling: Integer=0, MaxInteger=1, StretchToFit=2, ProportionalStretch=3
            if (strcmp(fs_scale, "None") == 0) rs.fullscreen_scaling = 0;
            else if (strcmp(fs_scale, "ProportionalStretch") == 0) rs.fullscreen_scaling = 1;
            else if (strcmp(fs_scale, "StretchToFit") == 0) rs.fullscreen_scaling = 2;
            else if (strcmp(fs_scale, "Integer") == 0) rs.fullscreen_scaling = 3;
        }

        const char* win_scale = XmlChildText(rt_setup, "GameScaling", "MaxInteger");
        if (win_scale) {
            // C# editor enum GameScaling: Integer=0, MaxInteger=1, StretchToFit=2, ProportionalStretch=3
            if (strcmp(win_scale, "None") == 0) rs.windowed_scaling = 0;
            else if (strcmp(win_scale, "MaxRound") == 0) rs.windowed_scaling = 1;
            else if (strcmp(win_scale, "StretchToFit") == 0) rs.windowed_scaling = 2;
            else if (strcmp(win_scale, "MaxInteger") == 0) rs.windowed_scaling = 3;
        }
        rs.scaling_multiplier = XmlChildInt(rt_setup, "GameScalingMultiplier", 1);

        const char* gfx_filter = XmlChildText(rt_setup, "GraphicsFilter", "stdscale");
        if (gfx_filter) {
            // Engine uses string filter IDs; C# editor stores as index
            if (strcmp(gfx_filter, "stdscale") == 0) rs.graphics_filter = 0;
            else if (strcmp(gfx_filter, "linear") == 0) rs.graphics_filter = 1;
        }

        rs.vsync = XmlChildBool(rt_setup, "VSync", false);
        rs.aa_scaled_sprites = XmlChildBool(rt_setup, "AAScaledSprites", false);
        rs.render_at_screen_res = XmlChildBool(rt_setup, "RenderAtScreenResolution", false);

        const char* rot = XmlChildText(rt_setup, "Rotation", "Unlocked");
        if (rot) {
            // Engine ScreenRotation enum (Engine/ac/gamesetup.h)
            if (strcmp(rot, "Unlocked") == 0) rs.rotation = 0;  // kScreenRotation_Unlocked
            else if (strcmp(rot, "Portrait") == 0) rs.rotation = 1;  // kScreenRotation_Portrait
            else if (strcmp(rot, "Landscape") == 0) rs.rotation = 2; // kScreenRotation_Landscape
        }

        // Audio
        rs.digital_sound = XmlChildInt(rt_setup, "DigitalSound", -1);
        rs.use_voice_pack = XmlChildBool(rt_setup, "UseVoicePack", true);

        // Gameplay
        rs.translation = XmlChildText(rt_setup, "Translation", "");

        // Mouse
        rs.auto_lock_mouse = XmlChildBool(rt_setup, "AutoLockMouse", false);
        rs.mouse_speed = (float)atof(XmlChildText(rt_setup, "MouseSpeed", "1.0"));

        // Touch
        rs.touch_emulation = XmlChildInt(rt_setup, "TouchToMouseEmulation", 1);
        rs.touch_motion = XmlChildInt(rt_setup, "TouchToMouseMotionMode", 0);

        // Misc
        rs.show_fps = XmlChildBool(rt_setup, "ShowFPS", false);

        // Performance
        rs.sprite_cache_size = XmlChildInt(rt_setup, "SpriteCacheSize", 128);
        rs.texture_cache_size = XmlChildInt(rt_setup, "TextureCacheSize", 128);
        rs.sound_cache_size = XmlChildInt(rt_setup, "SoundCacheSize", 32);
        rs.compress_saves = XmlChildBool(rt_setup, "CompressSaves", true);

        // Environment
        rs.use_custom_save_path = XmlChildBool(rt_setup, "UseCustomSavePath", false);
        rs.custom_save_path = XmlChildText(rt_setup, "CustomSavePath", "");
        rs.use_custom_appdata_path = XmlChildBool(rt_setup, "UseCustomAppDataPath", false);
        rs.custom_appdata_path = XmlChildText(rt_setup, "CustomAppDataPath", "");

        // Setup appearance
        rs.title_text = XmlChildText(rt_setup, "TitleText", "");
    }

    // --- Rooms ---
    const tinyxml2::XMLElement* rooms_wrapper = game_node->FirstChildElement("Rooms");
    if (rooms_wrapper)
    {
        const tinyxml2::XMLElement* room_folder = rooms_wrapper->FirstChildElement("UnloadedRoomFolder");
        if (room_folder)
        {
            CollectFolderItems(room_folder, "UnloadedRooms", "UnloadedRoom",
                [this](const tinyxml2::XMLElement* room_elem) {
                    GameData::RoomInfo room;
                    room.number = XmlChildInt(room_elem, "Number", -1);
                    room.description = XmlChildText(room_elem, "Description", "");
                    if (room.number >= 0)
                        game_data_->rooms.push_back(room);
                });
        }

        // Sort rooms by number
        std::sort(game_data_->rooms.begin(), game_data_->rooms.end(),
            [](const GameData::RoomInfo& a, const GameData::RoomInfo& b) {
                return a.number < b.number;
            });
    }

    // --- Characters ---
    const tinyxml2::XMLElement* chars_wrapper = game_node->FirstChildElement("Characters");
    if (chars_wrapper)
    {
        const tinyxml2::XMLElement* char_folder = chars_wrapper->FirstChildElement("CharacterFolder");
        if (char_folder)
        {
            game_data_->character_folders = FolderInfo{};
            CollectFolderItemsWithTree(char_folder, "Characters", "Character",
                game_data_->character_folders,
                [this](const tinyxml2::XMLElement* ch_elem) {
                    GameData::CharacterInfo ch;
                    ch.id = XmlChildInt(ch_elem, "ID", 0);
                    ch.script_name = XmlChildText(ch_elem, "ScriptName", "");
                    ch.real_name = XmlChildText(ch_elem, "RealName", "");
                    ch.room = XmlChildInt(ch_elem, "StartingRoom", 0);
                    ch.x = XmlChildInt(ch_elem, "StartX", 0);
                    ch.y = XmlChildInt(ch_elem, "StartY", 0);
                    ch.normal_view = XmlChildInt(ch_elem, "NormalView", -1);
                    ch.speech_view = XmlChildInt(ch_elem, "SpeechView", -1);
                    ch.idle_view = XmlChildInt(ch_elem, "IdleView", -1);
                    ch.thinking_view = XmlChildInt(ch_elem, "ThinkingView", -1);
                    ch.blinking_view = XmlChildInt(ch_elem, "BlinkingView", -1);
                    // Movement properties
                    ch.movement_speed = XmlChildInt(ch_elem, "MovementSpeed", 3);
                    ch.movement_speed_x = XmlChildInt(ch_elem, "MovementSpeedX", 3);
                    ch.movement_speed_y = XmlChildInt(ch_elem, "MovementSpeedY", 3);
                    ch.uniform_movement_speed = XmlChildBool(ch_elem, "UniformMovementSpeed", true);
                    ch.animation_delay = XmlChildInt(ch_elem, "AnimationDelay", 0);
                    ch.solid = XmlChildBool(ch_elem, "Solid", true);
                    ch.turn_before_walking = XmlChildBool(ch_elem, "TurnBeforeWalking", true);
                    ch.turn_when_facing = XmlChildBool(ch_elem, "TurnWhenFacing", true);
                    ch.diagonal_loops = XmlChildBool(ch_elem, "DiagonalLoops", true);
                    ch.adjust_speed_with_scaling = XmlChildBool(ch_elem, "AdjustSpeedWithScaling", true);
                    ch.movement_linked_to_animation = XmlChildBool(ch_elem, "MovementLinkedToAnimation", true);
                    // Appearance properties
                    ch.baseline = XmlChildInt(ch_elem, "Baseline", 0);
                    ch.use_room_area_scaling = XmlChildBool(ch_elem, "UseRoomAreaScaling", true);
                    ch.clickable = XmlChildBool(ch_elem, "Clickable", true);
                    ch.transparency = XmlChildInt(ch_elem, "Transparency", 0);
                    ch.speech_animation_delay = XmlChildInt(ch_elem, "SpeechAnimationDelay", 0);
                    ch.speech_color = XmlChildInt(ch_elem, "SpeechColor", 0);
                    ch.use_room_area_lighting = XmlChildBool(ch_elem, "UseRoomAreaLighting", true);
                    ch.scale_volume = XmlChildBool(ch_elem, "AdjustVolumeWithScaling", false);
                    ch.idle_delay = XmlChildInt(ch_elem, "IdleDelay", 20);
                    ch.idle_anim_speed = XmlChildInt(ch_elem, "IdleAnimationDelay", 5);
                    // Event handlers
                    ch.interactions = LoadInteractions(ch_elem,
                        (int)InteractionSchemas::Character().size());
                    // Custom properties
                    const tinyxml2::XMLElement* props_elem = ch_elem->FirstChildElement("Properties");
                    if (props_elem)
                    {
                        for (const tinyxml2::XMLElement* cp = props_elem->FirstChildElement("CustomProperty");
                             cp; cp = cp->NextSiblingElement("CustomProperty"))
                        {
                            std::string pname = XmlChildText(cp, "Name", "");
                            std::string pval = XmlChildText(cp, "Value", "");
                            if (!pname.empty())
                                ch.custom_properties[pname] = pval;
                        }
                    }
                    game_data_->characters.push_back(ch);
                },
                [](const tinyxml2::XMLElement* e) { return XmlChildInt(e, "ID", 0); });
        }
    }

    // --- Player Character ---
    // <PlayerCharacter> is a direct child of <Game>, containing the character ID
    game_data_->player_character_id = XmlChildInt(game_node, "PlayerCharacter", 0);
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

    const tinyxml2::XMLElement* guis_wrapper = game_node->FirstChildElement("GUIs");
    if (guis_wrapper)
    {
        // Single-GUI element processor (shared between collect_guis and collect_gui_folder)
        auto collect_single_gui = [this, &ParseTextAlignment, &ParseClickAction](const tinyxml2::XMLElement* gui) {
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
                    g.id = XmlChildInt(tw_props, "ID", 0);
                    g.name = XmlChildText(tw_props, "Name", "");
                    g.border_color = XmlChildInt(tw_props, "TextColor", 1);
                    g.bg_color = XmlChildInt(tw_props, "BackgroundColor", 0);
                    g.bg_image = XmlChildInt(tw_props, "BackgroundImage", 0);
                    g.padding = XmlChildInt(tw_props, "Padding", 3);
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
                    g.id = XmlChildInt(props, "ID", 0);
                    g.name = XmlChildText(props, "Name", "");
                    g.x = XmlChildInt(props, "Left", 0);
                    g.y = XmlChildInt(props, "Top", 0);
                    g.width = XmlChildInt(props, "Width", 100);
                    g.height = XmlChildInt(props, "Height", 100);
                    g.bg_color = XmlChildInt(props, "BackgroundColor", 0);
                    g.border_color = XmlChildInt(props, "BorderColor", 15);
                    g.bg_image = XmlChildInt(props, "BackgroundImage", 0);
                    g.transparency = XmlChildInt(props, "Transparency", 0);
                    g.z_order = XmlChildInt(props, "ZOrder", 0);
                    g.clickable = XmlChildBool(props, "Clickable", true);
                    g.on_click = XmlChildText(props, "OnClick", "");
                    g.script_module = XmlChildText(props, "ScriptModule", "GlobalScript.asc");
                    g.popup_at_mouse_y = XmlChildInt(props, "PopupYPos", -1);

                    // Try new format <PopupStyle> + <Visible> first
                    const char* popup_str = XmlChildText(props, "PopupStyle", nullptr);
                    if (popup_str) {
                        if (strcmp(popup_str, "Normal") == 0) g.popup_style = 0;
                        else if (strcmp(popup_str, "MouseYPos") == 0) g.popup_style = 1;
                        else if (strcmp(popup_str, "PopupModal") == 0) g.popup_style = 2;
                        else if (strcmp(popup_str, "Persistent") == 0) g.popup_style = 3;
                        else g.popup_style = 0;
                        g.visible = XmlChildBool(props, "Visible", true);
                    } else {
                        // Fall back to old format <Visibility>
                        const char* vis_str = XmlChildText(props, "Visibility", "Normal");
                        if (strcmp(vis_str, "Normal") == 0) { g.popup_style = 0; g.visible = true; }
                        else if (strcmp(vis_str, "MouseYPos") == 0 || strcmp(vis_str, "PopupYPos") == 0) { g.popup_style = 1; g.visible = false; }
                        else if (strcmp(vis_str, "ScriptOnly") == 0) { g.popup_style = 2; g.visible = false; }
                        else if (strcmp(vis_str, "PopupModal") == 0) { g.popup_style = 2; g.visible = false; }
                        else if (strcmp(vis_str, "Persistent") == 0) { g.popup_style = 3; g.visible = true; }
                        else { g.popup_style = 0; g.visible = true; }
                    }
                } else {
                    // Fallback: read from outer element
                    g.id = XmlChildInt(gui, "ID", 0);
                    g.name = XmlChildText(gui, "Name", "");
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
                        ci.id = XmlChildInt(ctrl, "ID", 0);
                        ci.name = XmlChildText(ctrl, "Name", "");
                        ci.x = XmlChildInt(ctrl, "Left", 0);
                        ci.y = XmlChildInt(ctrl, "Top", 0);
                        ci.width = XmlChildInt(ctrl, "Width", 60);
                        ci.height = XmlChildInt(ctrl, "Height", 20);
                        ci.text = XmlChildText(ctrl, "Text", "");
                        ci.image = XmlChildInt(ctrl, "Image", -1);
                        ci.font = XmlChildInt(ctrl, "Font", 0);
                        // Common control flags (defaults match C# GUIControl defaults)
                        ci.enabled = XmlChildBool(ctrl, "Enabled", true);
                        ci.ctrl_visible = XmlChildBool(ctrl, "Visible", true);
                        ci.ctrl_clickable = XmlChildBool(ctrl, "Clickable", true);
                        ci.translated = XmlChildBool(ctrl, "Translated", true);
                        ci.show_border = XmlChildBool(ctrl, "ShowBorder", false);
                        // SolidBackground defaults depend on type (set after type detection below)
                        // Read event handler and type-specific properties
                        if (ci.type_tag == "GUIButton" || ci.type_tag == "GUITextWindowEdge") {
                            ci.event_handler = XmlChildText(ctrl, "OnClick", "");
                            ci.mouseover_image = XmlChildInt(ctrl, "MouseoverImage", 0);
                            ci.pushed_image = XmlChildInt(ctrl, "PushedImage", 0);
                            ci.text_color = XmlChildInt(ctrl, "TextColor", 0);
                            ci.click_action = ParseClickAction(XmlChildText(ctrl, "ClickAction", "None"));
                            ci.new_mode_number = XmlChildInt(ctrl, "NewModeNumber", 0);
                            ci.text_alignment = ParseTextAlignment(XmlChildText(ctrl, "TextAlignment", "TopMiddle"));
                            ci.clip_image = XmlChildBool(ctrl, "ClipImage", false);
                            ci.wrap_text = XmlChildBool(ctrl, "WrapText", false);
                            ci.solid_background = XmlChildBool(ctrl, "SolidBackground", true);
                        } else if (ci.type_tag == "GUILabel") {
                            ci.text_color = XmlChildInt(ctrl, "TextColor", 0);
                            ci.text_alignment = ParseTextAlignment(XmlChildText(ctrl, "TextAlignment", "TopMiddle"));
                            ci.solid_background = XmlChildBool(ctrl, "SolidBackground", false);
                        } else if (ci.type_tag == "GUISlider") {
                            ci.event_handler = XmlChildText(ctrl, "OnChange", "");
                            ci.min_value = XmlChildInt(ctrl, "MinValue", 0);
                            ci.max_value = XmlChildInt(ctrl, "MaxValue", 10);
                            ci.slider_value = XmlChildInt(ctrl, "Value", 0);
                            ci.handle_image = XmlChildInt(ctrl, "HandleImage", 0);
                            ci.handle_offset = XmlChildInt(ctrl, "HandleOffset", 0);
                            ci.bg_image = XmlChildInt(ctrl, "BackgroundImage", 0);
                            ci.solid_background = XmlChildBool(ctrl, "SolidBackground", true);
                        } else if (ci.type_tag == "GUIInventory") {
                            ci.inv_char_id = XmlChildInt(ctrl, "CharacterID", -1);
                            ci.item_width = XmlChildInt(ctrl, "ItemWidth", 40);
                            ci.item_height = XmlChildInt(ctrl, "ItemHeight", 22);
                            ci.solid_background = XmlChildBool(ctrl, "SolidBackground", false);
                        } else if (ci.type_tag == "GUITextBox") {
                            ci.event_handler = XmlChildText(ctrl, "OnActivate", "");
                            ci.text_color = XmlChildInt(ctrl, "TextColor", 0);
                            ci.solid_background = XmlChildBool(ctrl, "SolidBackground", false);
                        } else if (ci.type_tag == "GUIListBox") {
                            ci.event_handler = XmlChildText(ctrl, "OnSelectionChanged", "");
                            ci.text_color = XmlChildInt(ctrl, "TextColor", 0);
                            ci.selected_text_color = XmlChildInt(ctrl, "SelectedTextColor", 0);
                            ci.selected_bg_color = XmlChildInt(ctrl, "SelectedBackgroundColor", 0);
                            ci.text_alignment = ParseTextAlignment(XmlChildText(ctrl, "TextAlignment", "Left"));
                            ci.show_border = XmlChildBool(ctrl, "ShowBorder", true);
                            ci.show_scroll_arrows = XmlChildBool(ctrl, "ShowScrollArrows", true);
                            ci.solid_background = XmlChildBool(ctrl, "SolidBackground", false);
                        } else {
                            ci.solid_background = XmlChildBool(ctrl, "SolidBackground", false);
                        }
                        g.controls.push_back(ci);
                    }
                }
                game_data_->guis.push_back(g);
        };

        // Container wrapper that iterates children and calls collect_single_gui
        auto collect_guis = [&collect_single_gui](const tinyxml2::XMLElement* items_elem) {
            if (!items_elem) return;
            for (const tinyxml2::XMLElement* gui = items_elem->FirstChildElement();
                 gui; gui = gui->NextSiblingElement())
            {
                collect_single_gui(gui);
            }
        };

        // Recursive GUI folder collection with tree building
        std::function<void(const tinyxml2::XMLElement*, FolderInfo&)> collect_gui_folder;
        collect_gui_folder = [&](const tinyxml2::XMLElement* folder_elem, FolderInfo& folder_out) {
            const char* name = folder_elem->Attribute("Name");
            if (name) folder_out.name = name;

            // Process subfolders
            const tinyxml2::XMLElement* subfolders = folder_elem->FirstChildElement("SubFolders");
            if (subfolders)
            {
                for (const tinyxml2::XMLElement* sub = subfolders->FirstChildElement("GUIFolder");
                     sub; sub = sub->NextSiblingElement("GUIFolder"))
                {
                    FolderInfo child;
                    collect_gui_folder(sub, child);
                    folder_out.subfolders.push_back(std::move(child));
                }
            }

            // Collect items at this level
            const tinyxml2::XMLElement* items = folder_elem->FirstChildElement("GUIs");
            if (items)
            {
                for (const tinyxml2::XMLElement* gui = items->FirstChildElement();
                     gui; gui = gui->NextSiblingElement())
                {
                    const char* tag = gui->Name();
                    if (strcmp(tag, "GUIMain") != 0 && strcmp(tag, "GUITextWindow") != 0)
                        continue;
                    // Extract ID for folder tree
                    const tinyxml2::XMLElement* normal_props = gui->FirstChildElement("NormalGUI");
                    const tinyxml2::XMLElement* tw_props = gui->FirstChildElement("TextWindowGUI");
                    int gui_id = 0;
                    if (tw_props) gui_id = XmlChildInt(tw_props, "ID", 0);
                    else if (normal_props) gui_id = XmlChildInt(normal_props, "ID", 0);
                    else gui_id = XmlChildInt(gui, "ID", 0);
                    folder_out.item_ids.push_back(gui_id);
                    collect_single_gui(gui);
                }
            }
        };

        const tinyxml2::XMLElement* gui_folder = guis_wrapper->FirstChildElement("GUIFolder");
        if (gui_folder)
        {
            game_data_->gui_folders = FolderInfo{};
            collect_gui_folder(gui_folder, game_data_->gui_folders);
        }
        // Fallback: also collect GUIs directly under wrapper
        // (older AGF format places items outside the folder structure)
        collect_guis(guis_wrapper);
    }

    // --- Inventory Items ---
    const tinyxml2::XMLElement* inv_wrapper = game_node->FirstChildElement("InventoryItems");
    if (inv_wrapper)
    {
        auto collect_inv = [this](const tinyxml2::XMLElement* inv_elem) {
            GameData::InventoryItemInfo inv;
            inv.id = XmlChildInt(inv_elem, "ID", 0);
            inv.script_name = XmlChildText(inv_elem, "Name", "");
            inv.description = XmlChildText(inv_elem, "Description", "");
            inv.image = XmlChildInt(inv_elem, "Image", 0);
            inv.cursor_image = XmlChildInt(inv_elem, "CursorImage", 0);
            inv.start_with = XmlChildBool(inv_elem, "PlayerStartsWithItem", false);
            // Event handlers
            inv.interactions = LoadInteractions(inv_elem,
                (int)InteractionSchemas::InventoryItem().size());
            // Custom properties
            const tinyxml2::XMLElement* props_elem = inv_elem->FirstChildElement("Properties");
            if (props_elem)
            {
                for (const tinyxml2::XMLElement* cp = props_elem->FirstChildElement("CustomProperty");
                     cp; cp = cp->NextSiblingElement("CustomProperty"))
                {
                    std::string pname = XmlChildText(cp, "Name", "");
                    std::string pval = XmlChildText(cp, "Value", "");
                    if (!pname.empty())
                        inv.custom_properties[pname] = pval;
                }
            }
            game_data_->inventory_items.push_back(inv);
        };

        const tinyxml2::XMLElement* inv_folder = inv_wrapper->FirstChildElement("InventoryItemFolder");
        if (inv_folder)
        {
            game_data_->inventory_folders = FolderInfo{};
            CollectFolderItemsWithTree(inv_folder, "InventoryItems", "InventoryItem",
                game_data_->inventory_folders, collect_inv,
                [](const tinyxml2::XMLElement* e) { return XmlChildInt(e, "ID", 0); });
        }
        // Fallback: also collect items directly under wrapper
        // (older AGF format places items outside the folder structure)
        for (const tinyxml2::XMLElement* inv_elem = inv_wrapper->FirstChildElement("InventoryItem");
             inv_elem; inv_elem = inv_elem->NextSiblingElement("InventoryItem"))
        {
            collect_inv(inv_elem);
        }
    }

    // --- Dialogs ---
    const tinyxml2::XMLElement* dlg_wrapper = game_node->FirstChildElement("Dialogs");
    if (dlg_wrapper)
    {
        const tinyxml2::XMLElement* dlg_folder = dlg_wrapper->FirstChildElement("DialogFolder");
        if (dlg_folder)
        {
            game_data_->dialog_folders = FolderInfo{};
            CollectFolderItemsWithTree(dlg_folder, "Dialogs", "Dialog",
                game_data_->dialog_folders,
                [this](const tinyxml2::XMLElement* dlg_elem) {
                    GameData::DialogInfo dlg;
                    dlg.id = XmlChildInt(dlg_elem, "ID", 0);
                    dlg.name = XmlChildText(dlg_elem, "Name", "");
                    dlg.script_name = XmlChildText(dlg_elem, "ScriptName", "");
                    // Fallback: older AGF format uses Name as script name
                    if (dlg.script_name.empty())
                        dlg.script_name = dlg.name;
                    dlg.script = XmlChildText(dlg_elem, "Script", "");
                    dlg.show_text_parser = XmlChildBool(dlg_elem, "ShowTextParser", false);
                    dlg.option_count = 0;
                    const tinyxml2::XMLElement* opts = dlg_elem->FirstChildElement("DialogOptions");
                    if (opts)
                    {
                        for (const tinyxml2::XMLElement* opt = opts->FirstChildElement("DialogOption");
                             opt; opt = opt->NextSiblingElement("DialogOption"))
                        {
                            GameData::DialogInfo::DialogOption dopt;
                            dopt.text = XmlChildText(opt, "Text", "");
                            dopt.show = XmlChildBool(opt, "Show", true);
                            dopt.say = XmlChildBool(opt, "Say", true);
                            dlg.options.push_back(dopt);
                        }
                    }
                    dlg.option_count = (int)dlg.options.size();
                    game_data_->dialogs.push_back(dlg);
                },
                [](const tinyxml2::XMLElement* e) { return XmlChildInt(e, "ID", 0); });
        }
    }

    // --- Cursors (flat list) ---
    const tinyxml2::XMLElement* cursors_node = game_node->FirstChildElement("Cursors");
    if (cursors_node)
    {
        for (const tinyxml2::XMLElement* cur = cursors_node->FirstChildElement("MouseCursor");
             cur; cur = cur->NextSiblingElement("MouseCursor"))
        {
            GameData::CursorInfo c;
            c.id = XmlChildInt(cur, "ID", 0);
            c.name = XmlChildText(cur, "Name", "");
            c.image = XmlChildInt(cur, "Image", 0);
            c.hotspot_x = XmlChildInt(cur, "HotspotX", 0);
            c.hotspot_y = XmlChildInt(cur, "HotspotY", 0);
            c.animate = XmlChildBool(cur, "Animate", false);
            c.view = XmlChildInt(cur, "View", 0);
            c.process_click = XmlChildBool(cur, "StandardMode", false);
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
            f.id = XmlChildInt(fnt, "ID", 0);
            f.name = XmlChildText(fnt, "Name", "");
            f.size = XmlChildInt(fnt, "PointSize", 10);
            f.source_file = XmlChildText(fnt, "SourceFilename", "");
            f.line_spacing = XmlChildInt(fnt, "LineSpacing", 0);
            f.size_multiplier = XmlChildInt(fnt, "SizeMultiplier", 1);
            const char* outline = XmlChildText(fnt, "OutlineStyle", "None");
            if (strcmp(outline, "None") == 0) f.outline_type = 0;
            else if (strcmp(outline, "Automatic") == 0) f.outline_type = 1;
            else if (strcmp(outline, "UseOutlineFont") == 0) f.outline_type = 2;
            f.outline_font = XmlChildInt(fnt, "OutlineFont", -1);
            game_data_->fonts.push_back(f);
        }
    }

    // --- Views ---
    const tinyxml2::XMLElement* views_wrapper = game_node->FirstChildElement("Views");
    if (views_wrapper)
    {
        const tinyxml2::XMLElement* view_folder = views_wrapper->FirstChildElement("ViewFolder");
        if (view_folder)
        {
            game_data_->view_folders = FolderInfo{};
            CollectFolderItemsWithTree(view_folder, "Views", "View",
                game_data_->view_folders,
                [this](const tinyxml2::XMLElement* view_elem) {
                    GameData::ViewInfo v;
                    v.id = XmlChildInt(view_elem, "ID", 0);
                    v.name = XmlChildText(view_elem, "Name", "");
                    v.loop_count = 0;
                    const tinyxml2::XMLElement* loops = view_elem->FirstChildElement("Loops");
                    if (loops)
                    {
                        for (const tinyxml2::XMLElement* loop_elem = loops->FirstChildElement("Loop");
                             loop_elem; loop_elem = loop_elem->NextSiblingElement("Loop"))
                        {
                            GameData::LoopData loop;
                            loop.run_next_loop = XmlChildBool(loop_elem, "RunNextLoop", false);
                            const tinyxml2::XMLElement* frames = loop_elem->FirstChildElement("Frames");
                            if (frames)
                            {
                                for (const tinyxml2::XMLElement* fr = frames->FirstChildElement("ViewFrame");
                                     fr; fr = fr->NextSiblingElement("ViewFrame"))
                                {
                                    GameData::FrameData frame;
                                    frame.sprite_id = XmlChildInt(fr, "Image", 0);
                                    frame.x_offset = XmlChildInt(fr, "OffsetX", 0);
                                    frame.y_offset = XmlChildInt(fr, "OffsetY", 0);
                                    frame.delay = XmlChildInt(fr, "Delay", 0);
                                    frame.flipped = XmlChildBool(fr, "Flipped", false);
                                    frame.sound = XmlChildInt(fr, "Sound", -1);
                                    loop.frames.push_back(frame);
                                }
                            }
                            v.loops.push_back(loop);
                            v.loop_count++;
                        }
                    }
                    game_data_->views.push_back(v);
                },
                [](const tinyxml2::XMLElement* e) { return XmlChildInt(e, "ID", 0); });
        }
    }

    // --- Scripts ---
    const tinyxml2::XMLElement* scripts_wrapper = game_node->FirstChildElement("Scripts");
    if (scripts_wrapper)
    {
        const tinyxml2::XMLElement* script_folder = scripts_wrapper->FirstChildElement("ScriptFolder");
        if (script_folder)
        {
            CollectFolderItems(script_folder, "ScriptAndHeaders", "ScriptAndHeader",
                [this](const tinyxml2::XMLElement* sah_elem) {
                    GameData::ScriptModule mod;
                    const tinyxml2::XMLElement* hdr_wrap = sah_elem->FirstChildElement("ScriptAndHeader_Header");
                    if (hdr_wrap) {
                        const tinyxml2::XMLElement* hdr_script = hdr_wrap->FirstChildElement("Script");
                        if (hdr_script) {
                            mod.header_file = XmlChildText(hdr_script, "FileName", "");
                            mod.name = XmlChildText(hdr_script, "Name", "");
                        }
                    }
                    const tinyxml2::XMLElement* scr_wrap = sah_elem->FirstChildElement("ScriptAndHeader_Script");
                    if (scr_wrap) {
                        const tinyxml2::XMLElement* scr_script = scr_wrap->FirstChildElement("Script");
                        if (scr_script) {
                            mod.script_file = XmlChildText(scr_script, "FileName", "");
                            if (mod.name.empty())
                                mod.name = XmlChildText(scr_script, "Name", "");
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
        }
    }

    // --- Audio Clips ---
    const tinyxml2::XMLElement* audio_wrapper = game_node->FirstChildElement("AudioClips");
    if (audio_wrapper)
    {
        const tinyxml2::XMLElement* audio_folder = audio_wrapper->FirstChildElement("AudioClipFolder");
        if (audio_folder)
        {
            game_data_->audio_clip_folders = FolderInfo{};
            CollectFolderItemsWithTree(audio_folder, "AudioClips", "AudioClip",
                game_data_->audio_clip_folders,
                [this](const tinyxml2::XMLElement* clip_elem) {
                    GameData::AudioClipInfo clip;
                    clip.id = XmlChildInt(clip_elem, "ID", 0);
                    clip.name = XmlChildText(clip_elem, "ScriptName", "");
                    if (clip.name.empty())
                        clip.name = XmlChildText(clip_elem, "Name", "");
                    clip.filename = NormalizePath(XmlChildText(clip_elem, "FileName", ""));
                    clip.source_filename = NormalizePath(XmlChildText(clip_elem, "SourceFileName", ""));
                    clip.type = XmlChildInt(clip_elem, "Type", 0);
                    clip.default_volume = XmlChildInt(clip_elem, "DefaultVolume", -1);
                    clip.default_priority = XmlChildInt(clip_elem, "DefaultPriority", 0);
                    clip.default_repeat = XmlChildInt(clip_elem, "DefaultRepeat", 0);
                    clip.bundling_type = XmlChildInt(clip_elem, "BundlingType", 0);
                    clip.file_type = XmlChildInt(clip_elem, "FileType", 0);
                    clip.file_last_modified = XmlChildText(clip_elem, "FileLastModifiedDate", "");
                    game_data_->audio_clips.push_back(clip);
                },
                [](const tinyxml2::XMLElement* e) { return XmlChildInt(e, "ID", 0); });
        }
    }

    // --- Audio Clip Types ---
    const tinyxml2::XMLElement* audio_types_node = game_node->FirstChildElement("AudioClipTypes");
    if (audio_types_node)
    {
        for (const tinyxml2::XMLElement* ct = audio_types_node->FirstChildElement("AudioClipType");
             ct; ct = ct->NextSiblingElement("AudioClipType"))
        {
            GameData::AudioClipTypeInfo ctype;
            ctype.id = XmlChildInt(ct, "TypeID", 0);
            ctype.name = XmlChildText(ct, "Name", "");
            ctype.max_channels = XmlChildInt(ct, "MaxChannels", 0);
            ctype.volume_reduction_while_speech = XmlChildInt(ct, "VolumeReductionWhileSpeechPlaying", 0);
            std::string crossfade = XmlChildText(ct, "CrossfadeClips", "No");
            if (crossfade == "Slow") ctype.crossfade_speed = 1;
            else if (crossfade == "Fast") ctype.crossfade_speed = 2;
            else ctype.crossfade_speed = 0;
            ctype.backwards_compat_type = XmlChildBool(ct, "BackwardsCompatibilityType", false);
            game_data_->audio_clip_types.push_back(ctype);
        }
    }

    // --- Global Variables ---
    const tinyxml2::XMLElement* gv_node = game_node->FirstChildElement("GlobalVariables");
    if (gv_node)
    {
        // Variables may be directly under GlobalVariables or inside a <Variables> child
        const tinyxml2::XMLElement* vars_parent = gv_node->FirstChildElement("Variables");
        if (!vars_parent)
            vars_parent = gv_node;
        for (const tinyxml2::XMLElement* gv_elem = vars_parent->FirstChildElement("GlobalVariable");
             gv_elem; gv_elem = gv_elem->NextSiblingElement("GlobalVariable"))
        {
            GameData::GlobalVariableInfo gv;
            gv.name = XmlChildText(gv_elem, "Name", "");
            // Type is stored as text in AGF ("int", "String", "float", "bool", or managed type)
            gv.type_name = XmlChildText(gv_elem, "Type", "int");
            gv.default_value = XmlChildText(gv_elem, "DefaultValue", "");
            gv.array_type = XmlChildInt(gv_elem, "ArrayType", 0);
            gv.array_size = XmlChildInt(gv_elem, "ArraySize", 0);
            if (!gv.name.empty())
                game_data_->global_variables.push_back(gv);
        }
    }

    // --- Lip Sync ---
    const tinyxml2::XMLElement* ls_node = game_node->FirstChildElement("LipSync");
    if (ls_node)
    {
        std::string type_str = XmlChildText(ls_node, "Type", "None");
        if (type_str == "Text")
            game_data_->lip_sync_type = 1;
        else if (type_str == "PamelaVoiceFiles")
            game_data_->lip_sync_type = 2;
        else
            game_data_->lip_sync_type = 0;

        game_data_->default_lipsync_frame = XmlChildInt(ls_node, "DefaultFrame", 0);

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
            prop.name = XmlChildText(item, "Name", "");
            prop.description = XmlChildText(item, "Description", "");
            prop.default_value = XmlChildText(item, "DefaultValue", "");
            // Type: "Boolean"->0, "Number"->1, "Text"->2
            const char* type_str = XmlChildText(item, "Type", "Boolean");
            if (strcmp(type_str, "Number") == 0)
                prop.type = 1;
            else if (strcmp(type_str, "Text") == 0 || strcmp(type_str, "String") == 0)
                prop.type = 2;
            else
                prop.type = 0; // Boolean
            prop.applies_to_characters = XmlChildBool(item, "AppliesToCharacters", false);
            prop.applies_to_hotspots = XmlChildBool(item, "AppliesToHotspots", false);
            prop.applies_to_objects = XmlChildBool(item, "AppliesToObjects", false);
            prop.applies_to_rooms = XmlChildBool(item, "AppliesToRooms", false);
            prop.applies_to_inv_items = XmlChildBool(item, "AppliesToInvItems", false);
            prop.translated = XmlChildBool(item, "Translated", false);
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
            entry.r = XmlChildInt(pe, "Red", 0);
            entry.g = XmlChildInt(pe, "Green", 0);
            entry.b = XmlChildInt(pe, "Blue", 0);
            const char* usage = XmlChildText(pe, "ColourType", "Background");
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
        auto parse_sprite = [&](const tinyxml2::XMLElement* spr_elem) {
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
                info.source_file = NormalizePath(XmlChildText(src, "FileName", ""));
                info.offset_x = XmlChildInt(src, "OffsetX", 0);
                info.offset_y = XmlChildInt(src, "OffsetY", 0);
                info.import_width = XmlChildInt(src, "ImportWidth", 0);
                info.import_height = XmlChildInt(src, "ImportHeight", 0);
                info.import_as_tile = XmlChildBool(src, "ImportAsTile", false);
                info.frame = XmlChildInt(src, "Frame", 0);
                info.remap_to_game_palette = XmlChildBool(src, "RemapToGamePalette", false);
                info.remap_to_room_palette = XmlChildBool(src, "RemapToRoomPalette", false);
                info.import_alpha_channel = XmlChildBool(src, "ImportAlphaChannel", true);
                info.transparent_color_index = XmlChildInt(src, "TransparentColorIndex", 0);

                // ImportMethod -> SpriteImportTransparency
                const char* method = XmlChildText(src, "ImportMethod", "LeaveAsIs");
                if (strcmp(method, "PaletteIndex0") == 0)
                    info.transparency = GameData::SpriteImportTransparency::PaletteIndex0;
                else if (strcmp(method, "TopLeft") == 0)
                    info.transparency = GameData::SpriteImportTransparency::TopLeft;
                else if (strcmp(method, "BottomLeft") == 0)
                    info.transparency = GameData::SpriteImportTransparency::BottomLeft;
                else if (strcmp(method, "TopRight") == 0)
                    info.transparency = GameData::SpriteImportTransparency::TopRight;
                else if (strcmp(method, "BottomRight") == 0)
                    info.transparency = GameData::SpriteImportTransparency::BottomRight;
                else if (strcmp(method, "NoTransparency") == 0)
                    info.transparency = GameData::SpriteImportTransparency::NoTransparency;
                else if (strcmp(method, "PaletteIndex") == 0)
                    info.transparency = GameData::SpriteImportTransparency::PaletteIndex;
                else
                    info.transparency = GameData::SpriteImportTransparency::LeaveAsIs;

                source_map[slot] = std::move(info);
            }
        };

        // Recursive lambda to parse sprite folder tree
        std::function<void(const tinyxml2::XMLElement*, GameData::SpriteFolderInfo&)>
            parse_sprite_folder = [&](const tinyxml2::XMLElement* folder_elem,
                                      GameData::SpriteFolderInfo& folder) {
            const char* name = folder_elem->Attribute("Name");
            if (name) folder.name = name;

            // Subfolders
            const tinyxml2::XMLElement* subfolders = folder_elem->FirstChildElement("SubFolders");
            if (subfolders) {
                for (const tinyxml2::XMLElement* sub = subfolders->FirstChildElement("SpriteFolder");
                     sub; sub = sub->NextSiblingElement("SpriteFolder")) {
                    GameData::SpriteFolderInfo child;
                    parse_sprite_folder(sub, child);
                    folder.subfolders.push_back(std::move(child));
                }
            }

            // Sprites in this folder
            const tinyxml2::XMLElement* sprites = folder_elem->FirstChildElement("Sprites");
            if (sprites) {
                for (const tinyxml2::XMLElement* spr = sprites->FirstChildElement("Sprite");
                     spr; spr = spr->NextSiblingElement("Sprite")) {
                    parse_sprite(spr);
                    int slot = 0;
                    spr->QueryIntAttribute("Slot", &slot);
                    folder.item_ids.push_back(slot);
                }
            }
        };

        const tinyxml2::XMLElement* sprites_wrapper = game_node->FirstChildElement("Sprites");
        if (sprites_wrapper) {
            const tinyxml2::XMLElement* sprite_folder = sprites_wrapper->FirstChildElement("SpriteFolder");
            if (sprite_folder) {
                parse_sprite_folder(sprite_folder, game_data_->root_sprite_folder);
            }
        }

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

    InitSubsystems(game_dir);
    return true;
}

bool Project::LoadGameFromAGS(const std::string& filepath)
{
    using namespace AGS::Common;

    // Try to open and read the main game file using AGS Common
    MainGameSource src;
    HGameFileError err = OpenMainGameFile(String(filepath.c_str()), src);
    if (!err)
    {
        fprintf(stderr, "Failed to open game file '%s': %s\n",
                filepath.c_str(), err->FullMessage().GetCStr());
        return false;
    }

    // Read game data
    GameSetupStruct game;
    LoadedGameEntities ents(game);
    err = ReadGameData(ents, std::move(src.InputStream), src.DataVersion);
    if (!err)
    {
        fprintf(stderr, "Failed to read game data: %s\n", err->FullMessage().GetCStr());
        return false;
    }

    // Apply updates/fixups
    err = UpdateGameData(ents, src.DataVersion);
    if (!err)
    {
        fprintf(stderr, "Warning: game data update had issues: %s\n", err->FullMessage().GetCStr());
        // Continue anyway, data may be usable
    }

    // Convert loaded AGS data into our editor GameData structure
    game_data_ = std::make_unique<GameData>();
    game_data_->game_title = game.gamename.GetCStr();
    game_data_->resolution_width = game.GetGameRes().Width;
    game_data_->resolution_height = game.GetGameRes().Height;
    game_data_->color_depth = game.color_depth * 8; // bytes to bits
    game_data_->target_fps = 40; // default; actual value read elsewhere

    // Characters
    for (int i = 0; i < game.numcharacters; i++)
    {
        GameData::CharacterInfo ch;
        ch.id = i;
        ch.script_name = game.chars[i].scrname;
        ch.real_name = game.chars[i].name;
        ch.room = game.chars[i].room;
        ch.x = game.chars[i].x;
        ch.y = game.chars[i].y;
        ch.normal_view = game.chars[i].defview;
        ch.speech_view = game.chars[i].talkview;
        ch.idle_view = game.chars[i].idleview;
        ch.thinking_view = game.chars[i].thinkview;
        ch.blinking_view = game.chars[i].blinkview;
        // Movement properties
        ch.movement_speed = game.chars[i].walkspeed;
        ch.movement_speed_x = game.chars[i].walkspeed;
        ch.movement_speed_y = game.chars[i].walkspeed_y;
        ch.uniform_movement_speed = (game.chars[i].walkspeed == game.chars[i].walkspeed_y);
        ch.animation_delay = game.chars[i].animspeed;
        ch.solid = !(game.chars[i].flags & CHF_NOBLOCKING);
        ch.blocking_width = game.chars[i].blocking_width;
        ch.blocking_height = game.chars[i].blocking_height;
        ch.turn_before_walking = !(game.chars[i].flags & CHF_NOTURNWHENWALK);
        ch.turn_when_facing = (game.chars[i].flags & CHF_TURNWHENFACE) != 0;
        ch.diagonal_loops = !(game.chars[i].flags & CHF_NODIAGONAL);
        ch.adjust_speed_with_scaling = (game.chars[i].flags & CHF_SCALEMOVESPEED) != 0;
        ch.movement_linked_to_animation = (game.chars[i].flags & CHF_ANTIGLIDE) != 0;
        // Appearance properties
        ch.baseline = game.chars[i].baseline;
        ch.clickable = !(game.chars[i].flags & CHF_NOINTERACT);
        ch.transparency = game.chars[i].transparency;
        ch.speech_animation_delay = game.chars[i].speech_anim_speed;
        ch.speech_color = game.chars[i].talkcolor;
        ch.use_room_area_lighting = !(game.chars[i].flags & CHF_NOLIGHTING);
        ch.scale_volume = (game.chars[i].flags & CHF_SCALEVOLUME) != 0;
        ch.idle_delay = game.chars[i].idledelay;
        ch.idle_anim_speed = game.chars[i].idle_anim_speed;
        // Player character
        ch.is_player = (i == game.playercharacter);
        game_data_->characters.push_back(ch);
    }

    // Views (with full loop/frame data)
    for (size_t i = 0; i < ents.Views.size(); i++)
    {
        GameData::ViewInfo v;
        v.id = (int)i;
        if (i < game.viewNames.size())
            v.name = game.viewNames[i].GetCStr();
        else
            v.name = "View " + std::to_string(i);
        v.loop_count = (int)ents.Views[i].loops.size();

        // Populate loop and frame data
        for (size_t l = 0; l < ents.Views[i].loops.size(); l++)
        {
            GameData::LoopData loop;
            loop.run_next_loop = (ents.Views[i].loops[l].flags & LOOPFLAG_RUNNEXTLOOP) != 0;
            for (size_t f = 0; f < ents.Views[i].loops[l].frames.size(); f++)
            {
                GameData::FrameData frame;
                frame.sprite_id = ents.Views[i].loops[l].frames[f].pic;
                frame.x_offset = ents.Views[i].loops[l].frames[f].xoffs;
                frame.y_offset = ents.Views[i].loops[l].frames[f].yoffs;
                frame.delay = ents.Views[i].loops[l].frames[f].speed;
                frame.flipped = (ents.Views[i].loops[l].frames[f].flags & VFLG_FLIPSPRITE) != 0;
                frame.sound = ents.Views[i].loops[l].frames[f].sound;
                loop.frames.push_back(frame);
            }
            v.loops.push_back(loop);
        }

        game_data_->views.push_back(v);
    }

    // Dialogs
    for (size_t i = 0; i < ents.Dialogs.size(); i++)
    {
        GameData::DialogInfo d;
        d.id = (int)i;
        if (i < game.dialogScriptNames.size())
            d.name = game.dialogScriptNames[i].GetCStr();
        else
            d.name = "Dialog " + std::to_string(i);
        d.option_count = (int)ents.Dialogs[i].Options.size();
        game_data_->dialogs.push_back(d);
    }

    // GUIs
    for (size_t i = 0; i < ents.Guis.size(); i++)
    {
        GameData::GUIInfo g;
        g.id = (int)i;
        g.name = ents.Guis[i].GetName().GetCStr();
        g.width = ents.Guis[i].GetWidth();
        g.height = ents.Guis[i].GetHeight();
        g.visible = ents.Guis[i].IsVisible();
        game_data_->guis.push_back(g);
    }

    // Cursors
    for (size_t i = 0; i < game.mcurs.size(); i++)
    {
        GameData::CursorInfo c;
        c.id = (int)i;
        c.name = game.mcurs[i].name.GetCStr();
        c.image = game.mcurs[i].pic;
        c.hotspot_x = game.mcurs[i].hotx;
        c.hotspot_y = game.mcurs[i].hoty;
        c.animate = (game.mcurs[i].view >= 0);
        c.view = game.mcurs[i].view;
        c.process_click = (game.mcurs[i].flags & MCF_STANDARD) != 0;
        game_data_->cursors.push_back(c);
    }

    // Inventory items
    for (int i = 0; i < game.numinvitems; i++)
    {
        GameData::InventoryItemInfo inv;
        inv.id = i;
        inv.script_name = std::string(game.invScriptNames[i].GetCStr());
        inv.description = std::string(game.invinfo[i].name.GetCStr());
        inv.image = game.invinfo[i].pic;
        inv.cursor_image = game.invinfo[i].cursorPic;
        inv.start_with = (game.invinfo[i].flags & IFLG_STARTWITH) != 0;
        game_data_->inventory_items.push_back(inv);
    }

    // Fonts (with extended info)
    for (size_t i = 0; i < game.fonts.size(); i++)
    {
        GameData::FontInfo f;
        f.id = (int)i;
        f.name = "Font " + std::to_string(i);
        f.size = game.fonts[i].Size;
        f.outline_type = (game.fonts[i].Flags & 0x01) ? 1 : 0; // auto outline flag
        f.outline_font = game.fonts[i].Outline;
        f.line_spacing = game.fonts[i].LineSpacing;
        f.size_multiplier = game.fonts[i].SizeMultiplier;
        game_data_->fonts.push_back(f);
    }

    // Audio clips
    for (size_t i = 0; i < game.audioClips.size(); i++)
    {
        GameData::AudioClipInfo a;
        a.id = (int)i;
        a.name = game.audioClips[i].scriptName.GetCStr();
        a.filename = game.audioClips[i].fileName.GetCStr();
        a.type = game.audioClips[i].type;
        a.default_volume = game.audioClips[i].defaultVolume;
        a.default_priority = game.audioClips[i].defaultPriority;
        a.default_repeat = game.audioClips[i].defaultRepeat;
        a.bundling_type = game.audioClips[i].bundlingType;
        a.file_type = (int)game.audioClips[i].fileType;
        game_data_->audio_clips.push_back(a);
    }

    // Rooms (from game data; detail room loading requires separate room files)
    for (int i = 0; i < game.roomCount; i++)
    {
        GameData::RoomInfo r;
        r.number = game.roomNumbers[i];
        if (i < (int)game.roomNames.size())
            r.description = game.roomNames[i].GetCStr();
        else
            r.description = "Room " + std::to_string(game.roomNumbers[i]);
        game_data_->rooms.push_back(r);
    }

    // Palette
    for (int i = 0; i < 256; i++)
    {
        GameData::PaletteEntry pe;
        pe.r = game.defpal[i].r * 4; // AGS palette is 6-bit, convert to 8-bit
        pe.g = game.defpal[i].g * 4;
        pe.b = game.defpal[i].b * 4;
        pe.colour_type = game.paluses[i]; // 0=Gamewide, 1=Locked, 2=Background
        game_data_->palette.push_back(pe);
    }

    // Sprite count info
    for (size_t i = 0; i < ents.SpriteCount && i < ents.SpriteFlags.size(); i++)
    {
        GameData::SpriteInfo s;
        s.id = (int)i;
        s.width = 0;   // Actual dimensions require reading sprite file
        s.height = 0;
        s.color_depth = 0;
        game_data_->sprites.push_back(s);
    }

    game_title_ = game_data_->game_title;

    // Initialize subsystems
    std::string dir = fs::path(filepath).parent_path().string();
    InitSubsystems(dir);

    return true;
}

bool Project::ImportOldGame(const std::string& game_file, bool create_backup,
                             const std::string& backup_dir, bool import_editor_dat)
{
    CloseProject();

    ImportOptions opts;
    opts.game_file_path = game_file;
    opts.create_backup = create_backup;
    opts.backup_dir = backup_dir;
    opts.import_editor_dat = import_editor_dat;

    game_data_ = std::make_unique<GameData>();

    ImportResult result = OldGameImporter::Import(opts, *game_data_,
        [](const std::string& status, float) {
            fprintf(stderr, "[ImportOld] %s\n", status.c_str());
        });

    if (!result.success)
    {
        fprintf(stderr, "[Project] Import failed: %s\n", result.error_message.c_str());
        game_data_.reset();
        return false;
    }

    project_path_ = game_file;
    UpdateProjectDir();
    game_title_ = game_data_->game_title;

    // Initialize subsystems
    InitSubsystems(project_dir_);

    loaded_ = true;
    dirty_ = true; // Mark dirty — needs saving in new format
    recent_files_.AddFile(project_path_);

    for (const auto& w : result.warnings)
        fprintf(stderr, "[ImportOld] Warning: %s\n", w.c_str());

    fprintf(stderr, "[ImportOld] Import complete: %d chars, %d views, %d dialogs, "
            "%d GUIs, %d rooms, %d sprites\n",
            result.characters_imported, result.views_imported,
            result.dialogs_imported, result.guis_imported,
            result.rooms_imported, result.sprites_imported);

    return true;
}

bool Project::OpenProject(const std::string& path)
{
    CloseProject();

    project_path_ = path;
    UpdateProjectDir();

    // Determine file extension
    std::string ext = fs::path(path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    bool loaded_ok = false;

    if (ext == ".agf")
    {
        // .agf is the XML project format — parse it directly
        loaded_ok = LoadFromAGF(path);

        // If the file has .agf extension but failed to parse as XML,
        // don't try binary loading — it's definitely an XML project file
        if (!loaded_ok)
        {
            fprintf(stderr, "[Project] Failed to load AGF project '%s'.\n", path.c_str());
            return false;
        }
    }
    else
    {
        // Try loading as binary AGS game file (game.ags / ac2game.dta / game28.dta)
        loaded_ok = LoadGameFromAGS(path);

        if (!loaded_ok)
        {
            // Check if there's a Game.agf in the same directory
            std::string agf_path = project_dir_ + "/Game.agf";
            if (fs::exists(agf_path))
            {
                fprintf(stderr, "[Project] Binary load failed, trying Game.agf in '%s'...\n",
                        project_dir_.c_str());
                project_path_ = agf_path;
                loaded_ok = LoadFromAGF(agf_path);
            }
        }

        if (!loaded_ok)
        {
            fprintf(stderr, "[Project] Could not load game data from '%s'.\n", path.c_str());
            return false;
        }
    }

    loaded_ = true;
    dirty_ = false;
    recent_files_.AddFile(project_path_);

    return true;
}

// -------------------------------------------------------------------------
// XML writing helper functions for AGF serialization
// -------------------------------------------------------------------------

static void XmlWriteText(tinyxml2::XMLDocument& doc, tinyxml2::XMLElement* parent,
                          const char* name, const char* value)
{
    tinyxml2::XMLElement* elem = doc.NewElement(name);
    if (value && value[0] != '\0')
        elem->SetText(value);
    parent->InsertEndChild(elem);
}

static void XmlWriteInt(tinyxml2::XMLDocument& doc, tinyxml2::XMLElement* parent,
                         const char* name, int value)
{
    tinyxml2::XMLElement* elem = doc.NewElement(name);
    elem->SetText(value);
    parent->InsertEndChild(elem);
}

static void XmlWriteBool(tinyxml2::XMLDocument& doc, tinyxml2::XMLElement* parent,
                          const char* name, bool value)
{
    tinyxml2::XMLElement* elem = doc.NewElement(name);
    elem->SetText(value ? "True" : "False");
    parent->InsertEndChild(elem);
}

// -------------------------------------------------------------------------
// SaveToAGF — Write GameData to .agf XML project file
// -------------------------------------------------------------------------
bool Project::SaveToAGF(const std::string& agf_path)
{
    if (!game_data_)
        return false;

    tinyxml2::XMLDocument doc;
    doc.InsertFirstChild(doc.NewDeclaration());

    tinyxml2::XMLElement* root = doc.NewElement("AGSEditorDocument");
    root->SetAttribute("Version", "3.6.1.0");
    root->SetAttribute("VersionIndex", "20");
    doc.InsertEndChild(root);

    tinyxml2::XMLElement* game_node = doc.NewElement("Game");
    root->InsertEndChild(game_node);

    // --- Settings ---
    {
        tinyxml2::XMLElement* settings = doc.NewElement("Settings");
        game_node->InsertEndChild(settings);

        XmlWriteBool(doc, settings, "AlwaysDisplayTextAsSpeech", game_data_->always_display_text_as_speech);
        XmlWriteBool(doc, settings, "AntiAliasFonts", game_data_->anti_alias_fonts);
        XmlWriteBool(doc, settings, "AntiGlideMode", game_data_->pixel_perfect_speed);

        // ColorDepth
        const char* depth_str = "TrueColor";
        if (game_data_->color_depth == 16) depth_str = "HighColor";
        else if (game_data_->color_depth == 8) depth_str = "Palette";
        XmlWriteText(doc, settings, "ColorDepth", depth_str);

        // CompressSpritesType
        const char* compress_names[] = { "None", "RLE", "LZW", "Deflate" };
        int comp_idx = game_data_->sprite_file_compression;
        if (comp_idx < 0 || comp_idx > 3) comp_idx = 0;
        XmlWriteText(doc, settings, "CompressSpritesType", compress_names[comp_idx]);

        // CustomResolution
        char res_buf[64];
        snprintf(res_buf, sizeof(res_buf), "%d,%d",
                 game_data_->resolution_width, game_data_->resolution_height);
        XmlWriteText(doc, settings, "CustomResolution", res_buf);

        XmlWriteBool(doc, settings, "DebugMode", game_data_->debug_mode);
        XmlWriteInt(doc, settings, "DefaultRoomMaskResolution", game_data_->default_room_mask_resolution);
        XmlWriteText(doc, settings, "Description", game_data_->description.c_str());
        XmlWriteText(doc, settings, "DeveloperName", game_data_->developer_name.c_str());
        XmlWriteText(doc, settings, "DeveloperURL", game_data_->developer_url.c_str());

        // Dialog settings
        XmlWriteInt(doc, settings, "DialogOptionsBullet", game_data_->dialog_bullet_sprite);
        XmlWriteInt(doc, settings, "DialogOptionsGUI", game_data_->dialog_options_gui);
        XmlWriteInt(doc, settings, "DialogOptionsGap", game_data_->dialog_options_gap);
        XmlWriteText(doc, settings, "DialogScriptNarrateFunction",
                     game_data_->dialog_narrate_function.empty() ? nullptr : game_data_->dialog_narrate_function.c_str());
        XmlWriteText(doc, settings, "DialogScriptSayFunction",
                     game_data_->dialog_say_function.empty() ? nullptr : game_data_->dialog_say_function.c_str());
        XmlWriteBool(doc, settings, "DisplayMultipleInventory", game_data_->display_multiple_inv);
        XmlWriteBool(doc, settings, "EnforceNewAudio", game_data_->enforce_new_audio);
        XmlWriteBool(doc, settings, "EnforceNewStrings", game_data_->enforce_new_strings);
        XmlWriteBool(doc, settings, "EnforceObjectBasedScript", game_data_->enforce_object_scripting);
        XmlWriteInt(doc, settings, "GameFPS", game_data_->target_fps);
        XmlWriteText(doc, settings, "GameFileName", game_data_->game_file_name.c_str());
        XmlWriteText(doc, settings, "GameName", game_data_->game_title.c_str());
        XmlWriteInt(doc, settings, "GlobalSpeechAnimationDelay", game_data_->global_speech_anim_delay);
        XmlWriteBool(doc, settings, "HandleInvClicksInScript", game_data_->handle_inv_clicks_in_script);
        XmlWriteBool(doc, settings, "InventoryCursors", game_data_->inventory_cursors);
        XmlWriteBool(doc, settings, "LeftToRightPrecedence", game_data_->left_to_right_precedence);
        XmlWriteInt(doc, settings, "MaximumScore", game_data_->max_score);
        XmlWriteText(doc, settings, "NumberDialogOptions",
                     game_data_->number_dialog_options ? "KeyShortcut" : "None");
        XmlWriteInt(doc, settings, "PlaySoundOnScore",
                    game_data_->play_sound_on_score ? game_data_->score_sound_clip : 0);

        // RenderAtScreenResolution
        XmlWriteText(doc, settings, "RenderAtScreenResolution",
                     game_data_->render_at_screen_resolution ? "True" : "UserDefined");

        // RoomTransition
        const char* transition_names[] = { "Instant", "FadeOutAndIn", "Dissolve", "BoxOut", "Crossfade" };
        int trans_idx = game_data_->room_transition;
        if (trans_idx < 0 || trans_idx > 4) trans_idx = 0;
        XmlWriteText(doc, settings, "RoomTransition", transition_names[trans_idx]);

        XmlWriteBool(doc, settings, "RunGameLoopsWhileDialogOptionsDisplayed", game_data_->run_game_loops_in_dialog);
        XmlWriteText(doc, settings, "SaveGameFileExtension", game_data_->save_game_extension.c_str());
        XmlWriteText(doc, settings, "SaveGameFolderName", game_data_->save_game_folder.c_str());
        XmlWriteBool(doc, settings, "SaveScreenshots", game_data_->save_screenshots);

        // ScriptAPIVersion and ScriptCompatLevel
        // Reverse-lookup actual kScriptAPI_* value to version name string
        const char* api_name = "Highest";
        if (game_data_->script_api_version != INT_MAX) {
            for (int i = 0; i < kScriptAPIVersionCount; i++) {
                if (kScriptAPIVersions[i].value == game_data_->script_api_version) {
                    api_name = kScriptAPIVersions[i].name;
                    break;
                }
            }
        }
        XmlWriteText(doc, settings, "ScriptAPIVersion", api_name);
        const char* compat_name = "Highest";
        if (game_data_->script_compat_level != INT_MAX) {
            for (int i = 0; i < kScriptAPIVersionCount; i++) {
                if (kScriptAPIVersions[i].value == game_data_->script_compat_level) {
                    compat_name = kScriptAPIVersions[i].name;
                    break;
                }
            }
        }
        XmlWriteText(doc, settings, "ScriptCompatLevel", compat_name);

        // SkipSpeech
        const char* skip_names[] = { "MouseOrKeyboardOrTimer", "KeyboardOnly", "MouseOnly", "TimerOnly" };
        int skip_idx = game_data_->skip_speech_style;
        if (skip_idx < 0 || skip_idx > 3) skip_idx = 0;
        XmlWriteText(doc, settings, "SkipSpeech", skip_names[skip_idx]);

        // SpeechStyle
        const char* speech_names[] = { "Lucasarts", "Sierra", "SierraWithBackground", "FullScreen" };
        int speech_idx = game_data_->speech_style;
        if (speech_idx < 0 || speech_idx > 3) speech_idx = 0;
        XmlWriteText(doc, settings, "SpeechStyle", speech_names[speech_idx]);

        XmlWriteInt(doc, settings, "SplitResources",
                    game_data_->split_resources ? game_data_->split_resource_threshold : 0);
        XmlWriteInt(doc, settings, "TextWindowGUI", game_data_->text_window_gui);
        XmlWriteInt(doc, settings, "ThoughtGUI", game_data_->thought_gui);
        XmlWriteBool(doc, settings, "TurnBeforeFacing", game_data_->turn_before_facing);
        XmlWriteBool(doc, settings, "TurnBeforeWalking", game_data_->turn_before_walking);
        XmlWriteBool(doc, settings, "UseGlobalSpeechAnimationDelay", game_data_->use_global_speech_anim_delay);
        XmlWriteBool(doc, settings, "UseOldCustomDialogOptionsAPI", game_data_->use_old_custom_dialog_api);
        XmlWriteBool(doc, settings, "UseOldKeyboardHandling", game_data_->use_old_keyboard_handling);

        // WhenInterfaceDisabled
        const char* iface_names[] = { "GreyOut", "GoBlack", "DisplayNormally", "SetGUIDisabled" };
        int iface_idx = game_data_->when_interface_disabled;
        if (iface_idx < 0 || iface_idx > 3) iface_idx = 0;
        XmlWriteText(doc, settings, "WhenInterfaceDisabled", iface_names[iface_idx]);
    }

    // --- Runtime Setup (Default Setup) ---
    {
        tinyxml2::XMLElement* rt = doc.NewElement("RuntimeSetup");
        game_node->InsertEndChild(rt);
        const auto& rs = game_data_->runtime_setup;

        // Graphics
        const char* gfx_drv_names[] = { "Software", "D3D9", "OpenGL" };
        int gfx_idx = rs.graphics_driver;
        if (gfx_idx < 0 || gfx_idx > 2) gfx_idx = 1;
        XmlWriteText(doc, rt, "GraphicsDriver", gfx_drv_names[gfx_idx]);
        XmlWriteBool(doc, rt, "Windowed", rs.windowed);
        XmlWriteBool(doc, rt, "FullscreenDesktop", rs.fullscreen_desktop);

        const char* fs_scale_names[] = { "None", "ProportionalStretch", "StretchToFit", "Integer" };
        int fs_idx = rs.fullscreen_scaling;
        if (fs_idx < 0 || fs_idx > 3) fs_idx = 1;
        XmlWriteText(doc, rt, "FullscreenGameScaling", fs_scale_names[fs_idx]);

        const char* win_scale_names[] = { "None", "MaxRound", "StretchToFit", "MaxInteger" };
        int ws_idx = rs.windowed_scaling;
        if (ws_idx < 0 || ws_idx > 3) ws_idx = 3;
        XmlWriteText(doc, rt, "GameScaling", win_scale_names[ws_idx]);
        XmlWriteInt(doc, rt, "GameScalingMultiplier", rs.scaling_multiplier);

        const char* filter_names[] = { "stdscale", "linear" };
        int fi_idx = rs.graphics_filter;
        if (fi_idx < 0 || fi_idx > 1) fi_idx = 0;
        XmlWriteText(doc, rt, "GraphicsFilter", filter_names[fi_idx]);

        XmlWriteBool(doc, rt, "VSync", rs.vsync);
        XmlWriteBool(doc, rt, "AAScaledSprites", rs.aa_scaled_sprites);
        XmlWriteBool(doc, rt, "RenderAtScreenResolution", rs.render_at_screen_res);

        const char* rot_names[] = { "Unlocked", "Portrait", "Landscape" };
        int rot_idx = rs.rotation;
        if (rot_idx < 0 || rot_idx > 2) rot_idx = 0;
        XmlWriteText(doc, rt, "Rotation", rot_names[rot_idx]);

        // Audio
        XmlWriteInt(doc, rt, "DigitalSound", rs.digital_sound);
        XmlWriteBool(doc, rt, "UseVoicePack", rs.use_voice_pack);

        // Gameplay
        XmlWriteText(doc, rt, "Translation", rs.translation.c_str());

        // Mouse
        XmlWriteBool(doc, rt, "AutoLockMouse", rs.auto_lock_mouse);
        char ms_buf[32];
        snprintf(ms_buf, sizeof(ms_buf), "%.2f", rs.mouse_speed);
        XmlWriteText(doc, rt, "MouseSpeed", ms_buf);

        // Touch
        XmlWriteInt(doc, rt, "TouchToMouseEmulation", rs.touch_emulation);
        XmlWriteInt(doc, rt, "TouchToMouseMotionMode", rs.touch_motion);

        // Misc
        XmlWriteBool(doc, rt, "ShowFPS", rs.show_fps);

        // Performance
        XmlWriteInt(doc, rt, "SpriteCacheSize", rs.sprite_cache_size);
        XmlWriteInt(doc, rt, "TextureCacheSize", rs.texture_cache_size);
        XmlWriteInt(doc, rt, "SoundCacheSize", rs.sound_cache_size);
        XmlWriteBool(doc, rt, "CompressSaves", rs.compress_saves);

        // Environment
        XmlWriteBool(doc, rt, "UseCustomSavePath", rs.use_custom_save_path);
        XmlWriteText(doc, rt, "CustomSavePath", rs.custom_save_path.c_str());
        XmlWriteBool(doc, rt, "UseCustomAppDataPath", rs.use_custom_appdata_path);
        XmlWriteText(doc, rt, "CustomAppDataPath", rs.custom_appdata_path.c_str());

        // Setup appearance
        XmlWriteText(doc, rt, "TitleText", rs.title_text.c_str());
    }

    // --- Rooms ---
    {
        tinyxml2::XMLElement* rooms = doc.NewElement("Rooms");
        game_node->InsertEndChild(rooms);
        tinyxml2::XMLElement* room_folder = doc.NewElement("UnloadedRoomFolder");
        room_folder->SetAttribute("Name", "Main");
        rooms->InsertEndChild(room_folder);
        tinyxml2::XMLElement* sub_folders = doc.NewElement("SubFolders");
        room_folder->InsertEndChild(sub_folders);
        tinyxml2::XMLElement* room_list = doc.NewElement("UnloadedRooms");
        room_folder->InsertEndChild(room_list);

        for (const auto& r : game_data_->rooms)
        {
            tinyxml2::XMLElement* room_elem = doc.NewElement("UnloadedRoom");
            XmlWriteInt(doc, room_elem, "Number", r.number);
            XmlWriteText(doc, room_elem, "Description", r.description.c_str());
            room_list->InsertEndChild(room_elem);
        }
    }

    // --- Characters ---
    {
        tinyxml2::XMLElement* chars = doc.NewElement("Characters");
        game_node->InsertEndChild(chars);
        tinyxml2::XMLElement* char_folder = doc.NewElement("CharacterFolder");
        chars->InsertEndChild(char_folder);

        // Build ID lookup map
        std::unordered_map<int, const GameData::CharacterInfo*> char_map;
        for (const auto& ch : game_data_->characters)
            char_map[ch.id] = &ch;

        auto write_character = [this, &char_map](tinyxml2::XMLDocument& d,
                                                  tinyxml2::XMLElement* parent, int id) {
            auto it = char_map.find(id);
            if (it == char_map.end()) return;
            const auto& ch = *it->second;
            tinyxml2::XMLElement* ch_elem = d.NewElement("Character");
            XmlWriteBool(d, ch_elem, "AdjustSpeedWithScaling", ch.adjust_speed_with_scaling);
            XmlWriteBool(d, ch_elem, "AdjustVolumeWithScaling", ch.scale_volume);
            XmlWriteInt(d, ch_elem, "AnimationDelay", ch.animation_delay);
            XmlWriteInt(d, ch_elem, "Baseline", ch.baseline);
            XmlWriteInt(d, ch_elem, "BlinkingView", ch.blinking_view);
            XmlWriteBool(d, ch_elem, "Clickable", ch.clickable);
            XmlWriteBool(d, ch_elem, "DiagonalLoops", ch.diagonal_loops);
            XmlWriteInt(d, ch_elem, "ID", ch.id);
            XmlWriteInt(d, ch_elem, "IdleAnimationDelay", ch.idle_anim_speed);
            XmlWriteInt(d, ch_elem, "IdleDelay", ch.idle_delay);
            XmlWriteInt(d, ch_elem, "IdleView", ch.idle_view);
            XmlWriteBool(d, ch_elem, "MovementLinkedToAnimation", ch.movement_linked_to_animation);
            XmlWriteInt(d, ch_elem, "MovementSpeed", ch.movement_speed);
            XmlWriteInt(d, ch_elem, "MovementSpeedX", ch.movement_speed_x);
            XmlWriteInt(d, ch_elem, "MovementSpeedY", ch.movement_speed_y);
            XmlWriteInt(d, ch_elem, "NormalView", ch.normal_view);
            XmlWriteText(d, ch_elem, "RealName", ch.real_name.c_str());
            XmlWriteText(d, ch_elem, "ScriptName", ch.script_name.c_str());
            XmlWriteBool(d, ch_elem, "Solid", ch.solid);
            XmlWriteInt(d, ch_elem, "SpeechAnimationDelay", ch.speech_animation_delay);
            XmlWriteInt(d, ch_elem, "SpeechColor", ch.speech_color);
            XmlWriteInt(d, ch_elem, "SpeechView", ch.speech_view);
            XmlWriteInt(d, ch_elem, "StartX", ch.x);
            XmlWriteInt(d, ch_elem, "StartY", ch.y);
            XmlWriteInt(d, ch_elem, "StartingRoom", ch.room);
            XmlWriteInt(d, ch_elem, "ThinkingView", ch.thinking_view);
            XmlWriteInt(d, ch_elem, "Transparency", ch.transparency);
            XmlWriteBool(d, ch_elem, "TurnBeforeWalking", ch.turn_before_walking);
            XmlWriteBool(d, ch_elem, "TurnWhenFacing", ch.turn_when_facing);
            XmlWriteBool(d, ch_elem, "UniformMovementSpeed", ch.uniform_movement_speed);
            XmlWriteBool(d, ch_elem, "UseRoomAreaLighting", ch.use_room_area_lighting);
            XmlWriteBool(d, ch_elem, "UseRoomAreaScaling", ch.use_room_area_scaling);
            // Event handlers
            SaveInteractions(d, ch_elem, ch.interactions);
            // Custom properties
            if (!ch.custom_properties.empty())
            {
                tinyxml2::XMLElement* props = d.NewElement("Properties");
                for (const auto& kv : ch.custom_properties)
                {
                    tinyxml2::XMLElement* cp = d.NewElement("CustomProperty");
                    XmlWriteText(d, cp, "Name", kv.first.c_str());
                    XmlWriteText(d, cp, "Value", kv.second.c_str());
                    props->InsertEndChild(cp);
                }
                ch_elem->InsertEndChild(props);
            }
            parent->InsertEndChild(ch_elem);
        };

        WriteFolderTree(doc, char_folder, game_data_->character_folders,
                        "CharacterFolder", "Characters", write_character);
    }

    // --- Views ---
    {
        tinyxml2::XMLElement* views = doc.NewElement("Views");
        game_node->InsertEndChild(views);
        tinyxml2::XMLElement* view_folder = doc.NewElement("ViewFolder");
        views->InsertEndChild(view_folder);

        std::unordered_map<int, const GameData::ViewInfo*> view_map;
        for (const auto& v : game_data_->views)
            view_map[v.id] = &v;

        WriteFolderTree(doc, view_folder, game_data_->view_folders,
            "ViewFolder", "Views",
            [&view_map](tinyxml2::XMLDocument& d, tinyxml2::XMLElement* parent, int id) {
                auto it = view_map.find(id);
                if (it == view_map.end()) return;
                const auto& v = *it->second;
                tinyxml2::XMLElement* view_elem = d.NewElement("View");
                XmlWriteInt(d, view_elem, "ID", v.id);
                XmlWriteText(d, view_elem, "Name", v.name.c_str());

                tinyxml2::XMLElement* loops_elem = d.NewElement("Loops");
                view_elem->InsertEndChild(loops_elem);

                int loop_id = 0;
                for (const auto& loop : v.loops)
                {
                    tinyxml2::XMLElement* loop_elem = d.NewElement("Loop");
                    XmlWriteInt(d, loop_elem, "ID", loop_id++);
                    XmlWriteBool(d, loop_elem, "RunNextLoop", loop.run_next_loop);

                    tinyxml2::XMLElement* frames_elem = d.NewElement("Frames");
                    loop_elem->InsertEndChild(frames_elem);

                    for (const auto& frame : loop.frames)
                    {
                        tinyxml2::XMLElement* fr_elem = d.NewElement("ViewFrame");
                        XmlWriteInt(d, fr_elem, "Image", frame.sprite_id);
                        XmlWriteInt(d, fr_elem, "OffsetX", frame.x_offset);
                        XmlWriteInt(d, fr_elem, "OffsetY", frame.y_offset);
                        XmlWriteInt(d, fr_elem, "Delay", frame.delay);
                        XmlWriteBool(d, fr_elem, "Flipped", frame.flipped);
                        XmlWriteInt(d, fr_elem, "Sound", frame.sound);
                        frames_elem->InsertEndChild(fr_elem);
                    }
                    loops_elem->InsertEndChild(loop_elem);
                }
                parent->InsertEndChild(view_elem);
            });
    }

    // --- GUIs ---
    {
        tinyxml2::XMLElement* guis = doc.NewElement("GUIs");
        game_node->InsertEndChild(guis);
        tinyxml2::XMLElement* gui_folder = doc.NewElement("GUIFolder");
        guis->InsertEndChild(gui_folder);

        std::unordered_map<int, const GameData::GUIInfo*> gui_map;
        for (const auto& g : game_data_->guis)
            gui_map[g.id] = &g;

        auto write_gui = [&gui_map](tinyxml2::XMLDocument& d,
                                     tinyxml2::XMLElement* parent, int id) {
            auto it = gui_map.find(id);
            if (it == gui_map.end()) return;
            const auto& g = *it->second;

            const char* gui_tag = g.tag_name.empty() ? "GUIMain" : g.tag_name.c_str();
            tinyxml2::XMLElement* gui_elem = d.NewElement(gui_tag);

            const char* props_tag = (strcmp(gui_tag, "GUITextWindow") == 0) ? "TextWindowGUI" : "NormalGUI";
            tinyxml2::XMLElement* props = d.NewElement(props_tag);
            XmlWriteInt(d, props, "ID", g.id);
            XmlWriteText(d, props, "Name", g.name.c_str());
            XmlWriteInt(d, props, "Left", g.x);
            XmlWriteInt(d, props, "Top", g.y);
            XmlWriteInt(d, props, "Width", g.width);
            XmlWriteInt(d, props, "Height", g.height);
            XmlWriteInt(d, props, "BackgroundColor", g.bg_color);
            XmlWriteInt(d, props, "BorderColor", g.border_color);
            XmlWriteInt(d, props, "Transparency", g.transparency);
            XmlWriteInt(d, props, "ZOrder", g.z_order);
            XmlWriteBool(d, props, "Clickable", g.clickable);

            const char* vis_names[] = { "Normal", "MouseYPos", "ScriptOnly", "Persistent" };
            int ps = g.popup_style;
            if (ps < 0 || ps > 3) ps = 0;
            XmlWriteText(d, props, "Visibility", vis_names[ps]);

            if (!g.on_click.empty())
                XmlWriteText(d, props, "OnClick", g.on_click.c_str());
            if (!g.script_module.empty())
                XmlWriteText(d, props, "ScriptModule", g.script_module.c_str());

            gui_elem->InsertEndChild(props);

            // Controls
            if (!g.controls.empty())
            {
                tinyxml2::XMLElement* controls = d.NewElement("Controls");
                for (const auto& ci : g.controls)
                {
                    const char* ctrl_tag = ci.type_tag.empty() ? "GUIButton" : ci.type_tag.c_str();
                    tinyxml2::XMLElement* ctrl = d.NewElement(ctrl_tag);
                    XmlWriteInt(d, ctrl, "ID", ci.id);
                    XmlWriteText(d, ctrl, "Name", ci.name.c_str());
                    XmlWriteInt(d, ctrl, "Left", ci.x);
                    XmlWriteInt(d, ctrl, "Top", ci.y);
                    XmlWriteInt(d, ctrl, "Width", ci.width);
                    XmlWriteInt(d, ctrl, "Height", ci.height);
                    XmlWriteText(d, ctrl, "Text", ci.text.c_str());
                    XmlWriteInt(d, ctrl, "Image", ci.image);
                    XmlWriteInt(d, ctrl, "Font", ci.font);
                    // Write event handler based on control type
                    if (!ci.event_handler.empty())
                    {
                        if (ci.type_tag == "GUIButton")
                            XmlWriteText(d, ctrl, "OnClick", ci.event_handler.c_str());
                        else if (ci.type_tag == "GUIListBox")
                            XmlWriteText(d, ctrl, "OnSelectionChanged", ci.event_handler.c_str());
                        else if (ci.type_tag == "GUISlider")
                            XmlWriteText(d, ctrl, "OnChange", ci.event_handler.c_str());
                        else if (ci.type_tag == "GUITextBox")
                            XmlWriteText(d, ctrl, "OnActivate", ci.event_handler.c_str());
                    }
                    controls->InsertEndChild(ctrl);
                }
                gui_elem->InsertEndChild(controls);
            }
            parent->InsertEndChild(gui_elem);
        };

        WriteFolderTree(doc, gui_folder, game_data_->gui_folders,
                        "GUIFolder", "GUIs", write_gui);
    }

    // --- Inventory Items ---
    {
        tinyxml2::XMLElement* inv = doc.NewElement("InventoryItems");
        game_node->InsertEndChild(inv);
        tinyxml2::XMLElement* inv_folder = doc.NewElement("InventoryItemFolder");
        inv->InsertEndChild(inv_folder);

        std::unordered_map<int, const GameData::InventoryItemInfo*> inv_map;
        for (const auto& item : game_data_->inventory_items)
            inv_map[item.id] = &item;

        WriteFolderTree(doc, inv_folder, game_data_->inventory_folders,
            "InventoryItemFolder", "InventoryItems",
            [this, &inv_map](tinyxml2::XMLDocument& d, tinyxml2::XMLElement* parent, int id) {
                auto it = inv_map.find(id);
                if (it == inv_map.end()) return;
                const auto& item = *it->second;
                tinyxml2::XMLElement* elem = d.NewElement("InventoryItem");
                XmlWriteInt(d, elem, "ID", item.id);
                XmlWriteText(d, elem, "Name", item.script_name.c_str());
                XmlWriteText(d, elem, "Description", item.description.c_str());
                XmlWriteInt(d, elem, "Image", item.image);
                XmlWriteInt(d, elem, "CursorImage", item.cursor_image);
                XmlWriteBool(d, elem, "PlayerStartsWithItem", item.start_with);
                SaveInteractions(d, elem, item.interactions);
                if (!item.custom_properties.empty())
                {
                    tinyxml2::XMLElement* props = d.NewElement("Properties");
                    for (const auto& kv : item.custom_properties)
                    {
                        tinyxml2::XMLElement* cp = d.NewElement("CustomProperty");
                        XmlWriteText(d, cp, "Name", kv.first.c_str());
                        XmlWriteText(d, cp, "Value", kv.second.c_str());
                        props->InsertEndChild(cp);
                    }
                    elem->InsertEndChild(props);
                }
                parent->InsertEndChild(elem);
            });
    }

    // --- Dialogs ---
    {
        tinyxml2::XMLElement* dlgs = doc.NewElement("Dialogs");
        game_node->InsertEndChild(dlgs);
        tinyxml2::XMLElement* dlg_folder = doc.NewElement("DialogFolder");
        dlgs->InsertEndChild(dlg_folder);

        std::unordered_map<int, const GameData::DialogInfo*> dlg_map;
        for (const auto& d : game_data_->dialogs)
            dlg_map[d.id] = &d;

        WriteFolderTree(doc, dlg_folder, game_data_->dialog_folders,
            "DialogFolder", "Dialogs",
            [&dlg_map](tinyxml2::XMLDocument& dd, tinyxml2::XMLElement* parent, int id) {
                auto it = dlg_map.find(id);
                if (it == dlg_map.end()) return;
                const auto& d = *it->second;
                tinyxml2::XMLElement* elem = dd.NewElement("Dialog");
                XmlWriteInt(dd, elem, "ID", d.id);
                XmlWriteText(dd, elem, "Name", d.name.c_str());
                XmlWriteText(dd, elem, "ScriptName", d.script_name.c_str());
                XmlWriteBool(dd, elem, "ShowTextParser", d.show_text_parser);
                if (!d.script.empty())
                {
                    tinyxml2::XMLElement* script_elem = dd.NewElement("Script");
                    tinyxml2::XMLText* cdata = dd.NewText(d.script.c_str());
                    cdata->SetCData(true);
                    script_elem->InsertEndChild(cdata);
                    elem->InsertEndChild(script_elem);
                }
                if (!d.options.empty())
                {
                    tinyxml2::XMLElement* opts_elem = dd.NewElement("DialogOptions");
                    for (const auto& opt : d.options)
                    {
                        tinyxml2::XMLElement* opt_elem = dd.NewElement("DialogOption");
                        XmlWriteBool(dd, opt_elem, "Say", opt.say);
                        XmlWriteBool(dd, opt_elem, "Show", opt.show);
                        XmlWriteText(dd, opt_elem, "Text", opt.text.c_str());
                        opts_elem->InsertEndChild(opt_elem);
                    }
                    elem->InsertEndChild(opts_elem);
                }
                parent->InsertEndChild(elem);
            });
    }

    // --- Cursors (flat list) ---
    {
        tinyxml2::XMLElement* cursors = doc.NewElement("Cursors");
        game_node->InsertEndChild(cursors);

        for (const auto& c : game_data_->cursors)
        {
            tinyxml2::XMLElement* elem = doc.NewElement("MouseCursor");
            XmlWriteInt(doc, elem, "ID", c.id);
            XmlWriteText(doc, elem, "Name", c.name.c_str());
            XmlWriteInt(doc, elem, "Image", c.image);
            XmlWriteInt(doc, elem, "HotspotX", c.hotspot_x);
            XmlWriteInt(doc, elem, "HotspotY", c.hotspot_y);
            XmlWriteBool(doc, elem, "Animate", c.animate);
            XmlWriteInt(doc, elem, "View", c.view);
            XmlWriteBool(doc, elem, "StandardMode", c.process_click);
            cursors->InsertEndChild(elem);
        }
    }

    // --- Fonts (flat list) ---
    {
        tinyxml2::XMLElement* fonts = doc.NewElement("Fonts");
        game_node->InsertEndChild(fonts);

        for (const auto& f : game_data_->fonts)
        {
            tinyxml2::XMLElement* elem = doc.NewElement("Font");
            XmlWriteInt(doc, elem, "ID", f.id);
            XmlWriteText(doc, elem, "Name", f.name.c_str());
            XmlWriteInt(doc, elem, "PointSize", f.size);
            XmlWriteText(doc, elem, "SourceFilename", f.source_file.c_str());
            XmlWriteInt(doc, elem, "LineSpacing", f.line_spacing);
            XmlWriteInt(doc, elem, "SizeMultiplier", f.size_multiplier);
            const char* outline_names[] = { "None", "Automatic", "UseOutlineFont" };
            int ot = f.outline_type;
            if (ot < 0 || ot > 2) ot = 0;
            XmlWriteText(doc, elem, "OutlineStyle", outline_names[ot]);
            XmlWriteInt(doc, elem, "OutlineFont", f.outline_font);
            fonts->InsertEndChild(elem);
        }
    }

    // --- Scripts ---
    {
        tinyxml2::XMLElement* scripts = doc.NewElement("Scripts");
        game_node->InsertEndChild(scripts);
        tinyxml2::XMLElement* script_folder = doc.NewElement("ScriptFolder");
        script_folder->SetAttribute("Name", "Main");
        scripts->InsertEndChild(script_folder);
        tinyxml2::XMLElement* sub_folders = doc.NewElement("SubFolders");
        script_folder->InsertEndChild(sub_folders);
        tinyxml2::XMLElement* script_list = doc.NewElement("ScriptAndHeaders");
        script_folder->InsertEndChild(script_list);

        for (const auto& mod : game_data_->script_modules)
        {
            tinyxml2::XMLElement* sah = doc.NewElement("ScriptAndHeader");

            tinyxml2::XMLElement* hdr_wrap = doc.NewElement("ScriptAndHeader_Header");
            tinyxml2::XMLElement* hdr_script = doc.NewElement("Script");
            XmlWriteText(doc, hdr_script, "FileName", mod.header_file.c_str());
            XmlWriteText(doc, hdr_script, "Name", mod.name.c_str());
            hdr_wrap->InsertEndChild(hdr_script);
            sah->InsertEndChild(hdr_wrap);

            tinyxml2::XMLElement* scr_wrap = doc.NewElement("ScriptAndHeader_Script");
            tinyxml2::XMLElement* scr_script = doc.NewElement("Script");
            XmlWriteText(doc, scr_script, "FileName", mod.script_file.c_str());
            XmlWriteText(doc, scr_script, "Name", mod.name.c_str());
            scr_wrap->InsertEndChild(scr_script);
            sah->InsertEndChild(scr_wrap);

            script_list->InsertEndChild(sah);
        }
    }

    // --- Audio Clips ---
    {
        tinyxml2::XMLElement* audio = doc.NewElement("AudioClips");
        game_node->InsertEndChild(audio);
        tinyxml2::XMLElement* audio_folder = doc.NewElement("AudioClipFolder");
        audio->InsertEndChild(audio_folder);

        std::unordered_map<int, const GameData::AudioClipInfo*> audio_map;
        for (const auto& a : game_data_->audio_clips)
            audio_map[a.id] = &a;

        WriteFolderTree(doc, audio_folder, game_data_->audio_clip_folders,
            "AudioClipFolder", "AudioClips",
            [&audio_map](tinyxml2::XMLDocument& d, tinyxml2::XMLElement* parent, int id) {
                auto it = audio_map.find(id);
                if (it == audio_map.end()) return;
                const auto& a = *it->second;
                tinyxml2::XMLElement* elem = d.NewElement("AudioClip");
                XmlWriteInt(d, elem, "ID", a.id);
                XmlWriteText(d, elem, "ScriptName", a.name.c_str());
                XmlWriteText(d, elem, "FileName", a.filename.c_str());
                if (!a.source_filename.empty())
                    XmlWriteText(d, elem, "SourceFileName", a.source_filename.c_str());
                XmlWriteInt(d, elem, "Type", a.type);
                XmlWriteInt(d, elem, "DefaultVolume", a.default_volume);
                XmlWriteInt(d, elem, "DefaultPriority", a.default_priority);
                XmlWriteInt(d, elem, "DefaultRepeat", a.default_repeat);
                XmlWriteInt(d, elem, "BundlingType", a.bundling_type);
                XmlWriteInt(d, elem, "FileType", a.file_type);
                if (!a.file_last_modified.empty())
                    XmlWriteText(d, elem, "FileLastModifiedDate", a.file_last_modified.c_str());
                parent->InsertEndChild(elem);
            });
    }

    // --- Audio Clip Types ---
    if (!game_data_->audio_clip_types.empty())
    {
        tinyxml2::XMLElement* act_node = doc.NewElement("AudioClipTypes");
        game_node->InsertEndChild(act_node);

        for (const auto& ct : game_data_->audio_clip_types)
        {
            tinyxml2::XMLElement* elem = doc.NewElement("AudioClipType");
            XmlWriteInt(doc, elem, "TypeID", ct.id);
            XmlWriteText(doc, elem, "Name", ct.name.c_str());
            XmlWriteInt(doc, elem, "MaxChannels", ct.max_channels);
            XmlWriteInt(doc, elem, "VolumeReductionWhileSpeechPlaying", ct.volume_reduction_while_speech);
            const char* crossfade_str = (ct.crossfade_speed == 1) ? "Slow" :
                                        (ct.crossfade_speed == 2) ? "Fast" : "No";
            XmlWriteText(doc, elem, "CrossfadeClips", crossfade_str);
            XmlWriteBool(doc, elem, "BackwardsCompatibilityType", ct.backwards_compat_type);
            act_node->InsertEndChild(elem);
        }
    }

    // --- Palette ---
    {
        tinyxml2::XMLElement* palette = doc.NewElement("Palette");
        game_node->InsertEndChild(palette);

        for (const auto& pe : game_data_->palette)
        {
            tinyxml2::XMLElement* elem = doc.NewElement("PaletteEntry");
            XmlWriteInt(doc, elem, "Red", pe.r);
            XmlWriteInt(doc, elem, "Green", pe.g);
            XmlWriteInt(doc, elem, "Blue", pe.b);
            const char* ct_str = "Background";
            if (pe.colour_type == 0) ct_str = "Gamewide";
            else if (pe.colour_type == 1) ct_str = "Locked";
            XmlWriteText(doc, elem, "ColourType", ct_str);
            palette->InsertEndChild(elem);
        }
    }

    // --- Global Variables ---
    if (!game_data_->global_variables.empty())
    {
        tinyxml2::XMLElement* gv_node = doc.NewElement("GlobalVariables");
        game_node->InsertEndChild(gv_node);

        for (const auto& gv : game_data_->global_variables)
        {
            tinyxml2::XMLElement* elem = doc.NewElement("GlobalVariable");
            XmlWriteText(doc, elem, "Name", gv.name.c_str());
            XmlWriteText(doc, elem, "Type", gv.type_name.c_str());
            XmlWriteText(doc, elem, "DefaultValue", gv.default_value.c_str());
            if (gv.array_type != 0)
            {
                XmlWriteInt(doc, elem, "ArrayType", gv.array_type);
                if (gv.array_type == 1) // static array needs size
                    XmlWriteInt(doc, elem, "ArraySize", gv.array_size);
            }
            gv_node->InsertEndChild(elem);
        }
    }

    // --- Global Messages (legacy AGS 2.x) ---
    {
        bool has_messages = false;
        for (int i = 0; i < GameData::NUM_GLOBAL_MESSAGES; i++)
        {
            if (!game_data_->global_messages[i].empty())
            {
                has_messages = true;
                break;
            }
        }
        if (has_messages)
        {
            tinyxml2::XMLElement* gm_node = doc.NewElement("GlobalMessages");
            game_node->InsertEndChild(gm_node);
            for (int i = 0; i < GameData::NUM_GLOBAL_MESSAGES; i++)
            {
                if (!game_data_->global_messages[i].empty())
                {
                    tinyxml2::XMLElement* msg = doc.NewElement("Message");
                    msg->SetAttribute("ID", i + GameData::GLOBAL_MESSAGE_ID_START);
                    msg->SetText(game_data_->global_messages[i].c_str());
                    gm_node->InsertEndChild(msg);
                }
            }
        }
    }

    // --- Text Parser ---
    if (!game_data_->text_parser_groups.empty())
    {
        tinyxml2::XMLElement* tp_node = doc.NewElement("TextParser");
        game_node->InsertEndChild(tp_node);

        for (const auto& grp : game_data_->text_parser_groups)
        {
            tinyxml2::XMLElement* grp_elem = doc.NewElement("WordGroup");
            XmlWriteInt(doc, grp_elem, "ID", grp.id);
            XmlWriteText(doc, grp_elem, "Name", grp.name.c_str());

            tinyxml2::XMLElement* words_elem = doc.NewElement("Words");
            for (const auto& w : grp.words)
            {
                tinyxml2::XMLElement* word_elem = doc.NewElement("Word");
                XmlWriteInt(doc, word_elem, "WordGroup", w.word_group);
                XmlWriteText(doc, word_elem, "Text", w.word.c_str());
                words_elem->InsertEndChild(word_elem);
            }
            grp_elem->InsertEndChild(words_elem);
            tp_node->InsertEndChild(grp_elem);
        }
    }

    // --- Lip Sync ---
    {
        tinyxml2::XMLElement* ls_node = doc.NewElement("LipSync");
        game_node->InsertEndChild(ls_node);

        const char* type_names[] = { "None", "Text", "PamelaVoiceFiles" };
        int type_idx = std::max(0, std::min(2, game_data_->lip_sync_type));
        XmlWriteText(doc, ls_node, "Type", type_names[type_idx]);
        XmlWriteInt(doc, ls_node, "DefaultFrame", game_data_->default_lipsync_frame);

        tinyxml2::XMLElement* frames_elem = doc.NewElement("Frames");
        for (int f = 0; f < GameData::kMaxLipSyncFrames; f++)
        {
            tinyxml2::XMLElement* cf = doc.NewElement("CharsForFrame");
            cf->SetText(game_data_->lip_sync_chars_per_frame[f].c_str());
            frames_elem->InsertEndChild(cf);
        }
        ls_node->InsertEndChild(frames_elem);
    }

    // --- Custom Properties ---
    if (!game_data_->custom_property_schemas.empty())
    {
        tinyxml2::XMLElement* cp_node = doc.NewElement("PropertyDefinitions");
        game_node->InsertEndChild(cp_node);

        for (const auto& p : game_data_->custom_property_schemas)
        {
            tinyxml2::XMLElement* elem = doc.NewElement("CustomPropertySchemaItem");
            XmlWriteText(doc, elem, "Name", p.name.c_str());
            XmlWriteText(doc, elem, "Description", p.description.c_str());
            XmlWriteText(doc, elem, "DefaultValue", p.default_value.c_str());
            // Type: 0=Boolean, 1=Number, 2=Text (String)
            const char* type_str = (p.type == 1) ? "Number" : (p.type == 2) ? "Text" : "Boolean";
            XmlWriteText(doc, elem, "Type", type_str);
            XmlWriteBool(doc, elem, "AppliesToCharacters", p.applies_to_characters);
            XmlWriteBool(doc, elem, "AppliesToHotspots", p.applies_to_hotspots);
            XmlWriteBool(doc, elem, "AppliesToObjects", p.applies_to_objects);
            XmlWriteBool(doc, elem, "AppliesToRooms", p.applies_to_rooms);
            XmlWriteBool(doc, elem, "AppliesToInvItems", p.applies_to_inv_items);
            XmlWriteBool(doc, elem, "Translated", p.translated);
            cp_node->InsertEndChild(elem);
        }
    }

    // --- Sprites (folder tree + source metadata) ---
    {
        // Helper: transparency enum to string
        auto transparency_str = [](GameData::SpriteImportTransparency t) -> const char* {
            switch (t) {
                case GameData::SpriteImportTransparency::PaletteIndex0: return "PaletteIndex0";
                case GameData::SpriteImportTransparency::TopLeft:       return "TopLeft";
                case GameData::SpriteImportTransparency::BottomLeft:    return "BottomLeft";
                case GameData::SpriteImportTransparency::TopRight:      return "TopRight";
                case GameData::SpriteImportTransparency::BottomRight:   return "BottomRight";
                case GameData::SpriteImportTransparency::NoTransparency:return "NoTransparency";
                case GameData::SpriteImportTransparency::PaletteIndex:  return "PaletteIndex";
                default: return "LeaveAsIs";
            }
        };

        // Build a lookup map from sprite ID -> SpriteInfo for fast access
        std::map<int, const GameData::SpriteInfo*> sprite_map;
        for (const auto& spr : game_data_->sprites)
            sprite_map[spr.id] = &spr;

        // Helper: write a single <Sprite> element
        auto write_sprite = [&](tinyxml2::XMLElement* parent, int slot_id) {
            auto it = sprite_map.find(slot_id);
            if (it == sprite_map.end()) return;
            const auto& spr = *it->second;

            tinyxml2::XMLElement* spr_elem = doc.NewElement("Sprite");
            spr_elem->SetAttribute("Slot", spr.id);
            spr_elem->SetAttribute("Width", spr.width);
            spr_elem->SetAttribute("Height", spr.height);
            spr_elem->SetAttribute("ColorDepth", spr.color_depth);
            spr_elem->SetAttribute("Resolution", spr.resolution.empty() ? "Real" : spr.resolution.c_str());
            spr_elem->SetAttribute("AlphaChannel", spr.alpha_channel ? "True" : "False");
            if (spr.colours_locked_to_room >= 0)
                spr_elem->SetAttribute("ColoursLockedToRoom", spr.colours_locked_to_room);

            // <Source> child element
            tinyxml2::XMLElement* src_elem = doc.NewElement("Source");
            XmlWriteText(doc, src_elem, "FileName", spr.source.source_file.c_str());
            XmlWriteInt(doc, src_elem, "OffsetX", spr.source.offset_x);
            XmlWriteInt(doc, src_elem, "OffsetY", spr.source.offset_y);
            XmlWriteInt(doc, src_elem, "ImportWidth", spr.source.import_width);
            XmlWriteInt(doc, src_elem, "ImportHeight", spr.source.import_height);
            XmlWriteBool(doc, src_elem, "ImportAsTile", spr.source.import_as_tile);
            XmlWriteInt(doc, src_elem, "Frame", spr.source.frame);
            XmlWriteBool(doc, src_elem, "RemapToGamePalette", spr.source.remap_to_game_palette);
            XmlWriteBool(doc, src_elem, "RemapToRoomPalette", spr.source.remap_to_room_palette);
            XmlWriteText(doc, src_elem, "ImportMethod", transparency_str(spr.source.transparency));
            XmlWriteInt(doc, src_elem, "TransparentColorIndex", spr.source.transparent_color_index);
            XmlWriteBool(doc, src_elem, "ImportAlphaChannel", spr.source.import_alpha_channel);
            spr_elem->InsertEndChild(src_elem);

            parent->InsertEndChild(spr_elem);
        };

        // Recursive lambda to write a sprite folder
        std::function<void(tinyxml2::XMLElement*, const GameData::SpriteFolderInfo&)>
            write_sprite_folder = [&](tinyxml2::XMLElement* parent,
                                      const GameData::SpriteFolderInfo& folder) {
            tinyxml2::XMLElement* folder_elem = doc.NewElement("SpriteFolder");
            folder_elem->SetAttribute("Name", folder.name.c_str());

            // SubFolders
            tinyxml2::XMLElement* subfolders_elem = doc.NewElement("SubFolders");
            for (const auto& sub : folder.subfolders)
                write_sprite_folder(subfolders_elem, sub);
            folder_elem->InsertEndChild(subfolders_elem);

            // Sprites in this folder
            tinyxml2::XMLElement* sprites_elem = doc.NewElement("Sprites");
            for (int slot_id : folder.item_ids)
                write_sprite(sprites_elem, slot_id);
            folder_elem->InsertEndChild(sprites_elem);

            parent->InsertEndChild(folder_elem);
        };

        tinyxml2::XMLElement* sprites_wrapper = doc.NewElement("Sprites");
        game_node->InsertEndChild(sprites_wrapper);

        // If we have a non-empty folder tree, use it
        if (!game_data_->root_sprite_folder.item_ids.empty() ||
            !game_data_->root_sprite_folder.subfolders.empty()) {
            write_sprite_folder(sprites_wrapper, game_data_->root_sprite_folder);
        } else {
            // No folder tree loaded (e.g., old project or imported binary).
            // Write all sprites into a single root folder.
            tinyxml2::XMLElement* root_folder = doc.NewElement("SpriteFolder");
            root_folder->SetAttribute("Name", "Main");
            tinyxml2::XMLElement* subfolders_elem = doc.NewElement("SubFolders");
            root_folder->InsertEndChild(subfolders_elem);
            tinyxml2::XMLElement* sprites_elem = doc.NewElement("Sprites");
            for (const auto& spr : game_data_->sprites)
                write_sprite(sprites_elem, spr.id);
            root_folder->InsertEndChild(sprites_elem);
            sprites_wrapper->InsertEndChild(root_folder);
        }
    }

    // Write the XML file
    tinyxml2::XMLError xml_err = doc.SaveFile(agf_path.c_str());
    if (xml_err != tinyxml2::XML_SUCCESS)
    {
        fprintf(stderr, "[Project] Failed to save AGF file '%s': %s\n",
                agf_path.c_str(), doc.ErrorStr());
        return false;
    }

    fprintf(stderr, "[Project] Saved AGF project '%s'.\n", agf_path.c_str());
    return true;
}

bool Project::SaveProject()
{
    if (!loaded_ || project_path_.empty())
        return false;

    // Determine save format from file extension
    std::string ext = fs::path(project_path_).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".agf")
    {
        if (!SaveToAGF(project_path_))
            return false;
    }
    else
    {
        // For non-AGF files (binary game files), save as AGF in the same directory
        std::string agf_path = project_dir_ + "/Game.agf";
        if (!SaveToAGF(agf_path))
            return false;
        project_path_ = agf_path;
    }

    dirty_ = false;
    return true;
}

bool Project::SaveProjectAs(const std::string& path)
{
    if (!loaded_)
        return false;

    project_path_ = path;
    UpdateProjectDir();
    return SaveProject();
}

void Project::CloseProject()
{
    loaded_ = false;
    dirty_ = false;
    project_path_.clear();
    project_dir_.clear();
    game_title_.clear();
    game_data_.reset();
    sprite_loader_.reset();
    room_loader_.reset();
    script_api_data_.reset();
}

} // namespace AGSEditor
