// AGS Editor ImGui - GUI Editor implementation (Phase 8)
// Full visual GUI designer with drag-and-drop controls, multi-select,
// resize handles, snap guides, copy/paste, context menu, preview mode.
#include "gui_editor.h"
#include "ui/editor_ui.h"
#include "ui/log_panel.h"
#include "ui/project_panel.h"
#include "ui/sprite_chooser.h"
#include "ui/events_widget.h"
#include "ui/file_dialog.h"
#include "ui/folder_tree_widget.h"
#include "core/dpi_helper.h"
#include "project/project.h"
#include "project/game_data.h"
#include "project/texture_cache.h"
#include "project/sprite_loader.h"
#include "app.h"
#include "imgui.h"
#include "tinyxml2.h"
#include <SDL.h>
#include <cstring>
#include <cstdio>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>
#include <set>

namespace AGSEditor
{

// ============================================================================
// Clipboard serialization for cross-instance copy/paste
// ============================================================================

static const char* kClipboardHeader = "AGS_GUI_CONTROLS_V1";

static std::string SerializeControls(const std::vector<GUIControl>& controls)
{
    std::ostringstream ss;
    ss << kClipboardHeader << "\n";
    for (const auto& c : controls)
    {
        ss << "CTRL\n";
        ss << "type=" << (int)c.type << "\n";
        ss << "name=" << c.name << "\n";
        ss << "x=" << c.x << "\n";
        ss << "y=" << c.y << "\n";
        ss << "w=" << c.width << "\n";
        ss << "h=" << c.height << "\n";
        ss << "z=" << c.z_order << "\n";
        ss << "text=" << c.text << "\n";
        ss << "visible=" << (int)c.visible << "\n";
        ss << "enabled=" << (int)c.enabled << "\n";
        ss << "clickable=" << (int)c.clickable << "\n";
        ss << "font=" << c.font << "\n";
        ss << "text_color=" << c.text_color << "\n";
        ss << "bg_color=" << c.bg_color << "\n";
        ss << "border_color=" << c.border_color << "\n";
        ss << "show_border=" << (int)c.show_border << "\n";
        ss << "transparency=" << c.transparency << "\n";
        ss << "text_align=" << c.text_align << "\n";
        ss << "image=" << c.image << "\n";
        ss << "mouseover_image=" << c.mouseover_image << "\n";
        ss << "pushed_image=" << c.pushed_image << "\n";
        ss << "min_value=" << c.min_value << "\n";
        ss << "max_value=" << c.max_value << "\n";
        ss << "value=" << c.value << "\n";
        ss << "handle_image=" << c.handle_image << "\n";
        ss << "bg_image=" << c.bg_image << "\n";
        ss << "item_width=" << c.item_width << "\n";
        ss << "item_height=" << c.item_height << "\n";
        ss << "char_id=" << c.char_id << "\n";
        ss << "END\n";
    }
    return ss.str();
}

static bool ParseKeyValue(const std::string& line, std::string& key, std::string& val)
{
    size_t eq = line.find('=');
    if (eq == std::string::npos) return false;
    key = line.substr(0, eq);
    val = line.substr(eq + 1);
    return true;
}

static std::vector<GUIControl> DeserializeControls(const char* text)
{
    std::vector<GUIControl> result;
    if (!text) return result;

    std::istringstream ss(text);
    std::string line;

    // Check header
    if (!std::getline(ss, line)) return result;
    // Strip trailing CR
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (line != kClipboardHeader) return result;

    GUIControl ctrl;
    bool in_ctrl = false;

    while (std::getline(ss, line))
    {
        if (!line.empty() && line.back() == '\r') line.pop_back();

        if (line == "CTRL")
        {
            ctrl = GUIControl();
            in_ctrl = true;
            continue;
        }
        if (line == "END")
        {
            if (in_ctrl) result.push_back(ctrl);
            in_ctrl = false;
            continue;
        }
        if (!in_ctrl) continue;

        std::string key, val;
        if (!ParseKeyValue(line, key, val)) continue;

        if (key == "type")             ctrl.type = (GUIControlType)std::stoi(val);
        else if (key == "name")        strncpy(ctrl.name, val.c_str(), sizeof(ctrl.name) - 1);
        else if (key == "x")           ctrl.x = std::stoi(val);
        else if (key == "y")           ctrl.y = std::stoi(val);
        else if (key == "w")           ctrl.width = std::stoi(val);
        else if (key == "h")           ctrl.height = std::stoi(val);
        else if (key == "z")           ctrl.z_order = std::stoi(val);
        else if (key == "text")        strncpy(ctrl.text, val.c_str(), sizeof(ctrl.text) - 1);
        else if (key == "visible")     ctrl.visible = (std::stoi(val) != 0);
        else if (key == "enabled")     ctrl.enabled = (std::stoi(val) != 0);
        else if (key == "clickable")   ctrl.clickable = (std::stoi(val) != 0);
        else if (key == "font")        ctrl.font = std::stoi(val);
        else if (key == "text_color")  ctrl.text_color = std::stoi(val);
        else if (key == "bg_color")    ctrl.bg_color = std::stoi(val);
        else if (key == "border_color") ctrl.border_color = std::stoi(val);
        else if (key == "show_border") ctrl.show_border = (std::stoi(val) != 0);
        else if (key == "transparency") ctrl.transparency = std::stoi(val);
        else if (key == "text_align")  ctrl.text_align = std::stoi(val);
        else if (key == "image")       ctrl.image = std::stoi(val);
        else if (key == "mouseover_image") ctrl.mouseover_image = std::stoi(val);
        else if (key == "pushed_image") ctrl.pushed_image = std::stoi(val);
        else if (key == "min_value")   ctrl.min_value = std::stoi(val);
        else if (key == "max_value")   ctrl.max_value = std::stoi(val);
        else if (key == "value")       ctrl.value = std::stoi(val);
        else if (key == "handle_image") ctrl.handle_image = std::stoi(val);
        else if (key == "bg_image")    ctrl.bg_image = std::stoi(val);
        else if (key == "item_width")  ctrl.item_width = std::stoi(val);
        else if (key == "item_height") ctrl.item_height = std::stoi(val);
        else if (key == "char_id")     ctrl.char_id = std::stoi(val);
    }

    return result;
}

// ============================================================================
// Helpers
// ============================================================================

const char* GUIEditor::ControlTypeName(GUIControlType type)
{
    switch (type) {
        case GUIControlType::Button:         return "Button";
        case GUIControlType::Label:          return "Label";
        case GUIControlType::TextBox:        return "TextBox";
        case GUIControlType::ListBox:        return "ListBox";
        case GUIControlType::Slider:         return "Slider";
        case GUIControlType::InvWindow:      return "InvWindow";
        case GUIControlType::TextWindowEdge: return "TWEdge";
        default: return "???";
    }
}

ImU32 GUIEditor::ControlTypeColor(GUIControlType type)
{
    switch (type) {
        case GUIControlType::Button:    return IM_COL32(80, 80, 140, 255);
        case GUIControlType::Label:     return IM_COL32(60, 100, 60, 255);
        case GUIControlType::TextBox:   return IM_COL32(100, 100, 70, 255);
        case GUIControlType::ListBox:   return IM_COL32(70, 70, 120, 255);
        case GUIControlType::Slider:    return IM_COL32(120, 70, 70, 255);
        case GUIControlType::InvWindow: return IM_COL32(80, 110, 80, 255);
        default:                        return IM_COL32(70, 70, 70, 255);
    }
}

int GUIEditor::NextControlId()
{
    if (guis_.empty() || selected_gui_ < 0) return 0;
    auto& gui = guis_[selected_gui_];
    int max_id = -1;
    for (auto& c : gui.controls)
        if (c.id > max_id) max_id = c.id;
    return max_id + 1;
}

// Get interaction event schema for a GUI control type
// Returns nullptr for types with no events (Label, InvWindow, TextWindowEdge)
static const std::vector<InteractionEvent>* GetControlSchema(GUIControlType type)
{
    switch (type) {
        case GUIControlType::Button:  return &InteractionSchemas::GUIButton();
        case GUIControlType::ListBox: return &InteractionSchemas::GUIListBox();
        case GUIControlType::Slider:  return &InteractionSchemas::GUISlider();
        case GUIControlType::TextBox: return &InteractionSchemas::GUITextBox();
        default: return nullptr;
    }
}

// ============================================================================
// Constructor
// ============================================================================

GUIEditor::GUIEditor(EditorUI& editor, int gui_id)
    : editor_(editor)
    , title_("GUI Editor")
{
    // Try to load from project GameData
    auto* project = editor_.GetApp().GetProject();
    auto* gd = (project && project->IsLoaded()) ? project->GetGameData() : nullptr;
    if (gd && !gd->guis.empty())
    {
        for (const auto& src : gd->guis)
        {
            GUIEntry g{};
            g.id = src.id;
            std::strncpy(g.name, src.name.c_str(), sizeof(g.name) - 1);
            g.type = (src.tag_name == "GUITextWindow") ? GUIType::TextWindow : GUIType::Normal;
            g.width = src.width;
            g.height = src.height;
            g.x = src.x;
            g.y = src.y;
            g.visible = src.visible;
            g.clickable = src.clickable;
            g.bg_color = src.bg_color;
            g.border_color = src.border_color;
            g.transparency = src.transparency;
            g.z_order = src.z_order;
            g.popup_style = src.popup_style;

            // GUI-level events
            g.events.script_module = src.script_module.empty() ? "GlobalScript.asc" : src.script_module;
            g.events.handler_functions.resize(1);
            g.events.handler_functions[0] = src.on_click;

            // Convert controls
            for (const auto& csrc : src.controls)
            {
                GUIControl ctrl{};
                ctrl.id = csrc.id;
                std::strncpy(ctrl.name, csrc.name.c_str(), sizeof(ctrl.name) - 1);
                std::strncpy(ctrl.text, csrc.text.c_str(), sizeof(ctrl.text) - 1);
                ctrl.x = csrc.x;
                ctrl.y = csrc.y;
                ctrl.width = csrc.width;
                ctrl.height = csrc.height;
                ctrl.image = csrc.image;
                ctrl.font = csrc.font;
                ctrl.visible = true;
                ctrl.enabled = true;

                // Load event handler into Interactions struct
                ctrl.events.script_module = g.events.script_module;
                ctrl.events.handler_functions.resize(1);
                ctrl.events.handler_functions[0] = csrc.event_handler;

                // Map type tag to enum
                if (csrc.type_tag == "GUIButton") ctrl.type = GUIControlType::Button;
                else if (csrc.type_tag == "GUILabel") ctrl.type = GUIControlType::Label;
                else if (csrc.type_tag == "GUITextBox") ctrl.type = GUIControlType::TextBox;
                else if (csrc.type_tag == "GUIListBox") ctrl.type = GUIControlType::ListBox;
                else if (csrc.type_tag == "GUISlider") ctrl.type = GUIControlType::Slider;
                else if (csrc.type_tag == "GUIInventoryWindow") ctrl.type = GUIControlType::InvWindow;
                else if (csrc.type_tag == "GUITextWindowEdge") ctrl.type = GUIControlType::TextWindowEdge;
                else ctrl.type = GUIControlType::Label;

                g.controls.push_back(ctrl);
            }
            guis_.push_back(g);
        }
    }

    // If no project data, create default GUIs as fallback
    if (guis_.empty())
    {
        GUIEntry g{};
        g.id = 0;
        std::strncpy(g.name, "gStatusBar", sizeof(g.name));
        g.width = 320; g.height = 20;
        g.visible = true; g.clickable = true;
        g.border_color = 15; g.popup_style = 3;

        GUIControl label{};
        label.id = 0; label.type = GUIControlType::Label;
        std::strncpy(label.name, "lblScore", sizeof(label.name));
        std::strncpy(label.text, "Score: 0 of 0", sizeof(label.text));
        label.x = 5; label.y = 2; label.width = 150; label.height = 16;
        label.visible = true; label.enabled = true; label.clickable = false;
        g.controls.push_back(label);
        guis_.push_back(g);
    }

    if (gui_id >= 0 && gui_id < (int)guis_.size())
        selected_gui_ = gui_id;
}

// ============================================================================
// Main Draw
// ============================================================================

void GUIEditor::Draw()
{
    float list_w = Dpi(170);
    float palette_w = Dpi(200);
    float tree_w = show_folder_tree_ ? Dpi(160) : 0.0f;
    ImVec2 avail = ImGui::GetContentRegionAvail();

    // Folder tree panel
    if (show_folder_tree_)
    {
        ImGui::BeginChild("##GUIFolders", ImVec2(tree_w, avail.y), ImGuiChildFlags_Borders);
        auto* project = editor_.GetApp().GetProject();
        auto* gd = project ? project->GetGameData() : nullptr;
        if (gd)
        {
            const auto* sel = static_cast<const FolderInfo*>(selected_folder_);
            const auto* new_sel = DrawFolderTreeWidget(sel, gd->gui_folders, "All GUIs", &gd->gui_folders);
            if (new_sel != sel)
                selected_folder_ = new_sel;
        }
        ImGui::EndChild();
        ImGui::SameLine();
    }

    // Left: GUI list
    ImGui::BeginChild("##GUIList", ImVec2(list_w, avail.y), ImGuiChildFlags_Borders);
    DrawGUIList();
    ImGui::EndChild();

    ImGui::SameLine();

    // Center: Toolbar + Canvas
    float canvas_w = avail.x - list_w - palette_w - tree_w - Dpi(16);
    ImGui::BeginChild("##GUICanvas", ImVec2(canvas_w, avail.y), ImGuiChildFlags_Borders);
    DrawToolbar();
    ImGui::Separator();
    DrawGUICanvas();
    ImGui::EndChild();

    ImGui::SameLine();

    // Right: control palette + properties
    ImGui::BeginChild("##GUIRight", ImVec2(0, avail.y), ImGuiChildFlags_Borders);
    DrawControlPalette();
    ImGui::Separator();
    if (!selected_controls_.empty())
        DrawControlProperties();
    else
        DrawGUIProperties();
    ImGui::EndChild();

    // Multi-line text editing popup (Ctrl+E for Labels/Buttons)
    if (show_text_edit_popup_)
    {
        ImGui::OpenPopup("Edit Text##MultiLine");
        show_text_edit_popup_ = false;
    }
    if (ImGui::BeginPopupModal("Edit Text##MultiLine", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Use [[ for newlines in AGS Label/Button text.");
        ImGui::Separator();
        ImGui::InputTextMultiline("##textedit", text_edit_buf_, sizeof(text_edit_buf_),
            ImVec2(Dpi(400), Dpi(200)));
        ImGui::Separator();
        if (ImGui::Button("OK", ImVec2(Dpi(120), 0)))
        {
            if (!guis_.empty() && selected_gui_ >= 0 &&
                primary_selection_ >= 0 &&
                primary_selection_ < (int)guis_[selected_gui_].controls.size())
            {
                std::strncpy(guis_[selected_gui_].controls[primary_selection_].text,
                    text_edit_buf_, sizeof(guis_[selected_gui_].controls[primary_selection_].text) - 1);
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(Dpi(120), 0)))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

// ============================================================================
// GUI List
// ============================================================================

void GUIEditor::DrawGUIList()
{
    if (ImGui::Button("Add Normal"))
    {
        GUIEntry g{};
        g.id = (int)guis_.size();
        std::snprintf(g.name, sizeof(g.name), "gGUI%d", g.id);
        g.type = GUIType::Normal;
        g.width = 160; g.height = 80;
        g.visible = true; g.clickable = true;
        g.border_color = 15;
        guis_.push_back(g);
        selected_gui_ = (int)guis_.size() - 1;
        selected_controls_.clear();
        editor_.GetLogPanel().AddLog("[GUI] Added GUI '%s'.", g.name);
        editor_.GetProjectPanel().MarkTreeDirty();
    }
    ImGui::SameLine();
    if (ImGui::Button("Add TW"))
    {
        GUIEntry g{};
        g.id = (int)guis_.size();
        std::snprintf(g.name, sizeof(g.name), "gTextWin%d", g.id);
        g.type = GUIType::TextWindow;
        g.width = 200; g.height = 100;
        g.visible = false; g.clickable = false;
        guis_.push_back(g);
        selected_gui_ = (int)guis_.size() - 1;
        selected_controls_.clear();
        editor_.GetLogPanel().AddLog("[GUI] Added TextWindow GUI '%s'.", g.name);
        editor_.GetProjectPanel().MarkTreeDirty();
    }

    ImGui::Separator();
    ImGui::Checkbox("Folders", &show_folder_tree_);

    // Build folder filter set
    std::set<int> folder_ids;
    if (selected_folder_)
        CollectFolderItemIds(*static_cast<const FolderInfo*>(selected_folder_), folder_ids);

    for (int i = 0; i < (int)guis_.size(); i++)
    {
        // Filter by folder
        if (selected_folder_ && folder_ids.find(guis_[i].id) == folder_ids.end())
            continue;
        char label[128];
        const char* type_tag = guis_[i].type == GUIType::TextWindow ? " [TW]" : "";
        std::snprintf(label, sizeof(label), "%d: %s%s (%dx%d)",
                      guis_[i].id, guis_[i].name, type_tag,
                      guis_[i].width, guis_[i].height);
        if (ImGui::Selectable(label, selected_gui_ == i))
        {
            selected_gui_ = i;
            selected_controls_.clear();
            primary_selection_ = -1;
        }
        BeginItemDragSource(guis_[i].id, label);

        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Change GUI ID..."))
            {
                selected_gui_ = i;
                change_gui_id_target_ = i;
                show_change_gui_id_ = true;
            }
            ImGui::EndPopup();
        }
    }

    // Change GUI ID popup
    if (show_change_gui_id_)
    {
        ImGui::OpenPopup("Change GUI ID");
        show_change_gui_id_ = false;
    }
    if (ImGui::BeginPopupModal("Change GUI ID", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (selected_gui_ >= 0 && selected_gui_ < (int)guis_.size())
        {
            ImGui::Text("Change '%s' (ID %d) to new position:",
                guis_[selected_gui_].name, selected_gui_);
            ImGui::InputInt("New ID", &change_gui_id_target_);
            change_gui_id_target_ = std::max(0, std::min(change_gui_id_target_, (int)guis_.size() - 1));
            ImGui::Separator();

            if (ImGui::Button("Swap", ImVec2(Dpi(120), 0)))
            {
                if (change_gui_id_target_ != selected_gui_)
                {
                    std::swap(guis_[selected_gui_], guis_[change_gui_id_target_]);
                    for (int j = 0; j < (int)guis_.size(); j++) guis_[j].id = j;
                    selected_gui_ = change_gui_id_target_;
                    selected_controls_.clear();
                    editor_.GetProjectPanel().MarkTreeDirty();
                }
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(Dpi(120), 0)))
                ImGui::CloseCurrentPopup();
        }
        else
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

// ============================================================================
// Toolbar
// ============================================================================

void GUIEditor::DrawToolbar()
{
    // Edit mode buttons
    const char* mode_labels[] = { "Select", "Button", "Label", "TextBox", "ListBox", "Slider", "InvWin" };
    GUIEditMode modes[] = {
        GUIEditMode::Select, GUIEditMode::AddButton, GUIEditMode::AddLabel,
        GUIEditMode::AddTextBox, GUIEditMode::AddListBox, GUIEditMode::AddSlider,
        GUIEditMode::AddInvWindow
    };

    bool is_tw = (!guis_.empty() && selected_gui_ >= 0 && guis_[selected_gui_].type == GUIType::TextWindow);
    for (int i = 0; i < 7; i++)
    {
        // TextWindow GUIs: only allow Select mode, disable add-control buttons
        bool disabled = (is_tw && i > 0);
        if (disabled) ImGui::BeginDisabled();
        bool active = (edit_mode_ == modes[i]);
        if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
        if (ImGui::Button(mode_labels[i]))
            edit_mode_ = modes[i];
        if (active) ImGui::PopStyleColor();
        if (disabled) ImGui::EndDisabled();
        if (i < 6) ImGui::SameLine();
    }

    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    // Undo / Redo
    bool can_undo = (undo_pos_ >= 0);
    bool can_redo = (undo_pos_ < (int)undo_stack_.size() - 1);
    if (!can_undo) ImGui::BeginDisabled();
    if (ImGui::Button("Undo##gui"))
        Undo();
    if (!can_undo) ImGui::EndDisabled();
    ImGui::SameLine();
    if (!can_redo) ImGui::BeginDisabled();
    if (ImGui::Button("Redo##gui"))
        Redo();
    if (!can_redo) ImGui::EndDisabled();
    ImGui::SameLine();

    ImGui::TextDisabled("|");
    ImGui::SameLine();

    // Zoom
    ImGui::SetNextItemWidth(Dpi(80));
    ImGui::SliderFloat("##Zoom", &canvas_zoom_, 0.5f, 6.0f, "%.0f%%", ImGuiSliderFlags_None);
    ImGui::SameLine();

    // Snap toggle
    ImGui::Checkbox("Snap", &snap_enabled_);

    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    // Export / Import .guf
    {
        bool has_gui = (!guis_.empty() && selected_gui_ >= 0 && selected_gui_ < (int)guis_.size());
        if (!has_gui) ImGui::BeginDisabled();
        if (ImGui::Button("Export .guf"))
        {
            auto* project = editor_.GetApp().GetProject();
            std::string default_dir = (project && project->IsLoaded()) ? project->GetProjectDir() : ".";
            int gui_idx = selected_gui_;
            FileDialog::Open(FileDialogType::SaveFile, "Export GUI",
                ".guf{AGS GUI File}",
                default_dir,
                [this, gui_idx](const std::string& path) {
                    ExportGUI(gui_idx, path);
                });
        }
        if (!has_gui) ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("Import .guf"))
        {
            auto* project = editor_.GetApp().GetProject();
            std::string default_dir = (project && project->IsLoaded()) ? project->GetProjectDir() : ".";
            FileDialog::Open(FileDialogType::OpenFile, "Import GUI",
                ".guf{AGS GUI File}",
                default_dir,
                [this](const std::string& path) {
                    ImportGUI(path);
                });
        }
    }
}

// ============================================================================
// GUI Canvas — DrawList-based rendering
// ============================================================================

void GUIEditor::DrawGUICanvas()
{
    if (guis_.empty() || selected_gui_ < 0 || selected_gui_ >= (int)guis_.size())
    {
        ImGui::TextDisabled("No GUI selected.");
        return;
    }

    auto& gui = guis_[selected_gui_];
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImVec2 canvas_size = ImGui::GetContentRegionAvail();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Canvas background
    dl->AddRectFilled(canvas_pos,
        ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
        IM_COL32(45, 45, 50, 255));

    // GUI placed centered in canvas
    float gw = gui.width * canvas_zoom_;
    float gh = gui.height * canvas_zoom_;
    ImVec2 gui_origin(
        canvas_pos.x + (canvas_size.x - gw) * 0.5f,
        canvas_pos.y + (canvas_size.y - gh) * 0.5f
    );

    // Clip to canvas
    dl->PushClipRect(canvas_pos,
        ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), true);

    // 1) GUI background
    DrawGUIBackground(dl, gui_origin, canvas_zoom_);

    // 2) Controls (sorted by z-order)
    std::vector<int> draw_order(gui.controls.size());
    for (int i = 0; i < (int)draw_order.size(); i++) draw_order[i] = i;
    std::sort(draw_order.begin(), draw_order.end(), [&](int a, int b) {
        return gui.controls[a].z_order < gui.controls[b].z_order;
    });

    for (int idx : draw_order)
        DrawControl(dl, gui_origin, canvas_zoom_, gui.controls[idx], idx);

    // 3) Selection handles
    for (int sel_idx : selected_controls_)
    {
        if (sel_idx >= 0 && sel_idx < (int)gui.controls.size())
            DrawSelectionHandles(dl, gui_origin, canvas_zoom_, gui.controls[sel_idx]);
    }

    // 4) Snap guides
    DrawSnapGuides(dl, gui_origin, canvas_zoom_);

    // 5) Rubber-band selection
    DrawRubberBand(dl);

    // 6) Add-control preview
    DrawAddPreview(dl, gui_origin, canvas_zoom_);

    dl->PopClipRect();

    // Interaction
    ImGui::SetCursorScreenPos(canvas_pos);
    ImGui::InvisibleButton("##gui_canvas_btn", canvas_size,
        ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);

    HandleCanvasInput(gui_origin, canvas_zoom_, canvas_pos, canvas_size);

    // Context menu
    if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
        show_context_menu_ = true;

    if (show_context_menu_)
    {
        ImGui::OpenPopup("##GUIContextMenu");
        show_context_menu_ = false;
    }
    DrawContextMenu();

    // Keyboard shortcuts (disabled for TextWindow GUIs except zoom)
    bool is_tw_canvas = (gui.type == GUIType::TextWindow);
    if (ImGui::IsItemFocused() || ImGui::IsItemHovered())
    {
        if (!is_tw_canvas)
        {
            auto& io = ImGui::GetIO();
            if (ImGui::IsKeyPressed(ImGuiKey_Delete))
                DeleteSelectedControls();
            if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_C))
                CopySelectedControls();
            if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_V))
                PasteControls(ImGui::GetMousePos());

            // Arrow key movement
            if (!selected_controls_.empty())
            {
                int dx = 0, dy = 0;
                if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow))  dx = -1;
                if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) dx = 1;
                if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))    dy = -1;
                if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))  dy = 1;
                if (dx || dy)
                {
                    for (int sel : selected_controls_)
                    {
                        if (sel >= 0 && sel < (int)gui.controls.size() && !gui.controls[sel].locked)
                        {
                            gui.controls[sel].x += dx;
                            gui.controls[sel].y += dy;
                        }
                    }
                }
            }
        }
    }

    // Zoom with mouse wheel
    if (ImGui::IsItemHovered())
    {
        float wheel = ImGui::GetIO().MouseWheel;
        if (std::fabs(wheel) > 0.001f)
        {
            canvas_zoom_ *= (wheel > 0) ? 1.1f : (1.0f / 1.1f);
            canvas_zoom_ = std::clamp(canvas_zoom_, 0.25f, 8.0f);
        }
    }
}

void GUIEditor::DrawGUIBackground(ImDrawList* dl, ImVec2 origin, float zoom)
{
    auto& gui = guis_[selected_gui_];
    float gw = gui.width * zoom;
    float gh = gui.height * zoom;

    // Shadow
    dl->AddRectFilled(ImVec2(origin.x + 3, origin.y + 3),
                      ImVec2(origin.x + gw + 3, origin.y + gh + 3),
                      IM_COL32(0, 0, 0, 60));

    // Background
    int bg_val = std::clamp(gui.bg_color * 16, 0, 255);
    dl->AddRectFilled(origin, ImVec2(origin.x + gw, origin.y + gh),
                      IM_COL32(bg_val, bg_val, bg_val, 255));

    // Border
    dl->AddRect(origin, ImVec2(origin.x + gw, origin.y + gh),
                IM_COL32(200, 200, 200, 255));

    // GUI name label
    dl->AddText(ImVec2(origin.x + Dpi(2), origin.y - Dpi(14)),
                IM_COL32(200, 200, 200, 200), gui.name);
}

void GUIEditor::DrawControl(ImDrawList* dl, ImVec2 gui_origin, float zoom,
                             GUIControl& ctrl, int index)
{
    float cx = gui_origin.x + ctrl.x * zoom;
    float cy = gui_origin.y + ctrl.y * zoom;
    float cw = ctrl.width * zoom;
    float ch = ctrl.height * zoom;
    ImVec2 p0(cx, cy), p1(cx + cw, cy + ch);

    // Alpha for transparency
    int alpha = 255 - (int)(ctrl.transparency * 2.55f);
    alpha = std::clamp(alpha, 0, 255);

    if (!ctrl.visible) alpha = alpha / 3;

    // Background
    ImU32 bg = ControlTypeColor(ctrl.type);
    bg = (bg & 0x00FFFFFF) | ((ImU32)alpha << 24);
    dl->AddRectFilled(p0, p1, bg);

    // Border
    bool is_selected = std::find(selected_controls_.begin(), selected_controls_.end(), index) != selected_controls_.end();
    ImU32 border = is_selected ? IM_COL32(255, 255, 0, 255) : IM_COL32(160, 160, 160, (int)(alpha * 0.8f));
    float border_thick = is_selected ? 2.0f : 1.0f;
    if (ctrl.show_border)
        dl->AddRect(p0, p1, border, 0, 0, border_thick);

    // Type-specific rendering
    switch (ctrl.type)
    {
    case GUIControlType::Button:
    {
        // Button look: raised 3D effect
        dl->AddLine(p0, ImVec2(p1.x, p0.y), IM_COL32(200, 200, 200, alpha), 1.0f); // top
        dl->AddLine(p0, ImVec2(p0.x, p1.y), IM_COL32(200, 200, 200, alpha), 1.0f); // left
        dl->AddLine(ImVec2(p0.x, p1.y), p1, IM_COL32(60, 60, 60, alpha), 1.0f);     // bottom
        dl->AddLine(ImVec2(p1.x, p0.y), p1, IM_COL32(60, 60, 60, alpha), 1.0f);     // right

        if (ctrl.image >= 0)
        {
            auto* project = editor_.GetProject();
            auto* loader = project ? project->GetSpriteLoader() : nullptr;
            auto& tex_cache = editor_.GetApp().GetTextureCache();
            SDL_Texture* tex = loader ? tex_cache.GetSpriteTexture(ctrl.image, loader) : nullptr;
            if (tex)
            {
                const auto* info = tex_cache.FindSprite(ctrl.image);
                if (info && info->width > 0 && info->height > 0)
                {
                    float sw = (float)info->width;
                    float sh = (float)info->height;
                    float scale = std::min(cw / sw, ch / sh);
                    float dw = sw * scale;
                    float dh = sh * scale;
                    float ox = (cw - dw) * 0.5f;
                    float oy = (ch - dh) * 0.5f;
                    ImU32 tint = IM_COL32(255, 255, 255, alpha);
                    dl->AddImage((ImTextureID)(intptr_t)tex,
                                 ImVec2(cx + ox, cy + oy),
                                 ImVec2(cx + ox + dw, cy + oy + dh),
                                 ImVec2(0, 0), ImVec2(1, 1), tint);
                }
            }
            else
            {
                char img_txt[16];
                std::snprintf(img_txt, sizeof(img_txt), "[S%d]", ctrl.image);
                ImVec2 ts2 = ImGui::CalcTextSize(img_txt);
                dl->AddText(ImVec2(cx + (cw - ts2.x) * 0.5f, cy + 2),
                            IM_COL32(150, 200, 255, alpha), img_txt);
            }
        }
        break;
    }
    case GUIControlType::TextBox:
    {
        // TextBox: white inset border
        dl->AddRectFilled(ImVec2(cx + 2, cy + 2), ImVec2(cx + cw - 2, cy + ch - 2),
                          IM_COL32(255, 255, 255, alpha / 4));
        // Cursor blink line
        dl->AddLine(ImVec2(cx + 4, cy + 3), ImVec2(cx + 4, cy + ch - 3),
                    IM_COL32(255, 255, 255, alpha / 2));
        break;
    }
    case GUIControlType::ListBox:
    {
        // ListBox: scrollbar indicator
        float sb_x = cx + cw - 12 * zoom;
        dl->AddRectFilled(ImVec2(sb_x, cy), ImVec2(cx + cw, cy + ch),
                          IM_COL32(80, 80, 80, alpha));
        dl->AddRectFilled(ImVec2(sb_x + 2, cy + 2), ImVec2(cx + cw - 2, cy + ch * 0.4f),
                          IM_COL32(140, 140, 140, alpha));
        break;
    }
    case GUIControlType::Slider:
    {
        // Slider: track + handle
        float track_y = cy + ch * 0.5f;
        dl->AddLine(ImVec2(cx + 4, track_y), ImVec2(cx + cw - 4, track_y),
                    IM_COL32(160, 160, 160, alpha), 2.0f);
        float handle_x = cx + cw * 0.5f;
        dl->AddRectFilled(ImVec2(handle_x - 4, cy + 2), ImVec2(handle_x + 4, cy + ch - 2),
                          IM_COL32(200, 200, 200, alpha));
        break;
    }
    case GUIControlType::InvWindow:
    {
        // InvWindow: grid pattern
        float item_w = ctrl.item_width * zoom;
        float item_h = ctrl.item_height * zoom;
        if (item_w > 4 && item_h > 4)
        {
            for (float ix = cx; ix < cx + cw; ix += item_w)
                dl->AddLine(ImVec2(ix, cy), ImVec2(ix, cy + ch),
                            IM_COL32(100, 100, 100, alpha / 2));
            for (float iy = cy; iy < cy + ch; iy += item_h)
                dl->AddLine(ImVec2(cx, iy), ImVec2(cx + cw, iy),
                            IM_COL32(100, 100, 100, alpha / 2));
        }
        break;
    }
    default: break;
    }

    // Text label (centered for buttons, left-aligned for labels/textbox)
    const char* display_text = ctrl.text[0] ? ctrl.text : ctrl.name;
    ImVec2 ts = ImGui::CalcTextSize(display_text);
    float tx, ty = cy + (ch - ts.y) * 0.5f;

    if (ctrl.type == GUIControlType::Button || ctrl.text_align == 1)
        tx = cx + (cw - ts.x) * 0.5f;
    else if (ctrl.text_align == 2)
        tx = cx + cw - ts.x - 3;
    else
        tx = cx + 3;

    // Clamp text within control
    dl->PushClipRect(p0, p1, true);
    dl->AddText(ImVec2(tx, ty), IM_COL32(255, 255, 255, alpha), display_text);
    dl->PopClipRect();

    // Locked indicator
    if (ctrl.locked)
    {
        float mid_x = (p0.x + p1.x) * 0.5f;
        float mid_y = (p0.y + p1.y) * 0.5f;
        dl->AddLine(ImVec2(mid_x - Dpi(6), mid_y - Dpi(6)), ImVec2(mid_x + Dpi(6), mid_y + Dpi(6)),
                    IM_COL32(255, 80, 80, 180), 2.0f);
        dl->AddLine(ImVec2(mid_x + Dpi(6), mid_y - Dpi(6)), ImVec2(mid_x - Dpi(6), mid_y + Dpi(6)),
                    IM_COL32(255, 80, 80, 180), 2.0f);
    }
}

void GUIEditor::DrawSelectionHandles(ImDrawList* dl, ImVec2 gui_origin, float zoom, GUIControl& ctrl)
{
    float cx = gui_origin.x + ctrl.x * zoom;
    float cy = gui_origin.y + ctrl.y * zoom;
    float cw = ctrl.width * zoom;
    float ch = ctrl.height * zoom;
    float hs = Dpi(4); // handle half-size

    bool is_primary = (primary_selection_ >= 0 &&
                       primary_selection_ < (int)guis_[selected_gui_].controls.size() &&
                       &guis_[selected_gui_].controls[primary_selection_] == &ctrl);
    ImU32 col = is_primary ? IM_COL32(255, 60, 60, 255) : IM_COL32(255, 255, 60, 255);

    // 8 resize handles: TL, T, TR, R, BR, B, BL, L
    ImVec2 handles[8] = {
        ImVec2(cx, cy),                           // TL
        ImVec2(cx + cw * 0.5f, cy),               // T
        ImVec2(cx + cw, cy),                       // TR
        ImVec2(cx + cw, cy + ch * 0.5f),           // R
        ImVec2(cx + cw, cy + ch),                  // BR
        ImVec2(cx + cw * 0.5f, cy + ch),           // B
        ImVec2(cx, cy + ch),                       // BL
        ImVec2(cx, cy + ch * 0.5f),                // L
    };

    for (int i = 0; i < 8; i++)
    {
        dl->AddRectFilled(ImVec2(handles[i].x - hs, handles[i].y - hs),
                          ImVec2(handles[i].x + hs, handles[i].y + hs), col);
    }
}

void GUIEditor::DrawSnapGuides(ImDrawList* dl, ImVec2 gui_origin, float zoom)
{
    for (auto& g : snap_guides_)
    {
        // Dotted yellow line
        float dx = g.to.x - g.from.x;
        float dy = g.to.y - g.from.y;
        float len = std::sqrt(dx * dx + dy * dy);
        if (len < 1.0f) continue;

        float dash = Dpi(4), gap = Dpi(3);
        float ndx = dx / len, ndy = dy / len;
        float pos = 0;
        while (pos < len)
        {
            float end = std::min(pos + dash, len);
            dl->AddLine(
                ImVec2(g.from.x + ndx * pos, g.from.y + ndy * pos),
                ImVec2(g.from.x + ndx * end, g.from.y + ndy * end),
                IM_COL32(255, 255, 0, 150), 1.0f);
            pos = end + gap;
        }
    }
    snap_guides_.clear();
}

void GUIEditor::DrawRubberBand(ImDrawList* dl)
{
    if (!rubber_banding_) return;
    ImVec2 p0(std::min(rubber_start_.x, rubber_end_.x), std::min(rubber_start_.y, rubber_end_.y));
    ImVec2 p1(std::max(rubber_start_.x, rubber_end_.x), std::max(rubber_start_.y, rubber_end_.y));
    dl->AddRectFilled(p0, p1, IM_COL32(100, 150, 255, 30));
    dl->AddRect(p0, p1, IM_COL32(100, 150, 255, 200), 0, 0, 1.0f);
}

void GUIEditor::DrawAddPreview(ImDrawList* dl, ImVec2 gui_origin, float zoom)
{
    if (!adding_control_) return;
    ImVec2 p0(std::min(add_start_.x, add_end_.x), std::min(add_start_.y, add_end_.y));
    ImVec2 p1(std::max(add_start_.x, add_end_.x), std::max(add_start_.y, add_end_.y));
    dl->AddRectFilled(p0, p1, IM_COL32(100, 255, 100, 40));
    dl->AddRect(p0, p1, IM_COL32(100, 255, 100, 200), 0, 0, 1.5f);
}

// ============================================================================
// Hit testing
// ============================================================================

int GUIEditor::HitTestControl(ImVec2 gui_origin, float zoom, ImVec2 mouse)
{
    if (guis_.empty() || selected_gui_ < 0) return -1;
    auto& gui = guis_[selected_gui_];

    // Test in reverse z-order (topmost first)
    std::vector<int> order(gui.controls.size());
    for (int i = 0; i < (int)order.size(); i++) order[i] = i;
    std::sort(order.begin(), order.end(), [&](int a, int b) {
        return gui.controls[a].z_order > gui.controls[b].z_order;
    });

    for (int idx : order)
    {
        auto& c = gui.controls[idx];
        float cx = gui_origin.x + c.x * zoom;
        float cy = gui_origin.y + c.y * zoom;
        float cw = c.width * zoom;
        float ch = c.height * zoom;
        if (mouse.x >= cx && mouse.x <= cx + cw && mouse.y >= cy && mouse.y <= cy + ch)
            return idx;
    }
    return -1;
}

int GUIEditor::HitTestResizeHandle(ImVec2 gui_origin, float zoom, ImVec2 mouse, int ctrl_idx)
{
    if (ctrl_idx < 0) return -1;
    auto& c = guis_[selected_gui_].controls[ctrl_idx];
    float cx = gui_origin.x + c.x * zoom;
    float cy = gui_origin.y + c.y * zoom;
    float cw = c.width * zoom;
    float ch = c.height * zoom;
    float hs = Dpi(6); // grab zone

    ImVec2 handles[8] = {
        ImVec2(cx, cy), ImVec2(cx + cw * 0.5f, cy),
        ImVec2(cx + cw, cy), ImVec2(cx + cw, cy + ch * 0.5f),
        ImVec2(cx + cw, cy + ch), ImVec2(cx + cw * 0.5f, cy + ch),
        ImVec2(cx, cy + ch), ImVec2(cx, cy + ch * 0.5f),
    };

    for (int i = 0; i < 8; i++)
    {
        if (std::fabs(mouse.x - handles[i].x) <= hs && std::fabs(mouse.y - handles[i].y) <= hs)
            return i;
    }
    return -1;
}

// ============================================================================
// Canvas Input
// ============================================================================

void GUIEditor::HandleCanvasInput(ImVec2 gui_origin, float zoom, ImVec2 canvas_pos, ImVec2 canvas_size)
{
    auto& gui = guis_[selected_gui_];
    ImVec2 mouse = ImGui::GetMousePos();

    // ---- Add control mode ----
    if (edit_mode_ != GUIEditMode::Select)
    {
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        {
            adding_control_ = true;
            add_start_ = mouse;
            add_end_ = mouse;
        }
        if (adding_control_ && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
            add_end_ = mouse;

        if (adding_control_ && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            adding_control_ = false;

            // Calculate control rect in GUI coords
            float x0 = (std::min(add_start_.x, add_end_.x) - gui_origin.x) / zoom;
            float y0 = (std::min(add_start_.y, add_end_.y) - gui_origin.y) / zoom;
            float w = std::fabs(add_end_.x - add_start_.x) / zoom;
            float h = std::fabs(add_end_.y - add_start_.y) / zoom;

            if (w >= 2 && h >= 2)
            {
                GUIControl ctrl{};
                ctrl.id = NextControlId();
                ctrl.x = (int)x0; ctrl.y = (int)y0;
                ctrl.width = (int)w; ctrl.height = (int)h;
                ctrl.visible = true; ctrl.enabled = true; ctrl.clickable = true;

                GUIControlType new_type;
                switch (edit_mode_) {
                    case GUIEditMode::AddButton:    new_type = GUIControlType::Button; break;
                    case GUIEditMode::AddLabel:     new_type = GUIControlType::Label; break;
                    case GUIEditMode::AddTextBox:   new_type = GUIControlType::TextBox; break;
                    case GUIEditMode::AddListBox:   new_type = GUIControlType::ListBox; break;
                    case GUIEditMode::AddSlider:    new_type = GUIControlType::Slider; break;
                    case GUIEditMode::AddInvWindow: new_type = GUIControlType::InvWindow; break;
                    default: new_type = GUIControlType::Button; break;
                }
                ctrl.type = new_type;
                std::snprintf(ctrl.name, sizeof(ctrl.name), "%s%d",
                              ControlTypeName(new_type), ctrl.id);
                std::strncpy(ctrl.text, ControlTypeName(new_type), sizeof(ctrl.text));

                // Z-order: on top
                int max_z = 0;
                for (auto& c : gui.controls) max_z = std::max(max_z, c.z_order);
                ctrl.z_order = max_z + 1;

                gui.controls.push_back(ctrl);
                selected_controls_ = { (int)gui.controls.size() - 1 };
                primary_selection_ = (int)gui.controls.size() - 1;

                editor_.GetLogPanel().AddLog("[GUI] Added %s '%s' to '%s'.",
                    ControlTypeName(new_type), ctrl.name, gui.name);
            }

            // Revert to select mode
            edit_mode_ = GUIEditMode::Select;
        }
        return;
    }

    // ---- Select mode ----

    // Hover cursor feedback for resize handles and controls
    if (!dragging_ && !resizing_ && !rubber_banding_ && primary_selection_ >= 0)
    {
        int handle = HitTestResizeHandle(gui_origin, zoom, mouse, primary_selection_);
        if (handle >= 0)
        {
            // 0=TL, 1=T, 2=TR, 3=R, 4=BR, 5=B, 6=BL, 7=L
            switch (handle) {
                case 0: case 4: ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE); break; // TL, BR
                case 2: case 6: ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNESW); break; // TR, BL
                case 1: case 5: ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);   break; // T, B
                case 3: case 7: ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);   break; // R, L
            }
        }
    }
    if (resizing_)
    {
        switch (resize_side_) {
            case 0: case 4: ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE); break;
            case 2: case 6: ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNESW); break;
            case 1: case 5: ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);   break;
            case 3: case 7: ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);   break;
        }
    }

    // Check for resize handle hit on primary selection (disabled for TextWindow GUIs)
    bool is_tw_input = (gui.type == GUIType::TextWindow);
    if (!is_tw_input && ImGui::IsItemClicked(ImGuiMouseButton_Left) && primary_selection_ >= 0)
    {
        int handle = HitTestResizeHandle(gui_origin, zoom, mouse, primary_selection_);
        if (handle >= 0)
        {
            resizing_ = true;
            resize_side_ = handle;
            auto& c = gui.controls[primary_selection_];
            drag_start_ = mouse;
            drag_original_pos_ = { ImVec2((float)c.x, (float)c.y) };
            drag_original_size_ = ImVec2((float)c.width, (float)c.height);
            return;
        }
    }

    // Handle resizing
    if (resizing_)
    {
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left) && primary_selection_ >= 0)
        {
            auto& c = gui.controls[primary_selection_];
            float dx = (mouse.x - drag_start_.x) / zoom;
            float dy = (mouse.y - drag_start_.y) / zoom;

            float ox = drag_original_pos_[0].x, oy = drag_original_pos_[0].y;
            float ow = drag_original_size_.x, oh = drag_original_size_.y;

            switch (resize_side_) {
                case 0: c.x = (int)(ox + dx); c.y = (int)(oy + dy); c.width = (int)(ow - dx); c.height = (int)(oh - dy); break; // TL
                case 1: c.y = (int)(oy + dy); c.height = (int)(oh - dy); break; // T
                case 2: c.y = (int)(oy + dy); c.width = (int)(ow + dx); c.height = (int)(oh - dy); break; // TR
                case 3: c.width = (int)(ow + dx); break; // R
                case 4: c.width = (int)(ow + dx); c.height = (int)(oh + dy); break; // BR
                case 5: c.height = (int)(oh + dy); break; // B
                case 6: c.x = (int)(ox + dx); c.width = (int)(ow - dx); c.height = (int)(oh + dy); break; // BL
                case 7: c.x = (int)(ox + dx); c.width = (int)(ow - dx); break; // L
            }

            // Clamp minimum size
            if (c.width < 2) c.width = 2;
            if (c.height < 2) c.height = 2;
        }
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            // Push undo for the resize
            if (primary_selection_ >= 0 && primary_selection_ < (int)gui.controls.size())
            {
                int sel = primary_selection_;
                int gui_idx = selected_gui_;
                ImVec2 old_p = drag_original_pos_[0];
                ImVec2 old_s = drag_original_size_;
                auto& c = gui.controls[sel];
                ImVec2 new_p((float)c.x, (float)c.y);
                ImVec2 new_s((float)c.width, (float)c.height);
                PushUndo("Resize control",
                    [this, gui_idx, sel, old_p, old_s]() {
                        auto& ct = guis_[gui_idx].controls[sel];
                        ct.x = (int)old_p.x; ct.y = (int)old_p.y;
                        ct.width = (int)old_s.x; ct.height = (int)old_s.y;
                    },
                    [this, gui_idx, sel, new_p, new_s]() {
                        auto& ct = guis_[gui_idx].controls[sel];
                        ct.x = (int)new_p.x; ct.y = (int)new_p.y;
                        ct.width = (int)new_s.x; ct.height = (int)new_s.y;
                    });
            }
            resizing_ = false;
        }
        return;
    }

    // Click to select
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
    {
        int hit = HitTestControl(gui_origin, zoom, mouse);
        bool ctrl_held = ImGui::GetIO().KeyCtrl;

        if (hit >= 0)
        {
            // Group-aware: if hit control is in a group, select all in group
            int group = gui.controls[hit].group_id;

            if (ctrl_held)
            {
                // Toggle selection
                auto it = std::find(selected_controls_.begin(), selected_controls_.end(), hit);
                if (it != selected_controls_.end())
                    selected_controls_.erase(it);
                else
                    selected_controls_.push_back(hit);
            }
            else
            {
                // If already selected, don't deselect (allow drag)
                if (std::find(selected_controls_.begin(), selected_controls_.end(), hit) == selected_controls_.end())
                {
                    selected_controls_.clear();
                    if (group > 0)
                    {
                        for (int i = 0; i < (int)gui.controls.size(); i++)
                            if (gui.controls[i].group_id == group)
                                selected_controls_.push_back(i);
                    }
                    else
                    {
                        selected_controls_ = { hit };
                    }
                }
            }

            primary_selection_ = hit;

            // Start drag (disabled for TextWindow GUIs)
            if (!is_tw_input)
            {
                dragging_ = true;
                drag_start_ = mouse;
                drag_original_pos_.clear();
                for (int sel : selected_controls_)
                {
                    auto& c = gui.controls[sel];
                    drag_original_pos_.push_back(ImVec2((float)c.x, (float)c.y));
                }
            }
        }
        else
        {
            if (!ctrl_held && !is_tw_input)
            {
                // Start rubber-band selection
                selected_controls_.clear();
                primary_selection_ = -1;
                rubber_banding_ = true;
                rubber_start_ = mouse;
                rubber_end_ = mouse;
            }
            else if (!ctrl_held)
            {
                // TextWindow: just deselect
                selected_controls_.clear();
                primary_selection_ = -1;
            }
        }
    }

    // Double-click to create/navigate default event handler
    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && ImGui::IsItemHovered())
    {
        int hit = HitTestControl(gui_origin, zoom, mouse);
        if (hit >= 0 && hit < (int)gui.controls.size())
        {
            // Double-clicked a control - use its default event
            auto& ctrl = gui.controls[hit];
            const auto* schema = GetControlSchema(ctrl.type);
            if (schema && !schema->empty())
            {
                if (ctrl.events.handler_functions.empty())
                    ctrl.events.handler_functions.resize(1);
                if (ctrl.events.script_module.empty())
                    ctrl.events.script_module = "GlobalScript.asc";

                std::string& handler = ctrl.events.handler_functions[0];
                if (handler.empty())
                {
                    // Create default handler
                    handler = std::string(ctrl.name) + "_" + (*schema)[0].suffix;
                    editor_.OpenScriptFile(ctrl.events.script_module);
                    editor_.GetLogPanel().AddLog("[GUI] Created handler: %s in %s",
                        handler.c_str(), ctrl.events.script_module.c_str());
                }
                else
                {
                    // Navigate to existing handler
                    editor_.OpenScriptFile(ctrl.events.script_module);
                }
                // Select the control and focus events panel
                selected_controls_ = { hit };
                primary_selection_ = hit;
                focus_events_ = true;
            }
        }
        else
        {
            // Double-clicked on GUI background - use GUI's OnClick event
            if (gui.type == GUIType::Normal)
            {
                if (gui.events.handler_functions.empty())
                    gui.events.handler_functions.resize(1);
                if (gui.events.script_module.empty())
                    gui.events.script_module = "GlobalScript.asc";

                std::string& handler = gui.events.handler_functions[0];
                if (handler.empty())
                {
                    handler = std::string(gui.name) + "_OnClick";
                    editor_.OpenScriptFile(gui.events.script_module);
                    editor_.GetLogPanel().AddLog("[GUI] Created handler: %s in %s",
                        handler.c_str(), gui.events.script_module.c_str());
                }
                else
                {
                    editor_.OpenScriptFile(gui.events.script_module);
                }
                // Deselect controls so GUI properties (with events) show
                selected_controls_.clear();
                primary_selection_ = -1;
                focus_events_ = true;
            }
        }
    }

    // Drag to move
    if (dragging_ && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        snap_guides_.clear();
        for (int i = 0; i < (int)selected_controls_.size(); i++)
        {
            int sel = selected_controls_[i];
            if (sel < 0 || sel >= (int)gui.controls.size()) continue;
            auto& c = gui.controls[sel];
            if (c.locked) continue;

            float dx = (mouse.x - drag_start_.x) / zoom;
            float dy = (mouse.y - drag_start_.y) / zoom;
            int new_x = (int)(drag_original_pos_[i].x + dx);
            int new_y = (int)(drag_original_pos_[i].y + dy);

            // Snapping
            if (snap_enabled_ && !ImGui::GetIO().KeyCtrl)
            {
                ImVec2 snapped = SnapPosition(sel, new_x, new_y);
                new_x = (int)snapped.x;
                new_y = (int)snapped.y;
            }

            c.x = new_x;
            c.y = new_y;
        }
    }

    if (dragging_ && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
        // Push undo for the drag move
        std::vector<std::pair<int, ImVec2>> old_pos, new_pos;
        for (int i = 0; i < (int)selected_controls_.size(); i++)
        {
            int sel = selected_controls_[i];
            if (sel >= 0 && sel < (int)gui.controls.size())
            {
                old_pos.push_back({sel, drag_original_pos_[i]});
                new_pos.push_back({sel, ImVec2((float)gui.controls[sel].x, (float)gui.controls[sel].y)});
            }
        }
        if (!old_pos.empty())
        {
            int gui_idx = selected_gui_;
            PushUndo("Drag move",
                [this, gui_idx, old_pos]() {
                    for (auto& [sel, pos] : old_pos) {
                        guis_[gui_idx].controls[sel].x = (int)pos.x;
                        guis_[gui_idx].controls[sel].y = (int)pos.y;
                    }
                },
                [this, gui_idx, new_pos]() {
                    for (auto& [sel, pos] : new_pos) {
                        guis_[gui_idx].controls[sel].x = (int)pos.x;
                        guis_[gui_idx].controls[sel].y = (int)pos.y;
                    }
                });
        }
        dragging_ = false;
        snap_guides_.clear();
    }

    // Rubber-band
    if (rubber_banding_)
    {
        rubber_end_ = mouse;
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            rubber_banding_ = false;
            // Select all controls within rubber band
            float rx0 = std::min(rubber_start_.x, rubber_end_.x);
            float ry0 = std::min(rubber_start_.y, rubber_end_.y);
            float rx1 = std::max(rubber_start_.x, rubber_end_.x);
            float ry1 = std::max(rubber_start_.y, rubber_end_.y);

            selected_controls_.clear();
            for (int i = 0; i < (int)gui.controls.size(); i++)
            {
                auto& c = gui.controls[i];
                float cx = gui_origin.x + c.x * zoom;
                float cy = gui_origin.y + c.y * zoom;
                float cw = c.width * zoom;
                float ch = c.height * zoom;

                if (cx + cw >= rx0 && cx <= rx1 && cy + ch >= ry0 && cy <= ry1)
                    selected_controls_.push_back(i);
            }
            if (!selected_controls_.empty())
                primary_selection_ = selected_controls_[0];
        }
    }

    // Arrow key movement for selected controls (disabled for TextWindow GUIs)
    if (!is_tw_input && !selected_controls_.empty() && !dragging_ && !resizing_ && ImGui::IsItemFocused())
    {
        int dx = 0, dy = 0;
        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow))  dx = -1;
        if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) dx = 1;
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))    dy = -1;
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))  dy = 1;
        if (dx != 0 || dy != 0)
        {
            // Capture positions for undo
            std::vector<std::pair<int, ImVec2>> old_positions;
            for (int sel : selected_controls_)
            {
                if (sel >= 0 && sel < (int)gui.controls.size() && !gui.controls[sel].locked)
                    old_positions.push_back({sel, ImVec2((float)gui.controls[sel].x, (float)gui.controls[sel].y)});
            }

            for (auto& [sel, _] : old_positions)
            {
                gui.controls[sel].x += dx;
                gui.controls[sel].y += dy;
            }

            auto new_positions = old_positions;
            for (auto& [sel, pos] : new_positions)
                pos = ImVec2((float)gui.controls[sel].x, (float)gui.controls[sel].y);

            int gui_idx = selected_gui_;
            PushUndo("Move controls",
                [this, gui_idx, old_positions]() {
                    for (auto& [sel, pos] : old_positions) {
                        guis_[gui_idx].controls[sel].x = (int)pos.x;
                        guis_[gui_idx].controls[sel].y = (int)pos.y;
                    }
                },
                [this, gui_idx, new_positions]() {
                    for (auto& [sel, pos] : new_positions) {
                        guis_[gui_idx].controls[sel].x = (int)pos.x;
                        guis_[gui_idx].controls[sel].y = (int)pos.y;
                    }
                });
        }

        // Delete/Backspace to delete selected controls
        if (ImGui::IsKeyPressed(ImGuiKey_Delete) || ImGui::IsKeyPressed(ImGuiKey_Backspace))
            DeleteSelectedControls();

        // Ctrl+C / Ctrl+V / Ctrl+X / Ctrl+Z / Ctrl+Y
        if (ImGui::GetIO().KeyCtrl)
        {
            if (ImGui::IsKeyPressed(ImGuiKey_C)) CopySelectedControls();
            if (ImGui::IsKeyPressed(ImGuiKey_V)) PasteControls(ImVec2(10, 10));
            if (ImGui::IsKeyPressed(ImGuiKey_X)) { CopySelectedControls(); DeleteSelectedControls(); }
            if (ImGui::IsKeyPressed(ImGuiKey_Z)) Undo();
            if (ImGui::IsKeyPressed(ImGuiKey_Y)) Redo();
            if (ImGui::IsKeyPressed(ImGuiKey_E))
            {
                // Ctrl+E: open multi-line text editor for Label/Button
                if (primary_selection_ >= 0 && primary_selection_ < (int)gui.controls.size())
                {
                    auto& c = gui.controls[primary_selection_];
                    if (c.type == GUIControlType::Label || c.type == GUIControlType::Button)
                    {
                        std::strncpy(text_edit_buf_, c.text, sizeof(text_edit_buf_) - 1);
                        text_edit_buf_[sizeof(text_edit_buf_) - 1] = '\0';
                        show_text_edit_popup_ = true;
                    }
                }
            }
        }
    }
}

// ============================================================================
// Snap
// ============================================================================

ImVec2 GUIEditor::SnapPosition(int ctrl_idx, int x, int y)
{
    auto& gui = guis_[selected_gui_];
    auto& c = gui.controls[ctrl_idx];
    float best_x = (float)x, best_y = (float)y;

    for (int i = 0; i < (int)gui.controls.size(); i++)
    {
        if (i == ctrl_idx) continue;
        if (std::find(selected_controls_.begin(), selected_controls_.end(), i) != selected_controls_.end())
            continue;

        auto& other = gui.controls[i];

        // Snap left edge to left edge
        if (std::abs(x - other.x) <= (int)kSnapThreshold)
            best_x = (float)other.x;
        // Snap left edge to right edge
        if (std::abs(x - (other.x + other.width)) <= (int)kSnapThreshold)
            best_x = (float)(other.x + other.width);
        // Snap right edge to left edge
        if (std::abs((x + c.width) - other.x) <= (int)kSnapThreshold)
            best_x = (float)(other.x - c.width);
        // Snap right edge to right edge
        if (std::abs((x + c.width) - (other.x + other.width)) <= (int)kSnapThreshold)
            best_x = (float)(other.x + other.width - c.width);

        // Snap top to top
        if (std::abs(y - other.y) <= (int)kSnapThreshold)
            best_y = (float)other.y;
        // Snap top to bottom
        if (std::abs(y - (other.y + other.height)) <= (int)kSnapThreshold)
            best_y = (float)(other.y + other.height);
        // Snap bottom to top
        if (std::abs((y + c.height) - other.y) <= (int)kSnapThreshold)
            best_y = (float)(other.y - c.height);
        // Snap bottom to bottom
        if (std::abs((y + c.height) - (other.y + other.height)) <= (int)kSnapThreshold)
            best_y = (float)(other.y + other.height - c.height);
    }

    return ImVec2(best_x, best_y);
}

// ============================================================================
// Control Palette
// ============================================================================

void GUIEditor::DrawControlPalette()
{
    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Add Control");
    ImGui::Separator();

    struct CtrlType { const char* label; GUIEditMode mode; };
    CtrlType types[] = {
        { "Button",    GUIEditMode::AddButton },
        { "Label",     GUIEditMode::AddLabel },
        { "TextBox",   GUIEditMode::AddTextBox },
        { "ListBox",   GUIEditMode::AddListBox },
        { "Slider",    GUIEditMode::AddSlider },
        { "InvWindow", GUIEditMode::AddInvWindow },
    };
    for (auto& t : types)
    {
        bool active = (edit_mode_ == t.mode);
        if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
        if (ImGui::Button(t.label, ImVec2(-1, 0)))
            edit_mode_ = t.mode;
        if (active) ImGui::PopStyleColor();
    }

    ImGui::Spacing();

    // Control list for current GUI
    if (!guis_.empty() && selected_gui_ >= 0)
    {
        auto& gui = guis_[selected_gui_];
        ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Controls (%d)", (int)gui.controls.size());
        ImGui::Separator();

        ImGui::BeginChild("##CtrlList", ImVec2(0, Dpi(120)), ImGuiChildFlags_Borders);
        for (int i = 0; i < (int)gui.controls.size(); i++)
        {
            auto& c = gui.controls[i];
            char label[128];
            std::snprintf(label, sizeof(label), "%d: %s [%s]%s", c.id, c.name,
                          ControlTypeName(c.type), c.locked ? " (L)" : "");
            bool sel = std::find(selected_controls_.begin(), selected_controls_.end(), i)
                       != selected_controls_.end();
            if (ImGui::Selectable(label, sel))
            {
                if (!ImGui::GetIO().KeyCtrl)
                    selected_controls_.clear();
                selected_controls_.push_back(i);
                primary_selection_ = i;
            }
        }
        ImGui::EndChild();
    }
}

// ============================================================================
// Properties
// ============================================================================

void GUIEditor::DrawGUIProperties()
{
    if (guis_.empty() || selected_gui_ < 0) return;
    auto& gui = guis_[selected_gui_];

    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "GUI Properties");
    ImGui::Separator();

    ImGui::InputText("Name", gui.name, sizeof(gui.name));

    const char* types[] = { "Normal", "TextWindow" };
    int type_idx = (int)gui.type;
    if (ImGui::Combo("Type", &type_idx, types, 2))
        gui.type = (GUIType)type_idx;

    ImGui::InputInt("Width", &gui.width);
    ImGui::InputInt("Height", &gui.height);
    gui.width = std::max(1, gui.width);
    gui.height = std::max(1, gui.height);

    if (gui.type == GUIType::Normal)
    {
        ImGui::InputInt("X", &gui.x);
        ImGui::InputInt("Y", &gui.y);
    }

    ImGui::Checkbox("Visible", &gui.visible);
    ImGui::Checkbox("Clickable", &gui.clickable);
    ImGui::SliderInt("Transparency", &gui.transparency, 0, 100, "%d%%");
    ImGui::InputInt("Z-Order", &gui.z_order);
    ImGui::InputInt("BG Color", &gui.bg_color);
    ImGui::InputInt("Border Color", &gui.border_color);

    const char* popup_styles[] = { "Normal", "Mouse-Y", "Script Only", "Persistent" };
    ImGui::Combo("Popup Style", &gui.popup_style, popup_styles, 4);

    // GUI-level events (OnClick for NormalGUI)
    if (gui.type == GUIType::Normal)
    {
        bool want_focus = focus_events_ && selected_controls_.empty();
        if (want_focus) focus_events_ = false;
        DrawEventsSection(
            gui.events,
            InteractionSchemas::NormalGUI(),
            gui.name,
            gui.events.script_module.empty() ? "GlobalScript.asc" : gui.events.script_module,
            [this](const std::string& script_module, const std::string& func_name) {
                editor_.OpenScriptFile(script_module);
                editor_.GetLogPanel().AddLog("[GUI] Created handler: %s in %s",
                    func_name.c_str(), script_module.c_str());
            },
            [this](const std::string& script_module, const std::string& func_name) {
                editor_.OpenScriptFile(script_module);
            },
            want_focus
        );
    }

    if (ImGui::Button("Delete GUI") && guis_.size() > 1)
    {
        confirm_delete_gui_ = true;
    }

    // Confirmation dialog for GUI deletion
    if (confirm_delete_gui_)
    {
        ImGui::OpenPopup("Confirm Delete GUI");
        confirm_delete_gui_ = false;
    }
    if (ImGui::BeginPopupModal("Confirm Delete GUI", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Are you sure you want to delete this GUI?");
        ImGui::Text("Doing so could break any scripts that refer to GUIs by number.");
        ImGui::Separator();
        if (ImGui::Button("Delete", ImVec2(Dpi(120), 0)))
        {
            guis_.erase(guis_.begin() + selected_gui_);
            if (selected_gui_ >= (int)guis_.size())
                selected_gui_ = (int)guis_.size() - 1;
            selected_controls_.clear();
            editor_.GetProjectPanel().MarkTreeDirty();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(Dpi(120), 0)))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

void GUIEditor::DrawControlProperties()
{
    if (guis_.empty() || selected_gui_ < 0) return;
    auto& gui = guis_[selected_gui_];

    // Single or multi-select
    bool multi = (selected_controls_.size() > 1);
    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f),
                       multi ? "Properties (%d selected)" : "Control Properties",
                       (int)selected_controls_.size());
    ImGui::Separator();

    if (primary_selection_ < 0 || primary_selection_ >= (int)gui.controls.size())
        return;

    auto& ctrl = gui.controls[primary_selection_];

    ImGui::Text("Type: %s", ControlTypeName(ctrl.type));
    ImGui::Text("ID: %d", ctrl.id);

    if (ImGui::CollapsingHeader("Design", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::InputText("Name", ctrl.name, sizeof(ctrl.name));
        ImGui::InputText("Text", ctrl.text, sizeof(ctrl.text));
        if (ctrl.type == GUIControlType::Label || ctrl.type == GUIControlType::Button)
        {
            ImGui::SameLine();
            if (ImGui::SmallButton("...##textedit"))
            {
                std::strncpy(text_edit_buf_, ctrl.text, sizeof(text_edit_buf_) - 1);
                text_edit_buf_[sizeof(text_edit_buf_) - 1] = '\0';
                show_text_edit_popup_ = true;
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Multi-line text editor (Ctrl+E)");
        }
    }

    // Events section (per control type)
    const auto* schema = GetControlSchema(ctrl.type);
    if (schema)
    {
        bool want_focus = focus_events_;
        focus_events_ = false;
        DrawEventsSection(
            ctrl.events,
            *schema,
            ctrl.name,
            ctrl.events.script_module.empty() ? "GlobalScript.asc" : ctrl.events.script_module,
            [this](const std::string& script_module, const std::string& func_name) {
                editor_.OpenScriptFile(script_module);
                editor_.GetLogPanel().AddLog("[GUI] Created handler: %s in %s",
                    func_name.c_str(), script_module.c_str());
            },
            [this](const std::string& script_module, const std::string& func_name) {
                editor_.OpenScriptFile(script_module);
            },
            want_focus
        );
    }

    if (ImGui::CollapsingHeader("Layout", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::InputInt("X", &ctrl.x);
        ImGui::InputInt("Y", &ctrl.y);
        ImGui::InputInt("Width", &ctrl.width);
        ImGui::InputInt("Height", &ctrl.height);
        ctrl.width = std::max(2, ctrl.width);
        ctrl.height = std::max(2, ctrl.height);
        ImGui::InputInt("Z-Order", &ctrl.z_order);
        ImGui::Checkbox("Locked", &ctrl.locked);
    }

    if (ImGui::CollapsingHeader("Appearance", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Visible", &ctrl.visible);
        ImGui::Checkbox("Enabled", &ctrl.enabled);
        ImGui::Checkbox("Clickable", &ctrl.clickable);
        ImGui::Checkbox("Show Border", &ctrl.show_border);
        ImGui::SliderInt("Transparency", &ctrl.transparency, 0, 100, "%d%%");
        ImGui::InputInt("Font", &ctrl.font);
        ImGui::InputInt("Text Color", &ctrl.text_color);
        ImGui::InputInt("BG Color", &ctrl.bg_color);
        ImGui::InputInt("Border Color", &ctrl.border_color);

        const char* aligns[] = { "Left", "Center", "Right" };
        ImGui::Combo("Text Align", &ctrl.text_align, aligns, 3);
    }

    // Type-specific properties
    if (ctrl.type == GUIControlType::Button)
    {
        if (ImGui::CollapsingHeader("Button"))
        {
            auto* project = editor_.GetApp().GetProject();
            auto* loader = project ? project->GetSpriteLoader() : nullptr;
            auto& tex_cache = editor_.GetApp().GetTextureCache();

            if (DrawSpriteField("Image", &ctrl.image, loader, tex_cache))
                ImGui::OpenPopup("Choose Sprite##BtnImg");
            if (ctrl.image >= 0)
            {
                ImGui::SameLine();
                if (ImGui::SmallButton("Auto-Size"))
                {
                    if (loader)
                    {
                        auto* metrics = loader->GetMetrics(ctrl.image);
                        if (metrics && metrics->exists)
                        {
                            ctrl.width = std::max(2, metrics->width);
                            ctrl.height = std::max(2, metrics->height);
                        }
                    }
                }
            }
            int chosen = ctrl.image;
            if (DrawSpriteChooserPopup("Choose Sprite##BtnImg", loader,
                    tex_cache, &chosen, ctrl.image))
                ctrl.image = chosen;

            if (DrawSpriteField("Mouseover Image", &ctrl.mouseover_image, loader, tex_cache))
                ImGui::OpenPopup("Choose Sprite##BtnMO");
            chosen = ctrl.mouseover_image;
            if (DrawSpriteChooserPopup("Choose Sprite##BtnMO", loader,
                    tex_cache, &chosen, ctrl.mouseover_image))
                ctrl.mouseover_image = chosen;

            if (DrawSpriteField("Pushed Image", &ctrl.pushed_image, loader, tex_cache))
                ImGui::OpenPopup("Choose Sprite##BtnPush");
            chosen = ctrl.pushed_image;
            if (DrawSpriteChooserPopup("Choose Sprite##BtnPush", loader,
                    tex_cache, &chosen, ctrl.pushed_image))
                ctrl.pushed_image = chosen;
        }
    }
    else if (ctrl.type == GUIControlType::Slider)
    {
        if (ImGui::CollapsingHeader("Slider"))
        {
            ImGui::InputInt("Min Value", &ctrl.min_value);
            ImGui::InputInt("Max Value", &ctrl.max_value);
            ImGui::InputInt("Value", &ctrl.value);

            auto* project = editor_.GetApp().GetProject();
            auto* loader = project ? project->GetSpriteLoader() : nullptr;
            auto& tex_cache = editor_.GetApp().GetTextureCache();

            if (DrawSpriteField("Handle Image", &ctrl.handle_image, loader, tex_cache))
                ImGui::OpenPopup("Choose Sprite##SldrH");
            int chosen = ctrl.handle_image;
            if (DrawSpriteChooserPopup("Choose Sprite##SldrH", loader,
                    tex_cache, &chosen, ctrl.handle_image))
                ctrl.handle_image = chosen;

            if (DrawSpriteField("BG Image", &ctrl.bg_image, loader, tex_cache))
                ImGui::OpenPopup("Choose Sprite##SldrBG");
            chosen = ctrl.bg_image;
            if (DrawSpriteChooserPopup("Choose Sprite##SldrBG", loader,
                    tex_cache, &chosen, ctrl.bg_image))
                ctrl.bg_image = chosen;
        }
    }
    else if (ctrl.type == GUIControlType::InvWindow)
    {
        if (ImGui::CollapsingHeader("InvWindow"))
        {
            ImGui::InputInt("Item Width", &ctrl.item_width);
            ImGui::InputInt("Item Height", &ctrl.item_height);
            ImGui::InputInt("Character ID", &ctrl.char_id);
            ImGui::TextDisabled("(-1 = player character)");
        }
    }

    if (ImGui::CollapsingHeader("Grouping"))
    {
        ImGui::InputInt("Group ID", &ctrl.group_id);
        ImGui::TextDisabled("0 = no group");
    }

    ImGui::Spacing();
    // TextWindow GUIs: cannot delete edge controls
    if (!(guis_[selected_gui_].type == GUIType::TextWindow))
    {
        if (ImGui::Button("Delete Control"))
            DeleteSelectedControls();
    }
}

// ============================================================================
// Context Menu
// ============================================================================

void GUIEditor::DrawContextMenu()
{
    // No context menu for TextWindow GUIs (edge controls cannot be modified)
    if (!guis_.empty() && selected_gui_ >= 0 && guis_[selected_gui_].type == GUIType::TextWindow)
        return;

    if (ImGui::BeginPopup("##GUIContextMenu"))
    {
        if (ImGui::MenuItem("Copy", "Ctrl+C", false, !selected_controls_.empty()))
            CopySelectedControls();
        if (ImGui::MenuItem("Paste", "Ctrl+V", false, !clipboard_.empty() || SDL_HasClipboardText()))
            PasteControls(ImGui::GetMousePos());
        if (ImGui::MenuItem("Delete", "Del", false, !selected_controls_.empty()))
            DeleteSelectedControls();

        ImGui::Separator();

        if (ImGui::MenuItem("Bring to Front", nullptr, false, !selected_controls_.empty()))
            BringToFront();
        if (ImGui::MenuItem("Send to Back", nullptr, false, !selected_controls_.empty()))
            SendToBack();

        ImGui::Separator();

        bool multi = (selected_controls_.size() > 1);
        if (ImGui::MenuItem("Align Left", nullptr, false, multi))   AlignControls(0);
        if (ImGui::MenuItem("Align Top", nullptr, false, multi))    AlignControls(1);
        if (ImGui::MenuItem("Align Right", nullptr, false, multi))  AlignControls(2);
        if (ImGui::MenuItem("Align Bottom", nullptr, false, multi)) AlignControls(3);

        bool three_plus = (selected_controls_.size() >= 3);
        if (ImGui::MenuItem("Distribute Horizontally", nullptr, false, three_plus))
            DistributeControls(true);
        if (ImGui::MenuItem("Distribute Vertically", nullptr, false, three_plus))
            DistributeControls(false);

        ImGui::Separator();

        if (ImGui::MenuItem("Lock", nullptr, false, !selected_controls_.empty()))
        {
            auto& gui = guis_[selected_gui_];
            for (int sel : selected_controls_)
                if (sel >= 0 && sel < (int)gui.controls.size())
                    gui.controls[sel].locked = true;
        }
        if (ImGui::MenuItem("Unlock", nullptr, false, !selected_controls_.empty()))
        {
            auto& gui = guis_[selected_gui_];
            for (int sel : selected_controls_)
                if (sel >= 0 && sel < (int)gui.controls.size())
                    gui.controls[sel].locked = false;
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Group", nullptr, false, multi))
        {
            auto& gui = guis_[selected_gui_];
            int gid = next_group_id_++;
            for (int sel : selected_controls_)
                if (sel >= 0 && sel < (int)gui.controls.size())
                    gui.controls[sel].group_id = gid;
        }
        if (ImGui::MenuItem("Ungroup", nullptr, false, !selected_controls_.empty()))
        {
            auto& gui = guis_[selected_gui_];
            for (int sel : selected_controls_)
                if (sel >= 0 && sel < (int)gui.controls.size())
                    gui.controls[sel].group_id = 0;
        }

        ImGui::EndPopup();
    }
}

// ============================================================================
// Operations
// ============================================================================

void GUIEditor::DeleteSelectedControls()
{
    if (guis_.empty() || selected_gui_ < 0) return;
    auto& gui = guis_[selected_gui_];

    // Capture deleted controls for undo
    int gui_idx = selected_gui_;
    std::vector<std::pair<int, GUIControl>> deleted;
    std::sort(selected_controls_.begin(), selected_controls_.end(), std::greater<int>());
    for (int sel : selected_controls_)
    {
        if (sel >= 0 && sel < (int)gui.controls.size())
            deleted.push_back({sel, gui.controls[sel]});
    }

    // Delete
    for (auto& [sel, _] : deleted)
        gui.controls.erase(gui.controls.begin() + sel);

    PushUndo("Delete controls",
        [this, gui_idx, deleted]() {
            // Undo: re-insert in forward order (lowest index first)
            auto sorted = deleted;
            std::sort(sorted.begin(), sorted.end(),
                [](auto& a, auto& b) { return a.first < b.first; });
            for (auto& [idx, ctrl] : sorted)
            {
                if (idx <= (int)guis_[gui_idx].controls.size())
                    guis_[gui_idx].controls.insert(guis_[gui_idx].controls.begin() + idx, ctrl);
                else
                    guis_[gui_idx].controls.push_back(ctrl);
            }
        },
        [this, gui_idx, deleted]() {
            // Redo: delete again (already in reverse order)
            for (auto& [sel, _] : deleted)
                if (sel >= 0 && sel < (int)guis_[gui_idx].controls.size())
                    guis_[gui_idx].controls.erase(guis_[gui_idx].controls.begin() + sel);
        });

    selected_controls_.clear();
    primary_selection_ = -1;
}

void GUIEditor::CopySelectedControls()
{
    if (guis_.empty() || selected_gui_ < 0) return;
    auto& gui = guis_[selected_gui_];
    clipboard_.clear();
    for (int sel : selected_controls_)
    {
        if (sel >= 0 && sel < (int)gui.controls.size())
            clipboard_.push_back(gui.controls[sel]);
    }
    // Also set system clipboard for cross-instance paste
    if (!clipboard_.empty())
    {
        std::string data = SerializeControls(clipboard_);
        SDL_SetClipboardText(data.c_str());
    }
}

void GUIEditor::PasteControls(ImVec2 pos)
{
    // Try system clipboard first for cross-instance paste
    std::vector<GUIControl> paste_src;
    if (SDL_HasClipboardText())
    {
        char* text = SDL_GetClipboardText();
        if (text)
        {
            paste_src = DeserializeControls(text);
            SDL_free(text);
        }
    }
    // Fall back to in-memory clipboard
    if (paste_src.empty())
        paste_src = clipboard_;
    if (paste_src.empty() || guis_.empty() || selected_gui_ < 0) return;
    auto& gui = guis_[selected_gui_];

    selected_controls_.clear();
    int base_x = paste_src[0].x, base_y = paste_src[0].y;

    for (auto ctrl : paste_src)
    {
        ctrl.id = NextControlId();
        ctrl.x = ctrl.x - base_x + 10;
        ctrl.y = ctrl.y - base_y + 10;
        ctrl.locked = false;
        ctrl.group_id = 0;
        std::snprintf(ctrl.name, sizeof(ctrl.name), "%s%d",
                      ControlTypeName(ctrl.type), ctrl.id);

        int max_z = 0;
        for (auto& c : gui.controls) max_z = std::max(max_z, c.z_order);
        ctrl.z_order = max_z + 1;

        gui.controls.push_back(ctrl);
        selected_controls_.push_back((int)gui.controls.size() - 1);
    }
    if (!selected_controls_.empty())
        primary_selection_ = selected_controls_[0];
}

void GUIEditor::BringToFront()
{
    if (guis_.empty() || selected_gui_ < 0) return;
    auto& gui = guis_[selected_gui_];
    int max_z = 0;
    for (auto& c : gui.controls) max_z = std::max(max_z, c.z_order);
    for (int sel : selected_controls_)
        if (sel >= 0 && sel < (int)gui.controls.size())
            gui.controls[sel].z_order = ++max_z;
}

void GUIEditor::SendToBack()
{
    if (guis_.empty() || selected_gui_ < 0) return;
    auto& gui = guis_[selected_gui_];
    int min_z = 0;
    for (auto& c : gui.controls) min_z = std::min(min_z, c.z_order);
    for (int sel : selected_controls_)
        if (sel >= 0 && sel < (int)gui.controls.size())
            gui.controls[sel].z_order = --min_z;
}

void GUIEditor::AlignControls(int mode)
{
    if (selected_controls_.size() < 2 || guis_.empty()) return;
    auto& gui = guis_[selected_gui_];

    int ref_val = 0;
    auto& ref = gui.controls[selected_controls_[0]];
    switch (mode) {
        case 0: ref_val = ref.x; break;
        case 1: ref_val = ref.y; break;
        case 2: ref_val = ref.x + ref.width; break;
        case 3: ref_val = ref.y + ref.height; break;
    }

    for (int sel : selected_controls_)
    {
        if (sel < 0 || sel >= (int)gui.controls.size()) continue;
        auto& c = gui.controls[sel];
        switch (mode) {
            case 0: c.x = ref_val; break;
            case 1: c.y = ref_val; break;
            case 2: c.x = ref_val - c.width; break;
            case 3: c.y = ref_val - c.height; break;
        }
    }
}

void GUIEditor::DistributeControls(bool horizontal)
{
    if (selected_controls_.size() < 3 || guis_.empty()) return;
    auto& gui = guis_[selected_gui_];

    // Sort by position
    std::vector<int> sorted = selected_controls_;
    if (horizontal)
        std::sort(sorted.begin(), sorted.end(), [&](int a, int b) { return gui.controls[a].x < gui.controls[b].x; });
    else
        std::sort(sorted.begin(), sorted.end(), [&](int a, int b) { return gui.controls[a].y < gui.controls[b].y; });

    int first = sorted.front(), last = sorted.back();
    int n = (int)sorted.size();

    if (horizontal)
    {
        float start = (float)gui.controls[first].x;
        float end = (float)gui.controls[last].x;
        float step = (end - start) / (float)(n - 1);
        for (int i = 0; i < n; i++)
            gui.controls[sorted[i]].x = (int)(start + step * i);
    }
    else
    {
        float start = (float)gui.controls[first].y;
        float end = (float)gui.controls[last].y;
        float step = (end - start) / (float)(n - 1);
        for (int i = 0; i < n; i++)
            gui.controls[sorted[i]].y = (int)(start + step * i);
    }
}

// ============================================================================
// Undo / Redo
// ============================================================================

void GUIEditor::PushUndo(const std::string& desc, std::function<void()> undo_fn, std::function<void()> redo_fn)
{
    // Discard any redo history beyond current position
    if (undo_pos_ + 1 < (int)undo_stack_.size())
        undo_stack_.erase(undo_stack_.begin() + undo_pos_ + 1, undo_stack_.end());

    undo_stack_.push_back({desc, std::move(undo_fn), std::move(redo_fn)});
    undo_pos_ = (int)undo_stack_.size() - 1;

    // Enforce max size
    while ((int)undo_stack_.size() > kMaxUndoSteps)
    {
        undo_stack_.erase(undo_stack_.begin());
        undo_pos_--;
    }
}

void GUIEditor::Undo()
{
    if (undo_pos_ < 0) return;
    undo_stack_[undo_pos_].undo_fn();
    undo_pos_--;
}

void GUIEditor::Redo()
{
    if (undo_pos_ + 1 >= (int)undo_stack_.size()) return;
    undo_pos_++;
    undo_stack_[undo_pos_].redo_fn();
}

// ============================================================================
// Export GUI (.guf)  — XML format compatible with the C# editor
// ============================================================================

static const char* ControlTypeTag(GUIControlType type)
{
    switch (type) {
        case GUIControlType::Button:         return "GUIButton";
        case GUIControlType::Label:          return "GUILabel";
        case GUIControlType::TextBox:        return "GUITextBox";
        case GUIControlType::ListBox:        return "GUIListBox";
        case GUIControlType::Slider:         return "GUISlider";
        case GUIControlType::InvWindow:      return "GUIInventoryWindow";
        case GUIControlType::TextWindowEdge: return "GUITextWindowEdge";
        default: return "GUILabel";
    }
}

static GUIControlType ControlTypeFromTag(const char* tag)
{
    if (!tag) return GUIControlType::Label;
    if (std::strcmp(tag, "GUIButton") == 0)            return GUIControlType::Button;
    if (std::strcmp(tag, "GUILabel") == 0)              return GUIControlType::Label;
    if (std::strcmp(tag, "GUITextBox") == 0)            return GUIControlType::TextBox;
    if (std::strcmp(tag, "GUIListBox") == 0)            return GUIControlType::ListBox;
    if (std::strcmp(tag, "GUISlider") == 0)             return GUIControlType::Slider;
    if (std::strcmp(tag, "GUIInventoryWindow") == 0)    return GUIControlType::InvWindow;
    if (std::strcmp(tag, "GUITextWindowEdge") == 0)     return GUIControlType::TextWindowEdge;
    return GUIControlType::Label;
}

static void XmlSetText(tinyxml2::XMLDocument& doc, tinyxml2::XMLElement* parent, const char* name, const char* value)
{
    auto* el = doc.NewElement(name);
    el->SetText(value);
    parent->InsertEndChild(el);
}

static void XmlSetInt(tinyxml2::XMLDocument& doc, tinyxml2::XMLElement* parent, const char* name, int value)
{
    auto* el = doc.NewElement(name);
    el->SetText(value);
    parent->InsertEndChild(el);
}

static void XmlSetBool(tinyxml2::XMLDocument& doc, tinyxml2::XMLElement* parent, const char* name, bool value)
{
    XmlSetText(doc, parent, name, value ? "True" : "False");
}

static const char* XmlGetText(const tinyxml2::XMLElement* parent, const char* name, const char* def = "")
{
    auto* el = parent->FirstChildElement(name);
    return (el && el->GetText()) ? el->GetText() : def;
}

static int XmlGetInt(const tinyxml2::XMLElement* parent, const char* name, int def = 0)
{
    auto* el = parent->FirstChildElement(name);
    if (!el || !el->GetText()) return def;
    return std::atoi(el->GetText());
}

static bool XmlGetBool(const tinyxml2::XMLElement* parent, const char* name, bool def = false)
{
    const char* txt = XmlGetText(parent, name, nullptr);
    if (!txt) return def;
    return (std::strcmp(txt, "True") == 0 || std::strcmp(txt, "true") == 0 || std::strcmp(txt, "1") == 0);
}

void GUIEditor::ExportGUI(int gui_idx, const std::string& path)
{
    if (gui_idx < 0 || gui_idx >= (int)guis_.size()) return;
    const auto& gui = guis_[gui_idx];

    tinyxml2::XMLDocument doc;
    doc.InsertFirstChild(doc.NewDeclaration());

    auto* root = doc.NewElement("ExportedGUI");
    root->SetAttribute("Version", "2");
    doc.InsertEndChild(root);

    // GUI definition
    auto* gui_el = doc.NewElement("GUI");
    root->InsertEndChild(gui_el);

    const char* gui_tag = (gui.type == GUIType::TextWindow) ? "GUITextWindow" : "GUIMain";
    XmlSetText(doc, gui_el, "TagName", gui_tag);
    XmlSetText(doc, gui_el, "Name", gui.name);
    XmlSetInt(doc, gui_el, "Left", gui.x);
    XmlSetInt(doc, gui_el, "Top", gui.y);
    XmlSetInt(doc, gui_el, "Width", gui.width);
    XmlSetInt(doc, gui_el, "Height", gui.height);
    XmlSetBool(doc, gui_el, "Visible", gui.visible);
    XmlSetBool(doc, gui_el, "Clickable", gui.clickable);
    XmlSetInt(doc, gui_el, "BackgroundColor", gui.bg_color);
    XmlSetInt(doc, gui_el, "BorderColor", gui.border_color);
    XmlSetInt(doc, gui_el, "Transparency", gui.transparency);
    XmlSetInt(doc, gui_el, "ZOrder", gui.z_order);
    XmlSetInt(doc, gui_el, "PopupStyle", gui.popup_style);

    // Events
    if (!gui.events.handler_functions.empty() && !gui.events.handler_functions[0].empty())
        XmlSetText(doc, gui_el, "OnClick", gui.events.handler_functions[0].c_str());

    // Controls
    auto* ctrls_el = doc.NewElement("Controls");
    gui_el->InsertEndChild(ctrls_el);

    // Track referenced sprite IDs
    std::set<int> sprite_ids;

    for (const auto& ctrl : gui.controls)
    {
        auto* ctrl_el = doc.NewElement("Control");
        ctrl_el->SetAttribute("Type", ControlTypeTag(ctrl.type));
        ctrls_el->InsertEndChild(ctrl_el);

        XmlSetText(doc, ctrl_el, "Name", ctrl.name);
        XmlSetInt(doc, ctrl_el, "Left", ctrl.x);
        XmlSetInt(doc, ctrl_el, "Top", ctrl.y);
        XmlSetInt(doc, ctrl_el, "Width", ctrl.width);
        XmlSetInt(doc, ctrl_el, "Height", ctrl.height);
        XmlSetInt(doc, ctrl_el, "ZOrder", ctrl.z_order);
        XmlSetText(doc, ctrl_el, "Text", ctrl.text);
        XmlSetBool(doc, ctrl_el, "Visible", ctrl.visible);
        XmlSetBool(doc, ctrl_el, "Enabled", ctrl.enabled);
        XmlSetBool(doc, ctrl_el, "Clickable", ctrl.clickable);
        XmlSetInt(doc, ctrl_el, "Font", ctrl.font);
        XmlSetInt(doc, ctrl_el, "TextColor", ctrl.text_color);
        XmlSetInt(doc, ctrl_el, "BackgroundColor", ctrl.bg_color);
        XmlSetInt(doc, ctrl_el, "BorderColor", ctrl.border_color);
        XmlSetBool(doc, ctrl_el, "ShowBorder", ctrl.show_border);
        XmlSetInt(doc, ctrl_el, "Transparency", ctrl.transparency);
        XmlSetInt(doc, ctrl_el, "TextAlignment", ctrl.text_align);

        // Type-specific
        if (ctrl.type == GUIControlType::Button || ctrl.type == GUIControlType::TextWindowEdge)
        {
            XmlSetInt(doc, ctrl_el, "Image", ctrl.image);
            XmlSetInt(doc, ctrl_el, "MouseoverImage", ctrl.mouseover_image);
            XmlSetInt(doc, ctrl_el, "PushedImage", ctrl.pushed_image);
            if (ctrl.image > 0)       sprite_ids.insert(ctrl.image);
            if (ctrl.mouseover_image > 0) sprite_ids.insert(ctrl.mouseover_image);
            if (ctrl.pushed_image > 0)    sprite_ids.insert(ctrl.pushed_image);
        }
        else if (ctrl.type == GUIControlType::Slider)
        {
            XmlSetInt(doc, ctrl_el, "MinValue", ctrl.min_value);
            XmlSetInt(doc, ctrl_el, "MaxValue", ctrl.max_value);
            XmlSetInt(doc, ctrl_el, "Value", ctrl.value);
            XmlSetInt(doc, ctrl_el, "HandleImage", ctrl.handle_image);
            XmlSetInt(doc, ctrl_el, "BackgroundImage", ctrl.bg_image);
            if (ctrl.handle_image > 0) sprite_ids.insert(ctrl.handle_image);
            if (ctrl.bg_image > 0)     sprite_ids.insert(ctrl.bg_image);
        }
        else if (ctrl.type == GUIControlType::InvWindow)
        {
            XmlSetInt(doc, ctrl_el, "ItemWidth", ctrl.item_width);
            XmlSetInt(doc, ctrl_el, "ItemHeight", ctrl.item_height);
            XmlSetInt(doc, ctrl_el, "CharacterID", ctrl.char_id);
        }

        // Event handler
        if (!ctrl.events.handler_functions.empty() && !ctrl.events.handler_functions[0].empty())
            XmlSetText(doc, ctrl_el, "EventHandler", ctrl.events.handler_functions[0].c_str());
    }

    // Referenced sprites list (IDs only — sprites must exist in target project)
    if (!sprite_ids.empty())
    {
        auto* sprites_el = doc.NewElement("UsedSprites");
        root->InsertEndChild(sprites_el);
        for (int sid : sprite_ids)
        {
            auto* s = doc.NewElement("Sprite");
            s->SetAttribute("ID", sid);
            sprites_el->InsertEndChild(s);
        }
    }

    auto err = doc.SaveFile(path.c_str());
    if (err == tinyxml2::XML_SUCCESS)
        editor_.GetLogPanel().AddLog("[GUI] Exported GUI '%s' to: %s", gui.name, path.c_str());
    else
        editor_.GetLogPanel().AddLog("[GUI] Failed to save .guf file: %s", path.c_str());
}

// ============================================================================
// Import GUI (.guf)
// ============================================================================

void GUIEditor::ImportGUI(const std::string& path)
{
    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(path.c_str()) != tinyxml2::XML_SUCCESS)
    {
        editor_.GetLogPanel().AddLog("[GUI] Failed to load .guf file: %s", path.c_str());
        return;
    }

    auto* root = doc.FirstChildElement("ExportedGUI");
    if (!root)
    {
        editor_.GetLogPanel().AddLog("[GUI] Invalid .guf file: missing <ExportedGUI> root.");
        return;
    }

    auto* gui_el = root->FirstChildElement("GUI");
    if (!gui_el)
    {
        editor_.GetLogPanel().AddLog("[GUI] Invalid .guf file: missing <GUI> element.");
        return;
    }

    GUIEntry g{};
    g.id = (int)guis_.size();

    const char* tag_name = XmlGetText(gui_el, "TagName", "GUIMain");
    g.type = (std::strcmp(tag_name, "GUITextWindow") == 0) ? GUIType::TextWindow : GUIType::Normal;

    // Load name, ensure uniqueness
    std::string base_name = XmlGetText(gui_el, "Name", "gImported");
    std::string unique_name = base_name;
    int suffix = 1;
    bool name_taken = true;
    while (name_taken)
    {
        name_taken = false;
        for (const auto& existing : guis_)
        {
            if (std::strcmp(existing.name, unique_name.c_str()) == 0)
            {
                name_taken = true;
                unique_name = base_name + std::to_string(suffix++);
                break;
            }
        }
    }
    std::strncpy(g.name, unique_name.c_str(), sizeof(g.name) - 1);

    g.x = XmlGetInt(gui_el, "Left", 0);
    g.y = XmlGetInt(gui_el, "Top", 0);
    g.width = XmlGetInt(gui_el, "Width", 160);
    g.height = XmlGetInt(gui_el, "Height", 80);
    g.visible = XmlGetBool(gui_el, "Visible", true);
    g.clickable = XmlGetBool(gui_el, "Clickable", true);
    g.bg_color = XmlGetInt(gui_el, "BackgroundColor", 0);
    g.border_color = XmlGetInt(gui_el, "BorderColor", 15);
    g.transparency = XmlGetInt(gui_el, "Transparency", 0);
    g.z_order = XmlGetInt(gui_el, "ZOrder", 0);
    g.popup_style = XmlGetInt(gui_el, "PopupStyle", 0);

    // GUI-level events
    const char* on_click = XmlGetText(gui_el, "OnClick", "");
    g.events.script_module = "GlobalScript.asc";
    g.events.handler_functions.resize(1);
    g.events.handler_functions[0] = on_click;

    // Controls
    auto* ctrls_el = gui_el->FirstChildElement("Controls");
    if (ctrls_el)
    {
        int ctrl_id = 0;
        for (auto* ctrl_el = ctrls_el->FirstChildElement("Control");
             ctrl_el; ctrl_el = ctrl_el->NextSiblingElement("Control"))
        {
            GUIControl ctrl{};
            ctrl.id = ctrl_id++;
            ctrl.type = ControlTypeFromTag(ctrl_el->Attribute("Type"));

            std::strncpy(ctrl.name, XmlGetText(ctrl_el, "Name", ""), sizeof(ctrl.name) - 1);
            ctrl.x = XmlGetInt(ctrl_el, "Left", 0);
            ctrl.y = XmlGetInt(ctrl_el, "Top", 0);
            ctrl.width = XmlGetInt(ctrl_el, "Width", 60);
            ctrl.height = XmlGetInt(ctrl_el, "Height", 20);
            ctrl.z_order = XmlGetInt(ctrl_el, "ZOrder", 0);
            std::strncpy(ctrl.text, XmlGetText(ctrl_el, "Text", ""), sizeof(ctrl.text) - 1);
            ctrl.visible = XmlGetBool(ctrl_el, "Visible", true);
            ctrl.enabled = XmlGetBool(ctrl_el, "Enabled", true);
            ctrl.clickable = XmlGetBool(ctrl_el, "Clickable", true);
            ctrl.font = XmlGetInt(ctrl_el, "Font", 0);
            ctrl.text_color = XmlGetInt(ctrl_el, "TextColor", 15);
            ctrl.bg_color = XmlGetInt(ctrl_el, "BackgroundColor", 0);
            ctrl.border_color = XmlGetInt(ctrl_el, "BorderColor", 15);
            ctrl.show_border = XmlGetBool(ctrl_el, "ShowBorder", true);
            ctrl.transparency = XmlGetInt(ctrl_el, "Transparency", 0);
            ctrl.text_align = XmlGetInt(ctrl_el, "TextAlignment", 0);

            // Type-specific
            if (ctrl.type == GUIControlType::Button || ctrl.type == GUIControlType::TextWindowEdge)
            {
                ctrl.image = XmlGetInt(ctrl_el, "Image", -1);
                ctrl.mouseover_image = XmlGetInt(ctrl_el, "MouseoverImage", -1);
                ctrl.pushed_image = XmlGetInt(ctrl_el, "PushedImage", -1);
            }
            else if (ctrl.type == GUIControlType::Slider)
            {
                ctrl.min_value = XmlGetInt(ctrl_el, "MinValue", 0);
                ctrl.max_value = XmlGetInt(ctrl_el, "MaxValue", 100);
                ctrl.value = XmlGetInt(ctrl_el, "Value", 50);
                ctrl.handle_image = XmlGetInt(ctrl_el, "HandleImage", -1);
                ctrl.bg_image = XmlGetInt(ctrl_el, "BackgroundImage", -1);
            }
            else if (ctrl.type == GUIControlType::InvWindow)
            {
                ctrl.item_width = XmlGetInt(ctrl_el, "ItemWidth", 40);
                ctrl.item_height = XmlGetInt(ctrl_el, "ItemHeight", 22);
                ctrl.char_id = XmlGetInt(ctrl_el, "CharacterID", -1);
            }

            // Event handler
            const char* handler = XmlGetText(ctrl_el, "EventHandler", "");
            ctrl.events.script_module = g.events.script_module;
            ctrl.events.handler_functions.resize(1);
            ctrl.events.handler_functions[0] = handler;

            g.controls.push_back(ctrl);
        }
    }

    guis_.push_back(g);
    selected_gui_ = (int)guis_.size() - 1;
    selected_controls_.clear();
    primary_selection_ = -1;
    editor_.GetProjectPanel().MarkTreeDirty();
    editor_.GetLogPanel().AddLog("[GUI] Imported GUI '%s' from: %s (%d controls)",
        g.name, path.c_str(), (int)g.controls.size());
}

} // namespace AGSEditor
