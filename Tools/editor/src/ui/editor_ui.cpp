// AGS Editor ImGui - Main Editor UI implementation
#include "editor_ui.h"
#include "menu_bar.h"
#include "project_panel.h"
#include "properties_panel.h"
#include "log_panel.h"
#include "panes/welcome_pane.h"
#include "panes/script_editor.h"
#include "panes/room_editor.h"
#include "panes/build_pane.h"

#include "imgui.h"
#include "app.h"
#include "project/project.h"
#include "core/preferences.h"

#include <SDL.h>
#include <algorithm>

namespace AGSEditor
{

EditorUI::EditorUI(AGSEditorApp& app)
    : app_(app)
{
    menu_bar_ = std::make_unique<MenuBar>(*this);
    project_panel_ = std::make_unique<ProjectPanel>(*this);
    properties_panel_ = std::make_unique<PropertiesPanel>(*this);
    log_panel_ = std::make_unique<LogPanel>(*this);

    // Open welcome pane by default
    OpenWelcomePane();
    log_panel_->AddLog("[Info] AGS Editor (ImGui) started.");
}

EditorUI::~EditorUI() = default;

void EditorUI::Draw()
{
    // Update window title with test/debug state prefix
    {
        auto* build_pane = FindPane<BuildPane>();
        bool running = build_pane && build_pane->GetBuildSystem().IsRunning();
        bool building = build_pane && build_pane->GetBuildSystem().IsBuilding();

        std::string prefix;
        if (running) prefix = "[Testing] ";
        else if (building) prefix = "[Building] ";

        auto* project = app_.GetProject();
        std::string base_title;
        if (project && project->IsLoaded())
            base_title = project->GetGameTitle() + " - AGS Editor";
        else
            base_title = "AGS Editor";

        std::string full_title = prefix + base_title;
        const char* current = SDL_GetWindowTitle(app_.GetWindow());
        if (!current || full_title != current)
            SDL_SetWindowTitle(app_.GetWindow(), full_title.c_str());
    }

    // Global keyboard shortcuts
    if (ImGui::IsKeyPressed(ImGuiKey_F1))
    {
        // Try to get context-sensitive help keyword from the active pane
        std::string keyword;
        if (active_pane_index_ >= 0 && active_pane_index_ < (int)panes_.size())
            keyword = panes_[active_pane_index_]->GetHelpKeyword();

        // Map known AGS API types to manual page names
        // Types that match the manual page name directly
        static const char* const kManualPages[] = {
            "AudioChannel", "AudioClip", "Button", "Camera", "Character",
            "DateTime", "Dialog", "DialogOptionsRenderingInfo", "Dictionary",
            "DrawingSurface", "DynamicSprite", "File", "Game",
            "GUI", "GUIControl", "Hotspot", "InventoryItem", "InvWindow",
            "Label", "ListBox", "Maths", "Mouse", "Object", "Overlay",
            "Parser", "Region", "Room", "Screen", "Set", "Slider",
            "Speech", "String", "System", "TextBox", "ViewFrame", "Viewport",
            nullptr
        };

        // Also map some common global function names
        static const struct { const char* keyword; const char* page; } kFunctionPages[] = {
            { "Display", "Globalfunctions_Message" },
            { "DisplayAt", "Globalfunctions_Message" },
            { "DisplayMessage", "Globalfunctions_Message" },
            { "Wait", "Globalfunctions_Wait" },
            { "WaitKey", "Globalfunctions_Wait" },
            { "WaitMouseKey", "Globalfunctions_Wait" },
            { "WaitInput", "Globalfunctions_Wait" },
            { "QuitGame", "Globalfunctions_General" },
            { "SaveGameSlot", "Globalfunctions_General" },
            { "RestoreGameSlot", "Globalfunctions_General" },
            { "SetTimer", "Globalfunctions_General" },
            { "IsTimerExpired", "Globalfunctions_General" },
            { "SetGameSpeed", "Globalfunctions_General" },
            { "GetGameSpeed", "Globalfunctions_General" },
            { "PlayVideo", "Multimedia" },
            { "SetBackgroundFrame", "Globalfunctions_Room" },
            { nullptr, nullptr }
        };

        std::string page;
        if (!keyword.empty())
        {
            // Check direct type name matches
            for (int i = 0; kManualPages[i]; i++)
            {
                if (keyword == kManualPages[i])
                {
                    page = keyword;
                    break;
                }
            }
            // Check function name matches
            if (page.empty())
            {
                for (int i = 0; kFunctionPages[i].keyword; i++)
                {
                    if (keyword == kFunctionPages[i].keyword)
                    {
                        page = kFunctionPages[i].page;
                        break;
                    }
                }
            }
        }

        std::string url = "https://adventuregamestudio.github.io/ags-manual/";
        if (!page.empty())
            url += page + ".html";

#if defined(__linux__)
        std::string cmd = "xdg-open " + url + " &";
        system(cmd.c_str());
#elif defined(__APPLE__)
        std::string cmd = "open " + url;
        system(cmd.c_str());
#elif defined(_WIN32)
        std::string cmd = "start " + url;
        system(cmd.c_str());
#endif
        if (!page.empty())
            log_panel_->AddLog("[Info] Opening AGS manual: %s (F1).", keyword.c_str());
        else
            log_panel_->AddLog("[Info] Opening AGS manual (F1).");
    }

    // Draw the main menu bar first
    menu_bar_->Draw();

    // Get the available area below the menu bar
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 work_pos = viewport->WorkPos;
    ImVec2 work_size = viewport->WorkSize;

    // Layout dimensions (from preferences, but interactively resizable)
    auto& prefs = app_.GetPreferences();
    float left_panel_width = work_size.x * prefs.left_panel_pct;
    float right_panel_width = work_size.x * prefs.right_panel_pct;
    float center_width = work_size.x - left_panel_width - right_panel_width;
    float bottom_panel_height = work_size.y * prefs.bottom_panel_pct;
    float center_height = work_size.y - bottom_panel_height;

    // Minimum panel sizes
    const float kMinPanelW = 100.0f;
    const float kMinPanelH = 60.0f;
    const float kSplitterThickness = 4.0f;

    // --- Left panel - Project Tree ---
    ImGui::SetNextWindowPos(ImVec2(work_pos.x, work_pos.y));
    ImGui::SetNextWindowSize(ImVec2(left_panel_width, work_size.y));
    project_panel_->Draw();

    // --- Left splitter (between project panel and center) ---
    {
        ImVec2 splitter_pos(work_pos.x + left_panel_width, work_pos.y);
        ImVec2 splitter_size(kSplitterThickness, work_size.y);
        ImGui::SetCursorScreenPos(splitter_pos);
        ImGui::InvisibleButton("##splitter_left", splitter_size);
        if (ImGui::IsItemHovered())
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        if (ImGui::IsItemActive())
        {
            float delta = ImGui::GetIO().MouseDelta.x;
            float new_w = left_panel_width + delta;
            if (new_w >= kMinPanelW && (work_size.x - new_w - right_panel_width) >= kMinPanelW)
            {
                prefs.left_panel_pct = new_w / work_size.x;
            }
        }
    }

    // --- Center area - Tabbed editor panes (top portion) ---
    ImGui::SetNextWindowPos(ImVec2(work_pos.x + left_panel_width + kSplitterThickness, work_pos.y));
    ImGui::SetNextWindowSize(ImVec2(center_width - 2 * kSplitterThickness, center_height - kSplitterThickness));
    DrawCenterPane();

    // --- Right splitter (between center and properties panel) ---
    {
        ImVec2 splitter_pos(work_pos.x + left_panel_width + center_width - kSplitterThickness, work_pos.y);
        ImVec2 splitter_size(kSplitterThickness, work_size.y);
        ImGui::SetCursorScreenPos(splitter_pos);
        ImGui::InvisibleButton("##splitter_right", splitter_size);
        if (ImGui::IsItemHovered())
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        if (ImGui::IsItemActive())
        {
            float delta = ImGui::GetIO().MouseDelta.x;
            float new_w = right_panel_width - delta;
            if (new_w >= kMinPanelW && (work_size.x - left_panel_width - new_w) >= kMinPanelW)
            {
                prefs.right_panel_pct = new_w / work_size.x;
            }
        }
    }

    // --- Right panel - Properties ---
    ImGui::SetNextWindowPos(ImVec2(work_pos.x + left_panel_width + center_width, work_pos.y));
    ImGui::SetNextWindowSize(ImVec2(right_panel_width, work_size.y));
    properties_panel_->Draw();

    // --- Bottom splitter (between center and log panel) ---
    {
        ImVec2 splitter_pos(work_pos.x + left_panel_width + kSplitterThickness,
                            work_pos.y + center_height - kSplitterThickness);
        ImVec2 splitter_size(center_width - 2 * kSplitterThickness, kSplitterThickness);
        ImGui::SetCursorScreenPos(splitter_pos);
        ImGui::InvisibleButton("##splitter_bottom", splitter_size);
        if (ImGui::IsItemHovered())
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
        if (ImGui::IsItemActive())
        {
            float delta = ImGui::GetIO().MouseDelta.y;
            float new_h = bottom_panel_height - delta;
            if (new_h >= kMinPanelH && (work_size.y - new_h) >= kMinPanelH)
            {
                prefs.bottom_panel_pct = new_h / work_size.y;
            }
        }
    }

    // --- Bottom panel - Log (below center) ---
    ImGui::SetNextWindowPos(ImVec2(work_pos.x + left_panel_width + kSplitterThickness, work_pos.y + center_height));
    ImGui::SetNextWindowSize(ImVec2(center_width - 2 * kSplitterThickness, bottom_panel_height));
    log_panel_->Draw();

    // Remove closed panes
    panes_.erase(
        std::remove_if(panes_.begin(), panes_.end(),
            [](const std::unique_ptr<EditorPane>& p) { return !p->IsOpen(); }),
        panes_.end());
}

void EditorUI::DrawCenterPane()
{
    ImGui::Begin("Editor", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings);

    if (panes_.empty())
    {
        ImGui::TextDisabled("No editor panes open.");
        ImGui::TextDisabled("Use File > Open or the Project tree to open content.");
    }
    else if (ImGui::BeginTabBar("EditorTabBar",
        ImGuiTabBarFlags_Reorderable |
        ImGuiTabBarFlags_AutoSelectNewTabs |
        ImGuiTabBarFlags_FittingPolicyScroll))
    {
        for (size_t i = 0; i < panes_.size(); i++)
        {
            auto& pane = panes_[i];
            if (!pane->IsOpen())
                continue;

            // Use pane ID to disambiguate tabs with the same title
            char tab_label[256];
            if (pane->IsModified())
                snprintf(tab_label, sizeof(tab_label), "%s *###pane_%d",
                         pane->GetTitle(), pane->GetPaneId());
            else
                snprintf(tab_label, sizeof(tab_label), "%s###pane_%d",
                         pane->GetTitle(), pane->GetPaneId());

            // Check if we need to programmatically focus this tab
            ImGuiTabItemFlags tab_flags = ImGuiTabItemFlags_None;
            if (pending_focus_pane_id_ == pane->GetPaneId())
            {
                tab_flags |= ImGuiTabItemFlags_SetSelected;
                pending_focus_pane_id_ = -1;
            }

            bool open = true;
            if (ImGui::BeginTabItem(tab_label, &open, tab_flags))
            {
                active_pane_index_ = (int)i;
                pane->Draw();
                ImGui::EndTabItem();
            }
            if (!open)
                pane->Close();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}

void EditorUI::OpenPane(std::unique_ptr<EditorPane> pane)
{
    if (pane)
        panes_.emplace_back(std::move(pane));
}

void EditorUI::OpenScriptFile(const std::string& filename)
{
    // Check if a ScriptEditor for this file is already open
    for (size_t i = 0; i < panes_.size(); i++)
    {
        auto* se = dynamic_cast<ScriptEditor*>(panes_[i].get());
        if (se && se->GetFilename() == filename)
        {
            // Already open — just activate that tab
            active_pane_index_ = (int)i;
            pending_focus_pane_id_ = panes_[i]->GetPaneId();
            return;
        }
    }
    // Not found — create a new ScriptEditor for this file
    OpenPane(std::make_unique<ScriptEditor>(*this, filename));
}

RoomEditor* EditorUI::OpenRoomEditor(int room_number)
{
    // Check if a RoomEditor for this room number is already open
    for (size_t i = 0; i < panes_.size(); i++)
    {
        auto* re = dynamic_cast<RoomEditor*>(panes_[i].get());
        if (re && re->GetRoomNumber() == room_number)
        {
            active_pane_index_ = (int)i;
            pending_focus_pane_id_ = panes_[i]->GetPaneId();
            return re;
        }
    }
    // Not found — create a new RoomEditor
    auto pane = std::make_unique<RoomEditor>(*this, room_number);
    RoomEditor* ptr = pane.get();
    OpenPane(std::move(pane));
    return ptr;
}

void EditorUI::ClosePaneByRoomNumber(int room_number)
{
    for (size_t i = 0; i < panes_.size(); i++)
    {
        auto* re = dynamic_cast<RoomEditor*>(panes_[i].get());
        if (re && re->GetRoomNumber() == room_number)
        {
            panes_.erase(panes_.begin() + i);
            if (active_pane_index_ >= (int)panes_.size())
                active_pane_index_ = (int)panes_.size() - 1;
            return;
        }
    }
}

void EditorUI::ClosePaneByScriptFilename(const std::string& filename)
{
    for (size_t i = 0; i < panes_.size(); i++)
    {
        auto* se = dynamic_cast<ScriptEditor*>(panes_[i].get());
        if (se && se->GetFilename() == filename)
        {
            panes_.erase(panes_.begin() + i);
            if (active_pane_index_ >= (int)panes_.size())
                active_pane_index_ = (int)panes_.size() - 1;
            return;
        }
    }
}

void EditorUI::OpenWelcomePane()
{
    OpenOrFocusPane<WelcomePane>(*this);
}

void EditorUI::NotifyProjectLoaded()
{
    // Close all existing editor panes that may reference old game data
    panes_.clear();
    active_pane_index_ = -1;
    pending_focus_pane_id_ = -1;

    // Clear cached GPU textures from the previous game
    app_.GetTextureCache().Clear();

    // Open a fresh welcome pane for the new project
    OpenWelcomePane();

    // Rebuild project tree for the new game data
    project_panel_->RebuildTree();

    // Update window title to show project name
    auto* project = app_.GetProject();
    if (project && project->IsLoaded())
    {
        std::string title = project->GetGameTitle() + " - AGS Editor";
        SDL_SetWindowTitle(app_.GetWindow(), title.c_str());
    }
    else
    {
        SDL_SetWindowTitle(app_.GetWindow(), "AGS Editor");
    }
}

void EditorUI::NotifyProjectClosed()
{
    // Close all open editor panes
    panes_.clear();
    active_pane_index_ = -1;
    pending_focus_pane_id_ = -1;

    // Open welcome pane
    OpenWelcomePane();

    // Rebuild project tree (will show empty tree since project data is cleared)
    project_panel_->RebuildTree();

    // Reset window title
    SDL_SetWindowTitle(app_.GetWindow(), "AGS Editor");
}

Project* EditorUI::GetProject() const
{
    return app_.GetProject();
}

} // namespace AGSEditor
