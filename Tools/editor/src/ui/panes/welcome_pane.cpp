// AGS Editor ImGui - Welcome Pane implementation
#include "welcome_pane.h"
#include "imgui.h"
#include "core/dpi_helper.h"
#include "app.h"
#include "project/project.h"
#include "ui/file_dialog.h"
#include "ui/dialogs/new_game_dialog.h"
#include "ui/log_panel.h"
#include <filesystem>

namespace AGSEditor
{

WelcomePane::WelcomePane(EditorUI& editor)
    : editor_(editor)
{
}

void WelcomePane::Draw()
{
    ImGui::Spacing();

    float avail_width = ImGui::GetContentRegionAvail().x;
    float content_width = Dpi(600);
    if (avail_width > content_width)
        ImGui::SetCursorPosX((avail_width - content_width) * 0.5f);

    ImGui::BeginGroup();

    // -- Title banner --
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.9f, 1.0f, 1.0f));
    ImGui::SetWindowFontScale(1.5f);
    ImGui::Text("Adventure Game Studio");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();

    ImGui::TextColored(ImVec4(0.6f, 0.7f, 0.8f, 1.0f),
        "Cross-Platform Editor (Dear ImGui Edition)");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // -- Welcome text --
    ImGui::Text("Welcome to AGS! What do you want to do?");
    ImGui::Spacing();
    ImGui::Spacing();

    // Two sections side by side: Actions (left) + Recent Games (right)
    float section_width = Dpi(280);
    float spacing = Dpi(30);

    // -- Left column: action buttons --
    ImGui::BeginChild("##WelcomeActions", ImVec2(section_width, Dpi(320)), false);
    DrawActionButtons();
    ImGui::EndChild();

    ImGui::SameLine(0, spacing);

    // -- Right column: recent games --
    ImGui::BeginChild("##WelcomeRecent", ImVec2(section_width, Dpi(320)), false);
    DrawRecentGames();
    ImGui::EndChild();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // -- Feature list --
    DrawFeatureList();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::TextDisabled("AGS v3.6.3 | Dear ImGui | SDL2 + SDL_Renderer");

    ImGui::EndGroup();
}

void WelcomePane::DrawActionButtons()
{
    ImVec2 btn_size(Dpi(260), Dpi(36));

    ImGui::TextColored(ImVec4(0.6f, 0.8f, 0.6f, 1.0f), "Get Started:");
    ImGui::Spacing();

    // Start a new game
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.40f, 0.20f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.55f, 0.25f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.30f, 0.65f, 0.30f, 1.0f));
    if (ImGui::Button("Start a New Game...", btn_size))
        ActionNewGame();
    ImGui::PopStyleColor(3);
    ImGui::TextWrapped("Create a new adventure game project from scratch or a template.");
    ImGui::Spacing();
    ImGui::Spacing();

    // Open existing game
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.25f, 0.45f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.35f, 0.55f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.30f, 0.40f, 0.65f, 1.0f));
    if (ImGui::Button("Open an Existing Game...", btn_size))
        ActionOpenGame();
    ImGui::PopStyleColor(3);
    ImGui::TextWrapped("Browse for a Game.agf project file to open.");
    ImGui::Spacing();
    ImGui::Spacing();

    // Quick start hints
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Shortcuts:");
    ImGui::BulletText("Ctrl+N  New Game");
    ImGui::BulletText("Ctrl+O  Open Game");
    ImGui::BulletText("F5  Build & Run");
    ImGui::BulletText("F7  Build Only");
}

void WelcomePane::DrawRecentGames()
{
    ImGui::TextColored(ImVec4(0.6f, 0.8f, 0.6f, 1.0f), "Recent Games:");
    ImGui::Spacing();

    auto* project = editor_.GetApp().GetProject();
    if (!project)
    {
        ImGui::TextDisabled("(Project system not available)");
        return;
    }

    const auto& recent = project->GetRecentFiles().GetFiles();
    if (recent.empty())
    {
        ImGui::TextDisabled("No recent games.");
        ImGui::Spacing();
        ImGui::TextWrapped(
            "Games you open will appear here for quick access.");
        return;
    }

    // Scrollable list of recent games
    float list_height = Dpi(260);
    if (ImGui::BeginChild("##RecentList", ImVec2(0, list_height),
        ImGuiChildFlags_Borders))
    {
        namespace fs = std::filesystem;
        for (size_t i = 0; i < recent.size(); i++)
        {
            const auto& path = recent[i];

            // Extract project name (parent folder name or filename)
            fs::path p(path);
            std::string display_name;
            if (p.has_parent_path())
                display_name = p.parent_path().filename().string();
            else
                display_name = p.filename().string();

            // Check if the path still exists
            bool exists = fs::exists(p);

            ImGui::PushID((int)i);

            if (!exists)
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.4f, 0.4f, 1.0f));

            float item_height = Dpi(42);

            // Invisible selectable for click hit-area
            if (ImGui::Selectable("##entry", false,
                ImGuiSelectableFlags_AllowDoubleClick,
                ImVec2(0, item_height)))
            {
                if (exists)
                    ActionOpenRecentGame(path);
            }
            bool hovered = ImGui::IsItemHovered();

            // Draw text content directly via the draw list over the selectable
            ImVec2 item_min = ImGui::GetItemRectMin();
            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImFont* font = ImGui::GetFont();
            float font_size = ImGui::GetFontSize();

            ImU32 name_col = !exists
                ? ImGui::GetColorU32(ImVec4(0.5f, 0.4f, 0.4f, 1.0f))
                : ImGui::GetColorU32(ImGuiCol_Text);
            ImU32 path_col = ImGui::GetColorU32(ImVec4(0.5f, 0.5f, 0.5f, 1.0f));

            std::string name_text = !exists
                ? display_name + " (missing)" : display_name;

            dl->AddText(font, font_size,
                ImVec2(item_min.x + Dpi(8), item_min.y + Dpi(2)),
                name_col, name_text.c_str());
            dl->AddText(font, font_size,
                ImVec2(item_min.x + Dpi(8), item_min.y + Dpi(2) + font_size + Dpi(2)),
                path_col, path.c_str());

            if (!exists)
                ImGui::PopStyleColor();

            if (hovered && exists)
                ImGui::SetTooltip("Click to open: %s", path.c_str());

            ImGui::PopID();
        }
    }
    ImGui::EndChild();
}

void WelcomePane::DrawFeatureList()
{
    ImGui::TextColored(ImVec4(0.6f, 0.8f, 0.6f, 1.0f), "Available Features:");
    ImGui::Spacing();

    ImGui::Columns(2, "featurecols", false);
    ImGui::BulletText("Script Editor");
    ImGui::BulletText("Room Editor");
    ImGui::BulletText("Sprite Manager");
    ImGui::BulletText("Character Editor");
    ImGui::BulletText("Dialog Editor");
    ImGui::NextColumn();
    ImGui::BulletText("GUI Editor");
    ImGui::BulletText("View/Animation Editor");
    ImGui::BulletText("Audio Management");
    ImGui::BulletText("Font Management");
    ImGui::BulletText("Build System");
    ImGui::Columns(1);
}

void WelcomePane::ActionNewGame()
{
    auto& app = editor_.GetApp();
    const auto* templates = &app.GetTemplateManager().GetTemplates();
    NewGameDialog::Open([this](const NewGameSettings& settings) {
        auto& a = editor_.GetApp();
        std::string project_dir = settings.project_dir;
        std::string project_path = project_dir + "/Game.agf";

        if (settings.selected_template && settings.selected_template->valid)
        {
            if (!a.GetTemplateManager().ExtractTemplate(
                    *settings.selected_template, project_dir))
            {
                editor_.GetLogPanel().AddLog("[Error] Failed to extract template: %s",
                    settings.selected_template->friendly_name.c_str());
                return;
            }
            if (a.GetProject()->OpenProject(project_path))
            {
                a.GetProject()->SetGameTitle(settings.game_title);
                a.LoadGameFonts();
                editor_.GetLogPanel().AddLog("[Info] Created new game from template: %s (%s)",
                    settings.game_title,
                    settings.selected_template->friendly_name.c_str());
                editor_.NotifyProjectLoaded();
            }
        }
        else
        {
            if (a.GetProject()->NewProject(
                    project_path, settings.game_title,
                    settings.GetWidth(), settings.GetHeight(),
                    settings.GetColorDepthBits()))
            {
                editor_.GetLogPanel().AddLog("[Info] Created new game: %s (%dx%d, %d-bit)",
                    settings.game_title, settings.GetWidth(),
                    settings.GetHeight(), settings.GetColorDepthBits());
                editor_.NotifyProjectLoaded();
            }
            else
            {
                editor_.GetLogPanel().AddLog("[Error] Failed to create new game project.");
            }
        }
    }, templates);
}

void WelcomePane::ActionOpenGame()
{
    FileDialog::Open(FileDialogType::OpenFile, "Open AGS Game",
        ".agf,.ags,.dta{AGS Game Files}",
        ".",
        [this](const std::string& path) {
            auto& app = editor_.GetApp();
            if (app.GetProject()->OpenProject(path))
            {
                app.LoadGameFonts();
                editor_.GetLogPanel().AddLog("[Info] Opened game: %s",
                    app.GetProject()->GetGameTitle().c_str());
                editor_.NotifyProjectLoaded();
            }
            else
            {
                editor_.GetLogPanel().AddLog("[Error] Failed to open game: %s",
                    path.c_str());
            }
        });
}

void WelcomePane::ActionOpenRecentGame(const std::string& path)
{
    auto& app = editor_.GetApp();
    if (app.GetProject()->OpenProject(path))
    {
        app.LoadGameFonts();
        editor_.GetLogPanel().AddLog("[Info] Opened recent game: %s",
            app.GetProject()->GetGameTitle().c_str());
        editor_.NotifyProjectLoaded();
    }
    else
    {
        editor_.GetLogPanel().AddLog("[Error] Failed to open game: %s",
            path.c_str());
    }
}

} // namespace AGSEditor
