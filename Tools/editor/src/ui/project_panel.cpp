// AGS Editor ImGui - Project Panel implementation
#include "project_panel.h"
#include "editor_ui.h"
#include "log_panel.h"
#include "file_dialog.h"
#include "project/room_loader.h"
#include "panes/sprite_manager.h"
#include "panes/settings_pane.h"
#include "panes/character_editor.h"
#include "panes/dialog_editor.h"
#include "panes/view_editor.h"
#include "panes/gui_editor.h"
#include "panes/font_editor.h"
#include "panes/audio_manager.h"
#include "panes/inventory_editor.h"
#include "panes/cursor_editor.h"
#include "panes/translation_editor.h"
#include "panes/global_variables_editor.h"
#include "panes/text_parser_editor.h"
#include "panes/lip_sync_editor.h"
#include "panes/speech_pane.h"
#include "panes/default_setup_pane.h"
#include "panes/find_results_pane.h"
#include "panes/script_editor.h"
#include "app.h"
#include "project/project.h"
#include "project/game_data.h"
#include "core/dpi_helper.h"
#include "imgui.h"
#include "IconsLucide.h"

#include <cstdio>
#include <cstring>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <functional>
#include <sstream>
#include <unordered_map>

namespace AGSEditor
{

// Helper: returns Lucide icon if icon font loaded, otherwise fallback text
static const char* Icon(const char* lc, const char* fallback)
{
    return g_icons_loaded ? lc : fallback;
}

// Helper: recursively build ProjectTreeNode children from FolderInfo.
// 'make_label' maps an item ID to a display label string.
// 'next_id' is incremented for each new node.
// 'category' is the parent category name (e.g. "Characters").
template<typename LabelFn>
static void BuildFolderNodes(ProjectTreeNode& parent, const FolderInfo& folder,
                              int& next_id, const std::string& category, LabelFn make_label)
{
    // Sub-folders first
    for (const auto& sub : folder.subfolders)
    {
        ProjectTreeNode folder_node(sub.name, " ");
        folder_node.id = next_id++;
        folder_node.category = category;
        BuildFolderNodes(folder_node, sub, next_id, category, make_label);
        parent.children.push_back(folder_node);
    }
    // Items in this folder
    for (int item_id : folder.item_ids)
    {
        std::string label = make_label(item_id);
        if (label.empty()) continue; // item not found
        ProjectTreeNode item_node(label, " ", true);
        item_node.id = next_id++;
        item_node.user_data = item_id;
        item_node.category = category;
        parent.children.push_back(item_node);
    }
}

ProjectPanel::ProjectPanel(EditorUI& editor)
    : editor_(editor)
{
    BuildDefaultTree();
}

void ProjectPanel::BuildDefaultTree()
{
    root_ = ProjectTreeNode("Game", "", false);
    root_.id = next_id_++;

    // Welcome
    {
        ProjectTreeNode welcome("Welcome", Icon(ICON_LC_HOUSE, "[W]"), true);
        welcome.id = next_id_++;
        root_.children.push_back(welcome);
    }

    // General settings
    {
        ProjectTreeNode settings("General Settings", Icon(ICON_LC_SETTINGS, "[S]"), true);
        settings.id = next_id_++;
        root_.children.push_back(settings);
    }

    // Default Setup
    {
        ProjectTreeNode setup("Default Setup", Icon(ICON_LC_WRENCH, "[DS]"), true);
        setup.id = next_id_++;
        root_.children.push_back(setup);
    }

    // Scripts
    {
        ProjectTreeNode scripts("Scripts", Icon(ICON_LC_CODE, "[>]"));
        scripts.id = next_id_++;

        auto* project = editor_.GetApp().GetProject();
        GameData* gd = (project && project->IsLoaded()) ? project->GetGameData() : nullptr;
        if (gd && !gd->script_modules.empty())
        {
            for (size_t mi = 0; mi < gd->script_modules.size(); mi++)
            {
                const auto& sm = gd->script_modules[mi];
                // Create a branch node per module
                ProjectTreeNode mod_node(sm.name, Icon(ICON_LC_FILE_CODE, "[S]"));
                mod_node.id = next_id_++;
                mod_node.user_data = (int)mi; // module index

                if (!sm.header_file.empty())
                {
                    ProjectTreeNode hdr(sm.header_file, " ", true);
                    hdr.id = next_id_++;
                    mod_node.children.push_back(hdr);
                }
                if (!sm.script_file.empty())
                {
                    ProjectTreeNode scr(sm.script_file, " ", true);
                    scr.id = next_id_++;
                    mod_node.children.push_back(scr);
                }
                scripts.children.push_back(mod_node);
            }
        }
        else
        {
            // Fallback for no project loaded
            ProjectTreeNode gs_mod("GlobalScript", Icon(ICON_LC_FILE_CODE, "[S]"));
            gs_mod.id = next_id_++;
            ProjectTreeNode gh("GlobalScript.ash", " ", true);
            gh.id = next_id_++;
            ProjectTreeNode gs("GlobalScript.asc", " ", true);
            gs.id = next_id_++;
            gs_mod.children.push_back(gh);
            gs_mod.children.push_back(gs);
            scripts.children.push_back(gs_mod);
        }

        root_.children.push_back(scripts);
    }

    // Characters — populated from GameData with folder hierarchy
    {
        ProjectTreeNode characters("Characters", Icon(ICON_LC_USER, "[C]"));
        characters.id = next_id_++;

        auto* project = editor_.GetApp().GetProject();
        GameData* gd = (project && project->IsLoaded()) ? project->GetGameData() : nullptr;
        if (gd && !gd->characters.empty())
        {
            // Build ID lookup map
            std::unordered_map<int, const GameData::CharacterInfo*> char_map;
            for (const auto& ch : gd->characters) char_map[ch.id] = &ch;

            BuildFolderNodes(characters, gd->character_folders, next_id_, "Characters",
                [&](int id) -> std::string {
                    auto it = char_map.find(id);
                    if (it == char_map.end()) return {};
                    char buf[128];
                    snprintf(buf, sizeof(buf), "%d: %s (%s)", it->second->id,
                             it->second->real_name.c_str(), it->second->script_name.c_str());
                    return buf;
                });
        }

        root_.children.push_back(characters);
    }

    // Inventory Items — populated from GameData with folder hierarchy
    {
        ProjectTreeNode items("Inventory Items", Icon(ICON_LC_BOX, "[I]"));
        items.id = next_id_++;

        auto* project = editor_.GetApp().GetProject();
        GameData* gd = (project && project->IsLoaded()) ? project->GetGameData() : nullptr;
        if (gd && !gd->inventory_items.empty())
        {
            std::unordered_map<int, const GameData::InventoryItemInfo*> inv_map;
            for (const auto& inv : gd->inventory_items) inv_map[inv.id] = &inv;

            BuildFolderNodes(items, gd->inventory_folders, next_id_, "Inventory Items",
                [&](int id) -> std::string {
                    auto it = inv_map.find(id);
                    if (it == inv_map.end()) return {};
                    char buf[128];
                    snprintf(buf, sizeof(buf), "%d: %s", it->second->id,
                             it->second->description.c_str());
                    return buf;
                });
        }

        root_.children.push_back(items);
    }

    // Dialogs — populated from GameData with folder hierarchy
    {
        ProjectTreeNode dialogs("Dialogs", Icon(ICON_LC_MESSAGE_SQUARE, "[D]"));
        dialogs.id = next_id_++;

        auto* project = editor_.GetApp().GetProject();
        GameData* gd = (project && project->IsLoaded()) ? project->GetGameData() : nullptr;
        if (gd && !gd->dialogs.empty())
        {
            std::unordered_map<int, const GameData::DialogInfo*> dlg_map;
            for (const auto& dlg : gd->dialogs) dlg_map[dlg.id] = &dlg;

            BuildFolderNodes(dialogs, gd->dialog_folders, next_id_, "Dialogs",
                [&](int id) -> std::string {
                    auto it = dlg_map.find(id);
                    if (it == dlg_map.end()) return {};
                    char buf[128];
                    snprintf(buf, sizeof(buf), "%d: %s", it->second->id,
                             it->second->name.c_str());
                    return buf;
                });
        }

        root_.children.push_back(dialogs);
    }

    // Views — populated from GameData with folder hierarchy
    {
        ProjectTreeNode views("Views", Icon(ICON_LC_EYE, "[V]"));
        views.id = next_id_++;

        auto* project = editor_.GetApp().GetProject();
        GameData* gd = (project && project->IsLoaded()) ? project->GetGameData() : nullptr;
        if (gd && !gd->views.empty())
        {
            std::unordered_map<int, const GameData::ViewInfo*> view_map;
            for (const auto& v : gd->views) view_map[v.id] = &v;

            BuildFolderNodes(views, gd->view_folders, next_id_, "Views",
                [&](int id) -> std::string {
                    auto it = view_map.find(id);
                    if (it == view_map.end()) return {};
                    char buf[128];
                    snprintf(buf, sizeof(buf), "%d: %s (%d loops)", it->second->id,
                             it->second->name.c_str(), (int)it->second->loops.size());
                    return buf;
                });
        }

        root_.children.push_back(views);
    }

    // Sprites
    {
        ProjectTreeNode sprites("Sprites", Icon(ICON_LC_IMAGE, "[#]"), true);
        sprites.id = next_id_++;
        root_.children.push_back(sprites);
    }

    // Rooms — populated from GameData if available
    {
        ProjectTreeNode rooms("Rooms", Icon(ICON_LC_DOOR_OPEN, "[R]"));
        rooms.id = next_id_++;

        auto* project = editor_.GetProject();
        GameData* gd = (project && project->IsLoaded()) ? project->GetGameData() : nullptr;
        if (gd && !gd->rooms.empty())
        {
            for (const auto& r : gd->rooms)
            {
                char buf[128];
                if (r.description.empty())
                    snprintf(buf, sizeof(buf), "%d:", r.number);
                else
                    snprintf(buf, sizeof(buf), "%d: %s", r.number, r.description.c_str());
                ProjectTreeNode room_node(buf, " ", true);
                room_node.id = next_id_++;
                room_node.user_data = r.number;
                room_node.category = "Rooms";
                rooms.children.push_back(room_node);
            }
        }

        root_.children.push_back(rooms);
    }

    // Audio — populated from GameData with folder hierarchy
    {
        ProjectTreeNode audio("Audio", Icon(ICON_LC_MUSIC, "[A]"));
        audio.id = next_id_++;

        auto* project = editor_.GetApp().GetProject();
        GameData* gd = (project && project->IsLoaded()) ? project->GetGameData() : nullptr;
        if (gd && !gd->audio_clips.empty())
        {
            std::unordered_map<int, const GameData::AudioClipInfo*> clip_map;
            for (const auto& clip : gd->audio_clips) clip_map[clip.id] = &clip;

            BuildFolderNodes(audio, gd->audio_clip_folders, next_id_, "Audio",
                [&](int id) -> std::string {
                    auto it = clip_map.find(id);
                    if (it == clip_map.end()) return {};
                    char buf[128];
                    snprintf(buf, sizeof(buf), "%d: %s", it->second->id,
                             it->second->name.c_str());
                    return buf;
                });
        }
        else if (gd && !gd->audio_clip_types.empty())
        {
            // Fallback: type-based grouping
            for (const auto& ct : gd->audio_clip_types)
            {
                ProjectTreeNode type_node(ct.name, " ");
                type_node.id = next_id_++;
                audio.children.push_back(type_node);
            }
        }
        else
        {
            // Fallback: empty Music/Sounds branches
            ProjectTreeNode music("Music", " ");
            music.id = next_id_++;
            audio.children.push_back(music);

            ProjectTreeNode sounds("Sounds", " ");
            sounds.id = next_id_++;
            audio.children.push_back(sounds);
        }

        root_.children.push_back(audio);
    }

    // Fonts — populated from GameData if available
    {
        ProjectTreeNode fonts("Fonts", Icon(ICON_LC_TYPE, "[F]"));
        fonts.id = next_id_++;

        auto* project = editor_.GetApp().GetProject();
        GameData* gd = (project && project->IsLoaded()) ? project->GetGameData() : nullptr;
        if (gd && !gd->fonts.empty())
        {
            for (const auto& fnt : gd->fonts)
            {
                char buf[128];
                snprintf(buf, sizeof(buf), "%d: %s", fnt.id, fnt.name.c_str());
                ProjectTreeNode fnt_node(buf, " ", true);
                fnt_node.id = next_id_++;
                fnt_node.user_data = fnt.id;
                fonts.children.push_back(fnt_node);
            }
        }

        root_.children.push_back(fonts);
    }

    // GUIs — populated from GameData with folder hierarchy
    {
        ProjectTreeNode guis("GUIs", Icon(ICON_LC_MONITOR, "[G]"));
        guis.id = next_id_++;

        auto* project = editor_.GetApp().GetProject();
        GameData* gd = (project && project->IsLoaded()) ? project->GetGameData() : nullptr;
        if (gd && !gd->guis.empty())
        {
            std::unordered_map<int, const GameData::GUIInfo*> gui_map;
            for (const auto& gui : gd->guis) gui_map[gui.id] = &gui;

            BuildFolderNodes(guis, gd->gui_folders, next_id_, "GUIs",
                [&](int id) -> std::string {
                    auto it = gui_map.find(id);
                    if (it == gui_map.end()) return {};
                    char buf[128];
                    snprintf(buf, sizeof(buf), "%d: %s", it->second->id,
                             it->second->name.c_str());
                    return buf;
                });
        }

        root_.children.push_back(guis);
    }

    // Cursors — populated from GameData if available
    {
        ProjectTreeNode cursors("Mouse Cursors", Icon(ICON_LC_MOUSE_POINTER, "[M]"));
        cursors.id = next_id_++;

        auto* project = editor_.GetApp().GetProject();
        GameData* gd = (project && project->IsLoaded()) ? project->GetGameData() : nullptr;
        if (gd && !gd->cursors.empty())
        {
            for (const auto& cur : gd->cursors)
            {
                char buf[128];
                snprintf(buf, sizeof(buf), "%d: %s", cur.id, cur.name.c_str());
                ProjectTreeNode cur_node(buf, " ", true);
                cur_node.id = next_id_++;
                cur_node.user_data = cur.id;
                cursors.children.push_back(cur_node);
            }
        }

        root_.children.push_back(cursors);
    }

    // Translations
    {
        ProjectTreeNode translations("Translations", Icon(ICON_LC_LANGUAGES, "[T]"));
        translations.id = next_id_++;
        root_.children.push_back(translations);
    }

    // Speech (voice files)
    {
        ProjectTreeNode speech("Speech", Icon(ICON_LC_MIC, "[SP]"), true);
        speech.id = next_id_++;
        root_.children.push_back(speech);
    }

    // Lip Sync
    {
        ProjectTreeNode lip_sync("Lip Sync", Icon(ICON_LC_SMILE, "[LS]"), true);
        lip_sync.id = next_id_++;
        root_.children.push_back(lip_sync);
    }

    // Text Parser
    {
        ProjectTreeNode text_parser("Text Parser", Icon(ICON_LC_TERMINAL, "[TP]"), true);
        text_parser.id = next_id_++;
        root_.children.push_back(text_parser);
    }

    // Global Variables
    {
        ProjectTreeNode global_vars("Global Variables", Icon(ICON_LC_VARIABLE, "[GV]"), true);
        global_vars.id = next_id_++;
        root_.children.push_back(global_vars);
    }

    // Plugins
    {
        ProjectTreeNode plugins("Plugins", Icon(ICON_LC_PUZZLE, "[PL]"));
        plugins.id = next_id_++;
        root_.children.push_back(plugins);
    }
}

void ProjectPanel::RebuildTree()
{
    BuildDefaultTree();
}

void ProjectPanel::Draw()
{
    // Auto-rebuild tree when marked dirty by other components
    if (tree_dirty_)
    {
        RebuildTree();
        tree_dirty_ = false;
    }

    ImGui::Begin("Project", nullptr,
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse);

    // Clear pending drag state at start of frame
    pending_drag_source_id_ = -1;
    pending_drag_target_id_ = -1;

    ImGui::TextDisabled("Project Explorer");
    ImGui::Separator();

    DrawTreeNode(root_);

    // Delete Room confirmation dialog
    if (confirm_delete_room_)
    {
        ImGui::OpenPopup("Delete Room?");
        confirm_delete_room_ = false;
    }
    if (ImGui::BeginPopupModal("Delete Room?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        auto* project = editor_.GetApp().GetProject();
        auto* gd = (project && project->IsLoaded()) ? project->GetGameData() : nullptr;

        ImGui::Text("Delete Room %d?", delete_room_number_);
        ImGui::Spacing();

        // Check if any character starts in this room
        if (gd)
        {
            bool has_chars = false;
            for (const auto& ch : gd->characters)
            {
                if (ch.room == delete_room_number_)
                {
                    if (!has_chars)
                    {
                        ImGui::TextColored(ImVec4(1, 0.6f, 0.2f, 1), "Warning: Characters starting in this room:");
                        has_chars = true;
                    }
                    ImGui::BulletText("%s (%s)", ch.real_name.c_str(), ch.script_name.c_str());
                }
            }
            if (has_chars)
                ImGui::Spacing();
        }

        ImGui::Text("This will delete the room file. This cannot be undone.");
        ImGui::Separator();

        if (ImGui::Button("Delete", ImVec2(Dpi(80), 0)))
        {
            if (gd)
            {
                // Close any open room editor for this room
                editor_.ClosePaneByRoomNumber(delete_room_number_);

                // Delete the .crm file
                namespace fs = std::filesystem;
                std::string room_dir = project->GetProjectDir();
                char room_name[64];
                snprintf(room_name, sizeof(room_name), "room%d.crm", delete_room_number_);
                std::string room_path = room_dir + "/" + room_name;
                std::error_code ec;
                fs::remove(room_path, ec);

                // Remove from game data
                gd->rooms.erase(
                    std::remove_if(gd->rooms.begin(), gd->rooms.end(),
                        [this](const GameData::RoomInfo& r) { return r.number == delete_room_number_; }),
                    gd->rooms.end());

                RebuildTree();
                editor_.GetLogPanel().AddLog("[Project] Deleted room %d.", delete_room_number_);
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(Dpi(80), 0)))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }

    // Delete Script Module confirmation dialog
    if (confirm_delete_module_)
    {
        ImGui::OpenPopup("Delete Script Module?");
        confirm_delete_module_ = false;
    }
    if (ImGui::BeginPopupModal("Delete Script Module?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        auto* project = editor_.GetApp().GetProject();
        auto* gd = (project && project->IsLoaded()) ? project->GetGameData() : nullptr;

        if (gd && delete_module_index_ >= 0 && delete_module_index_ < (int)gd->script_modules.size())
        {
            auto& mod = gd->script_modules[delete_module_index_];
            ImGui::Text("Delete script module '%s'?", mod.name.c_str());
            ImGui::Spacing();
            ImGui::Text("This will delete both %s and %s.", mod.header_file.c_str(), mod.script_file.c_str());
            ImGui::Text("This cannot be undone.");
            ImGui::Separator();

            if (ImGui::Button("Delete", ImVec2(Dpi(80), 0)))
            {
                // Close open editors
                editor_.ClosePaneByScriptFilename(mod.header_file);
                editor_.ClosePaneByScriptFilename(mod.script_file);

                // Delete files from disk
                namespace fs = std::filesystem;
                std::string proj_dir = project->GetProjectDir();
                std::error_code ec;
                fs::remove(proj_dir + "/" + mod.header_file, ec);
                fs::remove(proj_dir + "/" + mod.script_file, ec);

                std::string deleted_name = mod.name;
                gd->script_modules.erase(gd->script_modules.begin() + delete_module_index_);

                RebuildTree();
                editor_.GetLogPanel().AddLog("[Project] Deleted script module '%s'.", deleted_name.c_str());

                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(Dpi(80), 0)))
                ImGui::CloseCurrentPopup();
        }
        else
        {
            ImGui::Text("Invalid module index.");
            if (ImGui::Button("OK"))
                ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    // "Go to..." navigation dialogs
    DrawGoToDialogs();

    ImGui::End();
}

void ProjectPanel::DrawTreeNode(ProjectTreeNode& node)
{
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

    if (node.is_leaf)
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

    if (selected_node_id_ == node.id)
        flags |= ImGuiTreeNodeFlags_Selected;

    // Root starts open
    if (node.id == 0)
        flags |= ImGuiTreeNodeFlags_DefaultOpen;

    // Check if this node is being renamed
    bool is_renaming = (renaming_node_id_ == node.id);
    std::string label = node.icon.empty() ? node.name : (node.icon + " " + node.name);

    if (is_renaming)
    {
        // Show inline InputText for renaming
        ImGui::PushID(node.id);
        std::string tree_label = node.icon.empty() ? "##rename" : (node.icon + " ##rename");
        ImGui::TreeNodeEx((void*)(intptr_t)node.id, flags | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen, "%s", tree_label.c_str());
        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (!rename_focus_set_)
        {
            ImGui::SetKeyboardFocusHere();
            rename_focus_set_ = true;
        }
        bool enter = ImGui::InputText("##InlineRename", rename_buffer_, sizeof(rename_buffer_),
            ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
        bool escape = ImGui::IsKeyPressed(ImGuiKey_Escape);
        bool lost_focus = !ImGui::IsItemActive() && rename_focus_set_ && ImGui::IsItemDeactivated();
        if (enter || lost_focus)
        {
            // Apply rename
            std::string new_name(rename_buffer_);
            if (!new_name.empty() && new_name != node.name)
            {
                node.name = new_name;
                editor_.GetLogPanel().AddLog("[Info] Renamed tree node to '%s'", new_name.c_str());
            }
            renaming_node_id_ = -1;
            rename_focus_set_ = false;
        }
        else if (escape)
        {
            renaming_node_id_ = -1;
            rename_focus_set_ = false;
        }
        ImGui::PopID();
        return; // Don't draw normal context menus etc. while renaming
    }

    bool is_open = ImGui::TreeNodeEx((void*)(intptr_t)node.id, flags, "%s", label.c_str());

    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
    {
        selected_node_id_ = node.id;
    }

    // F2 to start inline rename for selected leaf nodes
    if (selected_node_id_ == node.id && node.is_leaf && ImGui::IsKeyPressed(ImGuiKey_F2)
        && renaming_node_id_ < 0)
    {
        renaming_node_id_ = node.id;
        strncpy(rename_buffer_, node.name.c_str(), sizeof(rename_buffer_) - 1);
        rename_buffer_[sizeof(rename_buffer_) - 1] = '\0';
        rename_focus_set_ = false;
    }

    // Drag-and-drop source: allow leaf nodes to be dragged
    if (node.is_leaf && ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
    {
        int drag_id = node.id;
        ImGui::SetDragDropPayload("PROJECT_NODE", &drag_id, sizeof(int));
        ImGui::Text("Move: %s", node.name.c_str());
        ImGui::EndDragDropSource();
    }

    // Drag-and-drop target: allow dropping on leaf nodes (reorder within parent)
    if (node.is_leaf && ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PROJECT_NODE"))
        {
            int source_id = *(const int*)payload->Data;
            // Reordering is handled by the parent — store pending reorder
            pending_drag_source_id_ = source_id;
            pending_drag_target_id_ = node.id;
        }
        ImGui::EndDragDropTarget();
    }

    // Drag-and-drop target on folder nodes: accept drops into the folder
    if (!node.is_leaf && ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PROJECT_NODE"))
        {
            int source_id = *(const int*)payload->Data;
            pending_drag_source_id_ = source_id;
            pending_drag_target_id_ = node.id;
        }
        ImGui::EndDragDropTarget();
    }

    // Double-click to open appropriate editor
    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
    {
        if (node.name == "Welcome")
            editor_.OpenWelcomePane();
        else if (node.name == "General Settings")
            editor_.OpenOrFocusPane<SettingsPane>(editor_);
        else if (node.name == "Default Setup")
            editor_.OpenOrFocusPane<DefaultSetupPane>(editor_);
        else if (node.name == "Characters")
            editor_.OpenOrFocusPane<CharacterEditor>(editor_);
        else if (node.name == "Inventory Items")
            editor_.OpenOrFocusPane<InventoryEditor>(editor_);
        else if (node.name == "Dialogs")
            editor_.OpenOrFocusPane<DialogEditor>(editor_);
        else if (node.name == "Views")
            editor_.OpenOrFocusPane<ViewEditor>(editor_);
        else if (node.name == "Sprites")
            editor_.OpenOrFocusPane<SpriteManager>(editor_);
        else if (node.name == "Audio")
            editor_.OpenOrFocusPane<AudioManager>(editor_);
        else if (node.name == "Fonts")
            editor_.OpenOrFocusPane<FontEditor>(editor_);
        else if (node.name == "GUIs")
            editor_.OpenOrFocusPane<GUIEditor>(editor_);
        else if (node.name == "Mouse Cursors")
            editor_.OpenOrFocusPane<CursorEditor>(editor_);
        else if (node.name == "Translations")
            editor_.OpenOrFocusPane<TranslationEditor>(editor_);
        else if (node.name == "Lip Sync")
            editor_.OpenOrFocusPane<LipSyncEditor>(editor_);
        else if (node.name == "Text Parser")
            editor_.OpenOrFocusPane<TextParserEditor>(editor_);
        else if (node.name == "Global Variables")
            editor_.OpenOrFocusPane<GlobalVariablesEditor>(editor_);
        else if (node.name == "Speech")
            editor_.OpenOrFocusPane<SpeechPane>(editor_);
        else if (node.is_leaf && (node.name.find(".asc") != std::string::npos || node.name.find(".ash") != std::string::npos))
            editor_.OpenScriptFile(node.name);
        else if (!node.is_leaf && node.user_data >= 0)
        {
            // Non-leaf with user_data: could be a script module or audio folder branch
            auto* project = editor_.GetApp().GetProject();
            auto* gd = (project && project->IsLoaded()) ? project->GetGameData() : nullptr;

            // Check if this node is under Audio category (open audio manager)
            bool is_audio_node = false;
            for (const auto& cat : root_.children)
            {
                if (cat.name == "Audio")
                {
                    // Recursive check if node is under Audio
                    std::function<bool(const ProjectTreeNode&)> find_in =
                        [&](const ProjectTreeNode& n) -> bool {
                            if (n.id == node.id) return true;
                            for (const auto& c : n.children)
                                if (find_in(c)) return true;
                            return false;
                        };
                    is_audio_node = find_in(cat);
                    if (is_audio_node)
                        editor_.OpenOrFocusPane<AudioManager>(editor_);
                    break;
                }
            }

            // Otherwise, treat as script module branch node
            if (!is_audio_node && gd && node.user_data >= 0 &&
                node.user_data < (int)gd->script_modules.size())
            {
                auto& mod = gd->script_modules[node.user_data];
                if (!mod.script_file.empty())
                    editor_.OpenScriptFile(mod.script_file);
            }
        }
        else if (node.is_leaf && node.user_data >= 0)
        {
            // Determine parent category by recursively searching all root children
            bool handled = false;
            std::function<bool(const ProjectTreeNode&, const std::string&)> find_leaf =
                [&](const ProjectTreeNode& parent, const std::string& cat_name) -> bool {
                    for (const auto& child : parent.children)
                    {
                        if (child.id == node.id) return true;
                        if (find_leaf(child, cat_name)) return true;
                    }
                    return false;
                };

            for (const auto& cat : root_.children)
            {
                if (!find_leaf(cat, cat.name)) continue;

                if (cat.name == "Characters")
                    editor_.OpenOrFocusPane<CharacterEditor>(editor_);
                else if (cat.name == "Inventory Items")
                    editor_.OpenOrFocusPane<InventoryEditor>(editor_);
                else if (cat.name == "Dialogs")
                    editor_.OpenOrFocusPane<DialogEditor>(editor_);
                else if (cat.name == "Views")
                    editor_.OpenOrFocusPane<ViewEditor>(editor_);
                else if (cat.name == "Fonts")
                    editor_.OpenOrFocusPane<FontEditor>(editor_);
                else if (cat.name == "GUIs")
                    editor_.OpenOrFocusPane<GUIEditor>(editor_);
                else if (cat.name == "Mouse Cursors")
                    editor_.OpenOrFocusPane<CursorEditor>(editor_);
                else if (cat.name == "Audio")
                    editor_.OpenOrFocusPane<AudioManager>(editor_);
                else if (cat.name == "Rooms")
                    editor_.OpenRoomEditor(node.user_data);
                else
                    continue;
                handled = true;
                break;
            }
            if (!handled)
                editor_.OpenRoomEditor(node.user_data);
        }
    }

    // Context menu
    if (ImGui::BeginPopupContextItem())
    {
        ImGui::Text("%s", node.name.c_str());
        ImGui::Separator();
        if (!node.is_leaf)
            DrawCategoryContextMenu(node);
        if (node.is_leaf)
            DrawLeafContextMenu(node);
        ImGui::EndPopup();
    }

    if (is_open && !node.is_leaf)
    {
        for (auto& child : node.children)
        {
            DrawTreeNode(child);
        }

        // Apply pending reorder within this node's children
        if (pending_drag_source_id_ >= 0 && pending_drag_target_id_ >= 0)
        {
            int src_idx = -1, tgt_idx = -1;
            for (size_t i = 0; i < node.children.size(); i++)
            {
                if (node.children[i].id == pending_drag_source_id_) src_idx = (int)i;
                if (node.children[i].id == pending_drag_target_id_) tgt_idx = (int)i;
            }
            if (src_idx >= 0 && tgt_idx >= 0 && src_idx != tgt_idx)
            {
                auto moved = std::move(node.children[src_idx]);
                node.children.erase(node.children.begin() + src_idx);
                // Adjust target index after removal
                if (tgt_idx > src_idx) tgt_idx--;
                node.children.insert(node.children.begin() + tgt_idx, std::move(moved));
                pending_drag_source_id_ = -1;
                pending_drag_target_id_ = -1;
            }
        }

        ImGui::TreePop();
    }
}

// -----------------------------------------------------------------------
// Category (non-leaf) context menus
// -----------------------------------------------------------------------
void ProjectPanel::DrawCategoryContextMenu(ProjectTreeNode& node)
{
    auto* project = editor_.GetApp().GetProject();
    auto* gd = (project && project->IsLoaded()) ? project->GetGameData() : nullptr;

    if (node.name == "Characters")
    {
        if (ImGui::MenuItem("Open Character Editor"))
            editor_.OpenOrFocusPane<CharacterEditor>(editor_);
        if (gd && ImGui::MenuItem("New Character"))
        {
            GameData::CharacterInfo ch{};
            ch.id = (int)gd->characters.size();
            ch.script_name = "cChar" + std::to_string(ch.id);
            ch.real_name = "Character " + std::to_string(ch.id);
            ch.room = 0;
            gd->characters.push_back(ch);
            RebuildTree();
            editor_.GetLogPanel().AddLog("[Project] Created character '%s'.", ch.script_name.c_str());
        }
        ImGui::Separator();
        if (gd && ImGui::MenuItem("Go to Character...", "Ctrl+G"))
        {
            goto_category_ = "Characters";
            goto_filter_[0] = '\0';
            goto_selected_id_ = -1;
            show_goto_dialog_ = true;
        }
    }
    else if (node.name == "Dialogs")
    {
        if (ImGui::MenuItem("Open Dialog Editor"))
            editor_.OpenOrFocusPane<DialogEditor>(editor_);
        if (gd && ImGui::MenuItem("New Dialog"))
        {
            GameData::DialogInfo dlg{};
            dlg.id = (int)gd->dialogs.size();
            dlg.name = "dDialog" + std::to_string(dlg.id);
            gd->dialogs.push_back(dlg);
            RebuildTree();
            editor_.GetLogPanel().AddLog("[Project] Created dialog '%s'.", dlg.name.c_str());
        }
        ImGui::Separator();
        if (gd && ImGui::MenuItem("Go to Dialog...", "Ctrl+G"))
        {
            goto_category_ = "Dialogs";
            goto_filter_[0] = '\0';
            goto_selected_id_ = -1;
            show_goto_dialog_ = true;
        }
    }
    else if (node.name == "Views")
    {
        if (ImGui::MenuItem("Open View Editor"))
            editor_.OpenOrFocusPane<ViewEditor>(editor_);
        if (gd && ImGui::MenuItem("New View"))
        {
            GameData::ViewInfo v;
            int max_id = -1;
            for (auto& ev : gd->views)
                if (ev.id > max_id) max_id = ev.id;
            v.id = max_id + 1;
            v.name = "VIEW" + std::to_string(v.id);
            v.loop_count = 1;
            GameData::LoopData loop;
            GameData::FrameData f{};
            loop.frames.push_back(f);
            v.loops.push_back(loop);
            gd->views.push_back(v);
            RebuildTree();
            editor_.GetLogPanel().AddLog("[Project] Created view '%s'.", v.name.c_str());
        }
        ImGui::Separator();
        if (gd && ImGui::MenuItem("Go to View...", "Ctrl+G"))
        {
            goto_category_ = "Views";
            goto_filter_[0] = '\0';
            goto_selected_id_ = -1;
            show_goto_dialog_ = true;
        }
    }
    else if (node.name == "Inventory Items")
    {
        if (ImGui::MenuItem("Open Inventory Editor"))
            editor_.OpenOrFocusPane<InventoryEditor>(editor_);
        if (gd && ImGui::MenuItem("New Inventory Item"))
        {
            GameData::InventoryItemInfo item{};
            item.id = (int)gd->inventory_items.size();
            item.script_name = "iInvItem" + std::to_string(item.id);
            item.description = "New Item";
            gd->inventory_items.push_back(item);
            RebuildTree();
            editor_.GetLogPanel().AddLog("[Project] Created inventory item '%s'.", item.script_name.c_str());
        }
        ImGui::Separator();
        if (gd && ImGui::MenuItem("Go to Inventory Item...", "Ctrl+G"))
        {
            goto_category_ = "Inventory Items";
            goto_filter_[0] = '\0';
            goto_selected_id_ = -1;
            show_goto_dialog_ = true;
        }
    }
    else if (node.name == "Fonts")
    {
        if (ImGui::MenuItem("Open Font Editor"))
            editor_.OpenOrFocusPane<FontEditor>(editor_);
    }
    else if (node.name == "GUIs")
    {
        if (ImGui::MenuItem("Open GUI Editor"))
            editor_.OpenOrFocusPane<GUIEditor>(editor_);
        ImGui::Separator();
        if (gd && ImGui::MenuItem("Go to GUI...", "Ctrl+G"))
        {
            goto_category_ = "GUIs";
            goto_filter_[0] = '\0';
            goto_selected_id_ = -1;
            show_goto_dialog_ = true;
        }
    }
    else if (node.name == "Mouse Cursors")
    {
        if (ImGui::MenuItem("Open Cursor Editor"))
            editor_.OpenOrFocusPane<CursorEditor>(editor_);
    }
    else if (node.name == "Rooms")
    {
        if (gd && ImGui::MenuItem("New Room"))
        {
            // Find next available room number
            int new_num = 0;
            for (const auto& r : gd->rooms)
                if (r.number >= new_num) new_num = r.number + 1;

            GameData::RoomInfo ri;
            ri.number = new_num;
            ri.description = "";
            gd->rooms.push_back(ri);

            std::sort(gd->rooms.begin(), gd->rooms.end(),
                [](const GameData::RoomInfo& a, const GameData::RoomInfo& b) {
                    return a.number < b.number;
                });

            auto* loader = project->GetRoomLoader();
            if (loader)
            {
                RoomData default_room;
                default_room.width = 320;
                default_room.height = 200;
                default_room.mask_resolution = 1;
                loader->SaveRoom(new_num, default_room);
            }

            RebuildTree();
            editor_.GetLogPanel().AddLog("[Project] Created room %d.", new_num);
            editor_.OpenRoomEditor(new_num);
        }
        if (gd && ImGui::MenuItem("Import Existing Room..."))
        {
            ImportRoom();
        }
        ImGui::Separator();
        if (gd && ImGui::MenuItem("Sort Rooms by Number"))
        {
            std::sort(gd->rooms.begin(), gd->rooms.end(),
                [](const GameData::RoomInfo& a, const GameData::RoomInfo& b) {
                    return a.number < b.number;
                });
            RebuildTree();
            editor_.GetLogPanel().AddLog("[Project] Rooms sorted by number.");
        }
        if (gd && ImGui::MenuItem("Go to Room...", "Ctrl+G"))
        {
            goto_category_ = "Rooms";
            goto_filter_[0] = '\0';
            goto_selected_id_ = -1;
            show_goto_dialog_ = true;
        }
    }
    else if (node.name == "Sprites")
    {
        if (ImGui::MenuItem("Open Sprite Manager"))
            editor_.OpenOrFocusPane<SpriteManager>(editor_);
    }
    else if (node.name == "Translations")
    {
        if (ImGui::MenuItem("Open Translation Editor"))
            editor_.OpenOrFocusPane<TranslationEditor>(editor_);
    }
    else if (node.name == "Audio")
    {
        if (ImGui::MenuItem("Open Audio Manager"))
            editor_.OpenOrFocusPane<AudioManager>(editor_);
        ImGui::Separator();
        if (gd && ImGui::MenuItem("Go to Audio Clip...", "Ctrl+G"))
        {
            goto_category_ = "Audio";
            goto_filter_[0] = '\0';
            goto_selected_id_ = -1;
            show_goto_dialog_ = true;
        }
    }
    else if (node.name == "Scripts")
    {
        if (gd && ImGui::MenuItem("New Script Module"))
        {
            std::string base_name = "Module";
            int counter = 1;
            std::string new_name;
            bool unique = false;
            while (!unique)
            {
                new_name = base_name + std::to_string(counter);
                unique = true;
                for (const auto& sm : gd->script_modules)
                {
                    if (sm.name == new_name)
                    {
                        unique = false;
                        counter++;
                        break;
                    }
                }
            }

            GameData::ScriptModule mod;
            mod.name = new_name;
            mod.header_file = new_name + ".ash";
            mod.script_file = new_name + ".asc";
            gd->script_modules.push_back(mod);

            std::string proj_dir = project->GetProjectDir();
            {
                std::string hdr_path = proj_dir + "/" + mod.header_file;
                std::ofstream hf(hdr_path);
                hf << "// " << new_name << " header\n";
            }
            {
                std::string scr_path = proj_dir + "/" + mod.script_file;
                std::ofstream sf(scr_path);
                sf << "// " << new_name << " script\n";
            }

            RebuildTree();
            editor_.GetLogPanel().AddLog("[Project] Created script module '%s'.", new_name.c_str());
        }
        if (gd && ImGui::MenuItem("Import Script Module..."))
        {
            ImportScriptModule();
        }
    }
    else if (gd && node.user_data >= 0)
    {
        // Script module branch node
        int mod_idx = node.user_data;
        if (mod_idx < (int)gd->script_modules.size())
        {
            auto& mod = gd->script_modules[mod_idx];
            bool is_global = (mod.name == "GlobalScript");

            if (ImGui::MenuItem("Open Header"))
                editor_.OpenScriptFile(mod.header_file);
            if (ImGui::MenuItem("Open Script"))
                editor_.OpenScriptFile(mod.script_file);

            ImGui::Separator();
            if (!is_global && ImGui::MenuItem("Export Module..."))
            {
                ExportScriptModule(mod_idx);
            }
            if (!is_global && ImGui::MenuItem("Delete Module"))
            {
                confirm_delete_module_ = true;
                delete_module_index_ = mod_idx;
            }
            ImGui::Separator();
            if (mod_idx > 0 && ImGui::MenuItem("Move Up"))
            {
                std::swap(gd->script_modules[mod_idx], gd->script_modules[mod_idx - 1]);
                RebuildTree();
            }
            if (mod_idx < (int)gd->script_modules.size() - 1 && ImGui::MenuItem("Move Down"))
            {
                std::swap(gd->script_modules[mod_idx], gd->script_modules[mod_idx + 1]);
                RebuildTree();
            }
        }
    }
}

// -----------------------------------------------------------------------
// Leaf node context menus
// -----------------------------------------------------------------------
void ProjectPanel::DrawLeafContextMenu(ProjectTreeNode& node)
{
    auto* project = editor_.GetApp().GetProject();
    auto* gd = (project && project->IsLoaded()) ? project->GetGameData() : nullptr;

    if (ImGui::MenuItem("Rename", "F2"))
    {
        renaming_node_id_ = node.id;
        strncpy(rename_buffer_, node.name.c_str(), sizeof(rename_buffer_) - 1);
        rename_buffer_[sizeof(rename_buffer_) - 1] = '\0';
        rename_focus_set_ = false;
    }
    ImGui::Separator();

    // Determine which category this leaf belongs to
    std::string cat = node.category;

    if (node.name == "General Settings")
    {
        if (ImGui::MenuItem("Open Settings"))
            editor_.OpenOrFocusPane<SettingsPane>(editor_);
    }
    else if (node.name.find(".asc") != std::string::npos ||
             node.name.find(".ash") != std::string::npos)
    {
        if (ImGui::MenuItem("Open Script"))
            editor_.OpenScriptFile(node.name);
    }
    else if (cat == "Rooms" && node.user_data >= 0)
    {
        if (ImGui::MenuItem("Edit this Room"))
            editor_.OpenRoomEditor(node.user_data);
        ImGui::Separator();
        if (ImGui::MenuItem("Delete Room"))
        {
            confirm_delete_room_ = true;
            delete_room_number_ = node.user_data;
        }
    }
    else if (cat == "Characters" && gd && node.user_data >= 0)
    {
        if (ImGui::MenuItem("Edit Character"))
            editor_.OpenOrFocusPane<CharacterEditor>(editor_);
        // Find the character's script name for Find All Usages
        for (const auto& ch : gd->characters)
        {
            if (ch.id == node.user_data)
            {
                ImGui::Separator();
                if (ImGui::MenuItem("Find All Usages"))
                    FindAllUsagesOf(ch.script_name);
                break;
            }
        }
    }
    else if (cat == "Views" && gd && node.user_data >= 0)
    {
        if (ImGui::MenuItem("Edit View"))
            editor_.OpenOrFocusPane<ViewEditor>(editor_);
        // Find the view's name for Find All Usages
        for (const auto& v : gd->views)
        {
            if (v.id == node.user_data)
            {
                ImGui::Separator();
                if (ImGui::MenuItem("Find All Usages"))
                    FindAllUsagesOf(v.name);
                break;
            }
        }
    }
    else if (cat == "Inventory Items" && gd && node.user_data >= 0)
    {
        if (ImGui::MenuItem("Edit Inventory Item"))
            editor_.OpenOrFocusPane<InventoryEditor>(editor_);
        for (const auto& inv : gd->inventory_items)
        {
            if (inv.id == node.user_data)
            {
                ImGui::Separator();
                if (ImGui::MenuItem("Find All Usages"))
                    FindAllUsagesOf(inv.script_name);
                break;
            }
        }
    }
    else if (cat == "Dialogs" && gd && node.user_data >= 0)
    {
        if (ImGui::MenuItem("Edit Dialog"))
            editor_.OpenOrFocusPane<DialogEditor>(editor_);
        for (const auto& d : gd->dialogs)
        {
            if (d.id == node.user_data)
            {
                ImGui::Separator();
                if (ImGui::MenuItem("Find All Usages"))
                    FindAllUsagesOf(d.name);
                break;
            }
        }
    }
    else if (cat == "GUIs" && gd && node.user_data >= 0)
    {
        if (ImGui::MenuItem("Edit GUI"))
            editor_.OpenOrFocusPane<GUIEditor>(editor_);
        for (const auto& gui : gd->guis)
        {
            if (gui.id == node.user_data)
            {
                ImGui::Separator();
                if (ImGui::MenuItem("Find All Usages"))
                    FindAllUsagesOf(gui.name);
                break;
            }
        }
    }
    else if (cat == "Audio" && gd && node.user_data >= 0)
    {
        if (ImGui::MenuItem("Open Audio Manager"))
            editor_.OpenOrFocusPane<AudioManager>(editor_);
        for (const auto& clip : gd->audio_clips)
        {
            if (clip.id == node.user_data)
            {
                ImGui::Separator();
                if (ImGui::MenuItem("Find All Usages"))
                    FindAllUsagesOf(clip.name);
                break;
            }
        }
    }
    else if (node.user_data >= 0)
    {
        // Fallback: try opening as room
        if (ImGui::MenuItem("Open"))
            editor_.OpenRoomEditor(node.user_data);
    }
}

// -----------------------------------------------------------------------
// "Go to..." navigation dialog
// -----------------------------------------------------------------------
void ProjectPanel::DrawGoToDialogs()
{
    if (show_goto_dialog_)
    {
        std::string popup_title = "Go to " + goto_category_ + "...";
        ImGui::OpenPopup(popup_title.c_str());
        show_goto_dialog_ = false;
    }

    // Render all possible "Go to..." popups
    static const char* categories[] = {
        "Rooms", "Characters", "Views", "Dialogs",
        "Inventory Items", "GUIs", "Audio"
    };
    for (const char* cat : categories)
    {
        std::string popup_title = std::string("Go to ") + cat + "...";
        if (!ImGui::BeginPopupModal(popup_title.c_str(), nullptr,
            ImGuiWindowFlags_AlwaysAutoResize))
            continue;

        auto* project = editor_.GetApp().GetProject();
        auto* gd = (project && project->IsLoaded()) ? project->GetGameData() : nullptr;
        if (!gd) { ImGui::CloseCurrentPopup(); ImGui::EndPopup(); continue; }

        ImGui::Text("Type to filter:");
        ImGui::SetNextItemWidth(Dpi(300));
        ImGui::InputText("##GoToFilter", goto_filter_,
            sizeof(goto_filter_), ImGuiInputTextFlags_AutoSelectAll);

        // Build list of matching items
        struct GoToItem { int id; std::string label; };
        std::vector<GoToItem> items;
        std::string filter_str(goto_filter_);

        // Convert filter to lowercase for case-insensitive matching
        std::string filter_lower = filter_str;
        for (auto& c : filter_lower) c = (char)std::tolower((unsigned char)c);

        auto matches_filter = [&](const std::string& text) -> bool {
            if (filter_lower.empty()) return true;
            std::string lower = text;
            for (auto& c : lower) c = (char)std::tolower((unsigned char)c);
            return lower.find(filter_lower) != std::string::npos;
        };

        std::string cat_str(cat);
        if (cat_str == "Rooms")
        {
            for (const auto& r : gd->rooms)
            {
                char buf[128];
                snprintf(buf, sizeof(buf), "%d: %s", r.number, r.description.c_str());
                std::string label(buf);
                if (matches_filter(label))
                    items.push_back({r.number, label});
            }
        }
        else if (cat_str == "Characters")
        {
            for (const auto& ch : gd->characters)
            {
                char buf[128];
                snprintf(buf, sizeof(buf), "%d: %s (%s)", ch.id,
                         ch.real_name.c_str(), ch.script_name.c_str());
                std::string label(buf);
                if (matches_filter(label))
                    items.push_back({ch.id, label});
            }
        }
        else if (cat_str == "Views")
        {
            for (const auto& v : gd->views)
            {
                char buf[128];
                snprintf(buf, sizeof(buf), "%d: %s", v.id, v.name.c_str());
                std::string label(buf);
                if (matches_filter(label))
                    items.push_back({v.id, label});
            }
        }
        else if (cat_str == "Dialogs")
        {
            for (const auto& d : gd->dialogs)
            {
                char buf[128];
                snprintf(buf, sizeof(buf), "%d: %s", d.id, d.name.c_str());
                std::string label(buf);
                if (matches_filter(label))
                    items.push_back({d.id, label});
            }
        }
        else if (cat_str == "Inventory Items")
        {
            for (const auto& inv : gd->inventory_items)
            {
                char buf[128];
                snprintf(buf, sizeof(buf), "%d: %s", inv.id, inv.description.c_str());
                std::string label(buf);
                if (matches_filter(label))
                    items.push_back({inv.id, label});
            }
        }
        else if (cat_str == "GUIs")
        {
            for (const auto& gui : gd->guis)
            {
                char buf[128];
                snprintf(buf, sizeof(buf), "%d: %s", gui.id, gui.name.c_str());
                std::string label(buf);
                if (matches_filter(label))
                    items.push_back({gui.id, label});
            }
        }
        else if (cat_str == "Audio")
        {
            for (const auto& clip : gd->audio_clips)
            {
                char buf[128];
                snprintf(buf, sizeof(buf), "%d: %s", clip.id, clip.name.c_str());
                std::string label(buf);
                if (matches_filter(label))
                    items.push_back({clip.id, label});
            }
        }

        // Show scrollable list
        ImGui::Text("%d item(s):", (int)items.size());
        if (ImGui::BeginChild("##GoToList", ImVec2(Dpi(300), Dpi(250)), ImGuiChildFlags_Borders))
        {
            for (const auto& item : items)
            {
                bool selected = (goto_selected_id_ == item.id);
                if (ImGui::Selectable(item.label.c_str(), selected,
                    ImGuiSelectableFlags_AllowDoubleClick))
                {
                    goto_selected_id_ = item.id;
                    if (ImGui::IsMouseDoubleClicked(0))
                    {
                        // Navigate on double-click
                        if (cat_str == "Rooms")
                            editor_.OpenRoomEditor(item.id);
                        else if (cat_str == "Characters")
                            editor_.OpenOrFocusPane<CharacterEditor>(editor_);
                        else if (cat_str == "Views")
                            editor_.OpenOrFocusPane<ViewEditor>(editor_);
                        else if (cat_str == "Dialogs")
                            editor_.OpenOrFocusPane<DialogEditor>(editor_);
                        else if (cat_str == "Inventory Items")
                            editor_.OpenOrFocusPane<InventoryEditor>(editor_);
                        else if (cat_str == "GUIs")
                            editor_.OpenOrFocusPane<GUIEditor>(editor_);
                        else if (cat_str == "Audio")
                            editor_.OpenOrFocusPane<AudioManager>(editor_);
                        ImGui::CloseCurrentPopup();
                    }
                }
            }
        }
        ImGui::EndChild();

        // OK / Cancel buttons
        bool go = ImGui::Button("Go", ImVec2(Dpi(80), 0));
        ImGui::SameLine();
        bool cancel = ImGui::Button("Cancel", ImVec2(Dpi(80), 0));
        if (ImGui::IsKeyPressed(ImGuiKey_Escape))
            cancel = true;
        if (ImGui::IsKeyPressed(ImGuiKey_Enter) && goto_selected_id_ >= 0)
            go = true;

        if (go && goto_selected_id_ >= 0)
        {
            if (cat_str == "Rooms")
                editor_.OpenRoomEditor(goto_selected_id_);
            else if (cat_str == "Characters")
                editor_.OpenOrFocusPane<CharacterEditor>(editor_);
            else if (cat_str == "Views")
                editor_.OpenOrFocusPane<ViewEditor>(editor_);
            else if (cat_str == "Dialogs")
                editor_.OpenOrFocusPane<DialogEditor>(editor_);
            else if (cat_str == "Inventory Items")
                editor_.OpenOrFocusPane<InventoryEditor>(editor_);
            else if (cat_str == "GUIs")
                editor_.OpenOrFocusPane<GUIEditor>(editor_);
            else if (cat_str == "Audio")
                editor_.OpenOrFocusPane<AudioManager>(editor_);
            ImGui::CloseCurrentPopup();
        }
        if (cancel)
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }
}

// -----------------------------------------------------------------------
// Find All Usages - searches all project scripts for a given identifier
// -----------------------------------------------------------------------
void ProjectPanel::FindAllUsagesOf(const std::string& script_name)
{
    auto* project = editor_.GetApp().GetProject();
    if (!project || !project->IsLoaded())
        return;

    GameData* gd = project->GetGameData();
    if (!gd)
        return;

    namespace fs = std::filesystem;
    std::string project_dir = project->GetProjectDir();
    std::vector<FindResult> results;

    // Helper: search a single script text for whole-word matches
    auto search_text = [&](const std::string& filename,
                           const std::string& content)
    {
        std::istringstream stream(content);
        std::string line;
        int line_num = 0;
        while (std::getline(stream, line))
        {
            line_num++;
            size_t pos = 0;
            while ((pos = line.find(script_name, pos)) != std::string::npos)
            {
                // Whole-word check
                bool word_start = (pos == 0) ||
                    (!std::isalnum((unsigned char)line[pos - 1]) &&
                     line[pos - 1] != '_');
                size_t end_pos = pos + script_name.size();
                bool word_end = (end_pos >= line.size()) ||
                    (!std::isalnum((unsigned char)line[end_pos]) &&
                     line[end_pos] != '_');
                if (word_start && word_end)
                {
                    FindResult r;
                    r.filename = filename;
                    r.line = line_num;
                    r.line_text = line;
                    r.char_index = (int)pos;
                    results.push_back(r);
                }
                pos = end_pos;
            }
        }
    };

    // Helper: read a file and search it
    auto search_file = [&](const std::string& filename)
    {
        std::string filepath = (fs::path(project_dir) / filename).string();
        std::ifstream file(filepath);
        if (!file.is_open())
            return;
        std::ostringstream ss;
        ss << file.rdbuf();
        search_text(filename, ss.str());
    };

    // Search all script module headers and bodies
    for (const auto& mod : gd->script_modules)
    {
        if (!mod.header_file.empty())
            search_file(mod.header_file);
        if (!mod.script_file.empty())
            search_file(mod.script_file);
    }

    // Search room scripts
    for (const auto& room : gd->rooms)
    {
        char room_script[64];
        snprintf(room_script, sizeof(room_script), "room%d.asc", room.number);
        std::string room_path = (fs::path(project_dir) / room_script).string();
        if (fs::exists(room_path))
            search_file(room_script);
    }

    // Search dialog scripts (in-memory)
    for (const auto& d : gd->dialogs)
    {
        if (!d.script.empty())
        {
            std::string dlg_name = "Dialog " + std::to_string(d.id);
            search_text(dlg_name, d.script);
        }
    }

    editor_.GetLogPanel().AddLog("[Find] '%s': %d result(s) across project scripts.",
        script_name.c_str(), (int)results.size());

    // Show results in FindResultsPane
    auto* pane = editor_.OpenOrFocusPane<FindResultsPane>(editor_);
    pane->SetResults(script_name, std::move(results));
    pane->SetNavigateCallback([this](const std::string& file, int line) {
        editor_.OpenScriptFile(file);
        for (auto& p : editor_.GetPanes())
        {
            auto* se = dynamic_cast<ScriptEditor*>(p.get());
            if (se && se->GetFilename() == file)
            {
                se->GoToLine(line);
                break;
            }
        }
    });
}

// -----------------------------------------------------------------------
// Import a .crm room file into the project
// -----------------------------------------------------------------------
void ProjectPanel::ImportRoom()
{
    auto* project = editor_.GetApp().GetProject();
    if (!project || !project->IsLoaded())
        return;

    FileDialog::Open(FileDialogType::OpenFile,
        "Import Room File",
        ".crm",
        project->GetProjectDir(),
        [this](const std::string& path) {
            auto* proj = editor_.GetApp().GetProject();
            if (!proj || !proj->IsLoaded()) return;
            auto* gd = proj->GetGameData();
            auto* loader = proj->GetRoomLoader();
            if (!gd || !loader) return;

            int new_num = loader->ImportRoom(path);
            if (new_num < 0)
            {
                editor_.GetLogPanel().AddLog("[Error] Failed to import room from '%s'.",
                    path.c_str());
                return;
            }

            // Add to game data if not already there
            bool found = false;
            for (const auto& r : gd->rooms)
            {
                if (r.number == new_num) { found = true; break; }
            }
            if (!found)
            {
                GameData::RoomInfo ri;
                ri.number = new_num;
                ri.description = "";
                gd->rooms.push_back(ri);
                std::sort(gd->rooms.begin(), gd->rooms.end(),
                    [](const GameData::RoomInfo& a, const GameData::RoomInfo& b) {
                        return a.number < b.number;
                    });
            }

            RebuildTree();
            editor_.GetLogPanel().AddLog("[Project] Imported room as room %d.", new_num);
            editor_.OpenRoomEditor(new_num);
        });
}

// -----------------------------------------------------------------------
// Import a script module (.scm or .ash+.asc pair)
// -----------------------------------------------------------------------
void ProjectPanel::ImportScriptModule()
{
    auto* project = editor_.GetApp().GetProject();
    if (!project || !project->IsLoaded())
        return;

    FileDialog::Open(FileDialogType::OpenFile,
        "Import Script Module",
        ".ash,.asc,.scm",
        project->GetProjectDir(),
        [this](const std::string& path) {
            auto* proj = editor_.GetApp().GetProject();
            if (!proj || !proj->IsLoaded()) return;
            auto* gd = proj->GetGameData();
            if (!gd) return;

            namespace fs = std::filesystem;
            fs::path import_path(path);
            std::string ext = import_path.extension().string();
            std::string stem = import_path.stem().string();
            std::string proj_dir = proj->GetProjectDir();

            // Check for name conflict
            for (const auto& sm : gd->script_modules)
            {
                if (sm.name == stem)
                {
                    editor_.GetLogPanel().AddLog(
                        "[Error] Script module '%s' already exists.", stem.c_str());
                    return;
                }
            }

            GameData::ScriptModule mod;
            mod.name = stem;
            mod.header_file = stem + ".ash";
            mod.script_file = stem + ".asc";

            if (ext == ".scm")
            {
                // .scm is a combined file: read and split at a marker or just
                // import as the script body with an empty header
                std::ifstream in(path);
                if (!in.is_open())
                {
                    editor_.GetLogPanel().AddLog(
                        "[Error] Cannot open '%s'.", path.c_str());
                    return;
                }
                std::ostringstream ss;
                ss << in.rdbuf();
                std::string content = ss.str();

                // Write header (empty) and script (full content)
                {
                    std::ofstream hf(proj_dir + "/" + mod.header_file);
                    hf << "// " << stem << " header\n";
                }
                {
                    std::ofstream sf(proj_dir + "/" + mod.script_file);
                    sf << content;
                }
            }
            else
            {
                // Import .ash or .asc - look for the companion file
                fs::path src_dir = import_path.parent_path();
                fs::path ash_path = src_dir / (stem + ".ash");
                fs::path asc_path = src_dir / (stem + ".asc");

                // Copy header
                if (fs::exists(ash_path))
                {
                    std::error_code ec;
                    fs::copy_file(ash_path, fs::path(proj_dir) / mod.header_file,
                        fs::copy_options::overwrite_existing, ec);
                }
                else
                {
                    std::ofstream hf(proj_dir + "/" + mod.header_file);
                    hf << "// " << stem << " header\n";
                }

                // Copy script
                if (fs::exists(asc_path))
                {
                    std::error_code ec;
                    fs::copy_file(asc_path, fs::path(proj_dir) / mod.script_file,
                        fs::copy_options::overwrite_existing, ec);
                }
                else
                {
                    std::ofstream sf(proj_dir + "/" + mod.script_file);
                    sf << "// " << stem << " script\n";
                }
            }

            gd->script_modules.push_back(mod);
            RebuildTree();
            editor_.GetLogPanel().AddLog(
                "[Project] Imported script module '%s'.", stem.c_str());
        });
}

// -----------------------------------------------------------------------
// Export a script module (.ash + .asc files)
// -----------------------------------------------------------------------
void ProjectPanel::ExportScriptModule(int module_index)
{
    auto* project = editor_.GetApp().GetProject();
    if (!project || !project->IsLoaded())
        return;
    auto* gd = project->GetGameData();
    if (!gd || module_index < 0 || module_index >= (int)gd->script_modules.size())
        return;

    FileDialog::Open(FileDialogType::SelectFolder,
        "Export Script Module - Select Folder",
        "",
        project->GetProjectDir(),
        [this, module_index](const std::string& folder) {
            auto* proj = editor_.GetApp().GetProject();
            if (!proj || !proj->IsLoaded()) return;
            auto* gd2 = proj->GetGameData();
            if (!gd2 || module_index >= (int)gd2->script_modules.size()) return;

            auto& m = gd2->script_modules[module_index];
            namespace fs = std::filesystem;
            std::string proj_dir = proj->GetProjectDir();

            // Copy header
            if (!m.header_file.empty())
            {
                std::error_code ec;
                fs::copy_file(
                    fs::path(proj_dir) / m.header_file,
                    fs::path(folder) / m.header_file,
                    fs::copy_options::overwrite_existing, ec);
            }
            // Copy script
            if (!m.script_file.empty())
            {
                std::error_code ec;
                fs::copy_file(
                    fs::path(proj_dir) / m.script_file,
                    fs::path(folder) / m.script_file,
                    fs::copy_options::overwrite_existing, ec);
            }

            editor_.GetLogPanel().AddLog(
                "[Project] Exported script module '%s' to '%s'.",
                m.name.c_str(), folder.c_str());
        });
}

} // namespace AGSEditor
