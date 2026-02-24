// AGS Editor ImGui - New Game Wizard Dialog implementation
#include "new_game_dialog.h"
#include "imgui.h"
#include "ui/file_dialog.h"
#include "core/dpi_helper.h"
#include "project/template_manager.h"

#include <cstring>
#include <filesystem>

namespace AGSEditor
{

// Resolution presets: name, width, height
static const struct { const char* name; int w; int h; } kResolutions[] = {
    { "320 x 200 (classic low-res)",    320,  200 },
    { "320 x 240 (classic 4:3)",        320,  240 },
    { "640 x 480 (standard)",           640,  480 },
    { "800 x 600 (SVGA)",              800,  600 },
    { "1024 x 768 (XGA)",             1024,  768 },
    { "1280 x 720 (HD 720p)",         1280,  720 },
    { "1920 x 1080 (Full HD)",        1920, 1080 },
    { "Custom...",                        0,    0 },
};
static const int kNumResolutions = sizeof(kResolutions) / sizeof(kResolutions[0]);

static const char* kColorDepths[] = { "8-bit (256 colors)", "16-bit (High Color)", "32-bit (True Color)" };
static const int kColorDepthBits[] = { 8, 16, 32 };

int NewGameSettings::GetWidth() const
{
    if (resolution_type < kNumResolutions - 1)
        return kResolutions[resolution_type].w;
    return custom_width;
}

int NewGameSettings::GetHeight() const
{
    if (resolution_type < kNumResolutions - 1)
        return kResolutions[resolution_type].h;
    return custom_height;
}

int NewGameSettings::GetColorDepthBits() const
{
    if (color_depth >= 0 && color_depth < 3)
        return kColorDepthBits[color_depth];
    return 32;
}

// Static members
bool NewGameDialog::open_ = false;
int NewGameDialog::current_page_ = 0;
NewGameSettings NewGameDialog::settings_;
NewGameDialog::Callback NewGameDialog::callback_;
const std::vector<GameTemplate>* NewGameDialog::templates_ = nullptr;

void NewGameDialog::Open(Callback on_create,
                         const std::vector<GameTemplate>* templates)
{
    open_ = true;
    current_page_ = 0;
    settings_ = NewGameSettings();
    callback_ = on_create;
    templates_ = templates;

    // Set default project directory to current working directory
    {
        std::string default_dir = std::filesystem::current_path().string() + "/MyGame";
        strncpy(settings_.project_dir, default_dir.c_str(), sizeof(settings_.project_dir) - 1);
    }
}

void NewGameDialog::Draw()
{
    if (!open_)
        return;

    ImGui::OpenPopup("New Game Wizard");

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(DpiVec(550, 450), ImGuiCond_Appearing);

    if (ImGui::BeginPopupModal("New Game Wizard", &open_, ImGuiWindowFlags_NoResize))
    {
        // Page indicator
        const char* pages[] = { "Basic", "Graphics", "Template", "Summary" };
        for (int i = 0; i < 4; i++)
        {
            if (i > 0) ImGui::SameLine();
            bool current = (i == current_page_);
            if (current)
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));
            ImGui::Text("%s%s", pages[i], (i < 3) ? " >" : "");
            if (current)
                ImGui::PopStyleColor();
        }
        ImGui::Separator();

        // Content area
        ImGui::BeginChild("WizardContent", ImVec2(0, -Dpi(40)));
        switch (current_page_)
        {
        case 0: DrawPageBasic();     break;
        case 1: DrawPageGraphics();  break;
        case 2: DrawPageTemplate();  break;
        case 3: DrawPageSummary();   break;
        }
        ImGui::EndChild();

        ImGui::Separator();

        // Navigation buttons
        if (current_page_ > 0)
        {
            if (ImGui::Button("< Back"))
                current_page_--;
            ImGui::SameLine();
        }

        float button_width = Dpi(80);
        if (current_page_ < 3)
        {
            float offset = ImGui::GetContentRegionAvail().x - button_width;
            if (current_page_ > 0)
                offset -= button_width + ImGui::GetStyle().ItemSpacing.x;
            ImGui::SameLine(0, offset);
            if (ImGui::Button("Next >", ImVec2(button_width, 0)))
                current_page_++;
        }
        else
        {
            float offset = ImGui::GetContentRegionAvail().x - button_width * 2 - ImGui::GetStyle().ItemSpacing.x;
            ImGui::SameLine(0, offset);
            if (ImGui::Button("Create", ImVec2(button_width, 0)))
            {
                if (callback_)
                    callback_(settings_);
                open_ = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(button_width, 0)))
            {
                open_ = false;
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::EndPopup();
    }
}

bool NewGameDialog::IsOpen()
{
    return open_;
}

void NewGameDialog::DrawPageBasic()
{
    ImGui::TextWrapped("Welcome to the New Game Wizard. Enter a title and choose where to save your project.");
    ImGui::Spacing();
    ImGui::Spacing();

    ImGui::Text("Game Title:");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##GameTitle", settings_.game_title, sizeof(settings_.game_title));

    ImGui::Spacing();
    ImGui::Spacing();

    ImGui::Text("Project Folder:");
    ImGui::SetNextItemWidth(-Dpi(80));
    ImGui::InputText("##ProjectDir", settings_.project_dir, sizeof(settings_.project_dir));
    ImGui::SameLine();
    if (ImGui::Button("Browse..."))
    {
        FileDialog::Open(FileDialogType::SelectFolder, "Select Project Folder", "",
            settings_.project_dir,
            [](const std::string& path) {
                strncpy(settings_.project_dir, path.c_str(), sizeof(settings_.project_dir) - 1);
            });
    }
}

void NewGameDialog::DrawPageGraphics()
{
    ImGui::TextWrapped("Choose the resolution and color depth for your game.");
    ImGui::Spacing();
    ImGui::Spacing();

    ImGui::Text("Game Resolution:");
    for (int i = 0; i < kNumResolutions; i++)
    {
        ImGui::RadioButton(kResolutions[i].name, &settings_.resolution_type, i);
    }

    // Custom resolution inputs
    if (settings_.resolution_type == kNumResolutions - 1)
    {
        ImGui::Indent();
        ImGui::SetNextItemWidth(Dpi(100));
        ImGui::InputInt("Width##Custom", &settings_.custom_width);
        ImGui::SetNextItemWidth(Dpi(100));
        ImGui::InputInt("Height##Custom", &settings_.custom_height);
        if (settings_.custom_width < 64) settings_.custom_width = 64;
        if (settings_.custom_height < 64) settings_.custom_height = 64;
        if (settings_.custom_width > 7680) settings_.custom_width = 7680;
        if (settings_.custom_height > 4320) settings_.custom_height = 4320;
        ImGui::Unindent();
    }

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Text("Color Depth:");
    ImGui::Combo("##ColorDepth", &settings_.color_depth, kColorDepths, 3);
}

void NewGameDialog::DrawPageTemplate()
{
    ImGui::TextWrapped("Choose a game template. Templates provide pre-made scripts and room setups to get started quickly.");
    ImGui::Spacing();
    ImGui::Spacing();

    if (templates_ && !templates_->empty())
    {
        // Real templates from TemplateManager
        ImGui::Text("Template:");
        for (int i = 0; i < (int)templates_->size(); i++)
        {
            const auto& tmpl = (*templates_)[i];
            ImGui::RadioButton(tmpl.friendly_name.c_str(), &settings_.template_index, i);
        }

        // Clamp index
        if (settings_.template_index >= (int)templates_->size())
            settings_.template_index = 0;

        settings_.selected_template = &(*templates_)[settings_.template_index];

        ImGui::Spacing();
        ImGui::Spacing();

        // Show description
        const auto& sel = (*templates_)[settings_.template_index];
        if (!sel.description.empty())
            ImGui::TextWrapped("Description: %s", sel.description.c_str());
        else
            ImGui::TextWrapped("Template: %s", sel.friendly_name.c_str());
    }
    else
    {
        // Fallback: hardcoded template names (no .agt files found)
        ImGui::Text("Template:");
        static const char* kTemplates[] = {
            "Empty Game (blank project)",
            "Sierra-style (verb coin)",
            "LucasArts-style (9-verb)",
            "Minimalist (1-click)",
        };
        static const int kNumTemplates = 4;
        for (int i = 0; i < kNumTemplates; i++)
            ImGui::RadioButton(kTemplates[i], &settings_.template_index, i);

        settings_.selected_template = nullptr;

        ImGui::Spacing();
        ImGui::Spacing();

        static const char* descriptions[] = {
            "An empty game with no pre-made content. You start from scratch.",
            "A Sierra/verb-coin style interface with walk/look/interact/talk cursors.",
            "A LucasArts-style interface with a 9-verb action panel at the bottom.",
            "A minimalist 1-click interface. Left-click interacts, right-click looks.",
        };
        ImGui::TextWrapped("Description: %s", descriptions[settings_.template_index]);

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f),
            "No .agt template files found. Place templates in a Templates/ folder "
            "next to the editor executable.");
    }
}

void NewGameDialog::DrawPageSummary()
{
    ImGui::TextWrapped("Review your settings and click 'Create' to create the new game.");
    ImGui::Spacing();
    ImGui::Spacing();

    ImGui::Text("Game Title:    %s", settings_.game_title);
    ImGui::Text("Project Dir:   %s", settings_.project_dir);
    ImGui::Text("Resolution:    %d x %d", settings_.GetWidth(), settings_.GetHeight());
    ImGui::Text("Color Depth:   %d-bit", settings_.GetColorDepthBits());

    if (settings_.selected_template)
        ImGui::Text("Template:      %s", settings_.selected_template->friendly_name.c_str());
    else
        ImGui::Text("Template:      Empty Game (built-in)");
}

} // namespace AGSEditor
