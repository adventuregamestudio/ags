// AGS Editor ImGui - Import Old Game Dialog implementation
#include "import_game_dialog.h"
#include "imgui.h"
#include "ui/file_dialog.h"
#include "core/dpi_helper.h"
#include "ui/IconsLucide.h"

#include <cstring>
#include <filesystem>

namespace fs = std::filesystem;

namespace AGSEditor
{

// Static members
bool ImportGameDialog::open_ = false;
int ImportGameDialog::current_page_ = 0;
ImportOptions ImportGameDialog::options_;
ImportGameDialog::Callback ImportGameDialog::callback_;
bool ImportGameDialog::show_results_ = false;
ImportResult ImportGameDialog::last_result_;

void ImportGameDialog::Open(Callback on_import)
{
    open_ = true;
    current_page_ = 0;
    options_ = ImportOptions();
    callback_ = on_import;
    show_results_ = false;
    last_result_ = ImportResult();
}

void ImportGameDialog::Draw()
{
    if (!open_ && !show_results_)
        return;

    // Results popup (shown after import)
    if (show_results_)
    {
        ImGui::OpenPopup("Import Results");
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(DpiVec(500, 400), ImGuiCond_Appearing);

        if (ImGui::BeginPopupModal("Import Results", &show_results_,
                                   ImGuiWindowFlags_NoResize))
        {
            DrawPageResult();

            ImGui::Separator();
            float bw = Dpi(80);
            float offset = ImGui::GetContentRegionAvail().x - bw;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);
            if (ImGui::Button("OK", ImVec2(bw, 0)))
            {
                show_results_ = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        return;
    }

    // Main wizard dialog
    ImGui::OpenPopup("Import Old Game");

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(DpiVec(550, 380), ImGuiCond_Appearing);

    if (ImGui::BeginPopupModal("Import Old Game", &open_, ImGuiWindowFlags_NoResize))
    {
        // Page indicator
        const char* pages[] = { "Select File", "Options" };
        for (int i = 0; i < 2; i++)
        {
            if (i > 0) ImGui::SameLine();
            bool current = (i == current_page_);
            if (current)
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));
            ImGui::Text("%s%s", pages[i], (i < 1) ? " >" : "");
            if (current)
                ImGui::PopStyleColor();
        }
        ImGui::Separator();

        // Content area
        ImGui::BeginChild("ImportWizContent", ImVec2(0, -Dpi(40)));
        switch (current_page_)
        {
        case 0: DrawPageSelectFile(); break;
        case 1: DrawPageOptions(); break;
        }
        ImGui::EndChild();

        ImGui::Separator();

        // Navigation buttons
        float bw = Dpi(80);

        if (current_page_ > 0)
        {
            if (ImGui::Button("< Back"))
                current_page_--;
            ImGui::SameLine();
        }

        if (current_page_ < 1)
        {
            float offset = ImGui::GetContentRegionAvail().x - bw;
            ImGui::SameLine(0, offset);
            bool can_next = !options_.game_file_path.empty() &&
                            fs::exists(options_.game_file_path);
            if (!can_next) ImGui::BeginDisabled();
            if (ImGui::Button("Next >", ImVec2(bw, 0)))
                current_page_++;
            if (!can_next) ImGui::EndDisabled();
        }
        else
        {
            float offset = ImGui::GetContentRegionAvail().x -
                           bw * 2 - ImGui::GetStyle().ItemSpacing.x;
            ImGui::SameLine(0, offset);

            if (ImGui::Button("Import", ImVec2(bw, 0)))
            {
                if (callback_)
                    callback_(options_);
                open_ = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(bw, 0)))
            {
                open_ = false;
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::EndPopup();
    }
}

void ImportGameDialog::DrawPageSelectFile()
{
    ImGui::TextWrapped(
        "Import an old AGS game (pre-3.x / AGS 2.72 format). "
        "Select the main game data file (ac2game.dta, game.ags, or game28.dta) "
        "from the game's directory.");

    ImGui::Spacing();
    ImGui::Spacing();

    // Path input
    ImGui::Text("Game file:");
    static char path_buf[512] = {};
    // Sync static buffer with options
    if (options_.game_file_path.size() < sizeof(path_buf))
        strncpy(path_buf, options_.game_file_path.c_str(), sizeof(path_buf) - 1);

    float browse_w = Dpi(80);
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - browse_w -
                            ImGui::GetStyle().ItemSpacing.x);
    if (ImGui::InputText("##GameFile", path_buf, sizeof(path_buf)))
        options_.game_file_path = path_buf;

    ImGui::SameLine();
    if (ImGui::Button(ICON_LC_FOLDER_OPEN " Browse", ImVec2(browse_w, 0)))
    {
        FileDialog::Open(FileDialogType::OpenFile, "Select Old AGS Game File",
            ".dta,.ags{AGS Game Files (ac2game.dta, game.ags)}",
            ".",
            [](const std::string& path) {
                options_.game_file_path = path;
            });
    }

    ImGui::Spacing();

    // Show info about the selected file
    if (!options_.game_file_path.empty())
    {
        if (fs::exists(options_.game_file_path))
        {
            std::string dir = fs::path(options_.game_file_path).parent_path().string();
            bool has_editor_dat = fs::exists(dir + "/editor.dat");
            bool has_sprites = fs::exists(dir + "/acsprset.spr");

            ImGui::Separator();
            ImGui::Text("File info:");
            ImGui::BulletText("Directory: %s", dir.c_str());
            ImGui::BulletText("editor.dat: %s",
                has_editor_dat ? "Found (AGS 2.72 metadata)" : "Not found");
            ImGui::BulletText("Sprite file: %s",
                has_sprites ? "Found" : "Not found");

            if (!has_editor_dat)
            {
                ImGui::Spacing();
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.2f, 1.0f));
                ImGui::TextWrapped(
                    ICON_LC_TRIANGLE_ALERT " No editor.dat found. Script modules "
                    "and sprite folder structure will not be imported. "
                    "The game binary data will still be read.");
                ImGui::PopStyleColor();
            }
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
            ImGui::Text(ICON_LC_CIRCLE_X " File not found: %s",
                        options_.game_file_path.c_str());
            ImGui::PopStyleColor();
        }
    }
}

void ImportGameDialog::DrawPageOptions()
{
    ImGui::TextWrapped("Configure import options before proceeding.");
    ImGui::Spacing();

    // Backup option
    ImGui::Checkbox("Create backup before importing", &options_.create_backup);
    if (options_.create_backup)
    {
        ImGui::Indent();
        ImGui::TextWrapped(
            "A copy of the entire game directory will be saved to a "
            "\"Backup\" folder next to the game directory.");

        static char backup_buf[512] = {};
        if (options_.backup_dir.size() < sizeof(backup_buf))
            strncpy(backup_buf, options_.backup_dir.c_str(), sizeof(backup_buf) - 1);

        ImGui::Text("Custom backup path (leave empty for default):");
        if (ImGui::InputText("##BackupDir", backup_buf, sizeof(backup_buf)))
            options_.backup_dir = backup_buf;
        ImGui::Unindent();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // editor.dat option
    ImGui::Checkbox("Import editor.dat metadata", &options_.import_editor_dat);
    ImGui::Indent();
    ImGui::TextWrapped(
        "If editor.dat is present, reads script modules, sprite folder "
        "structure, and room descriptions from the AGS 2.72 editor metadata.");
    ImGui::Unindent();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Summary of what will happen
    ImGui::Text("The import will:");
    ImGui::BulletText("Read game data (characters, views, dialogs, GUIs, etc.)");
    ImGui::BulletText("Load sprite information from acsprset.spr");
    ImGui::BulletText("Read room data from .crm files");
    if (options_.import_editor_dat)
    {
        ImGui::BulletText("Extract scripts from editor.dat as .asc/.ash files");
        ImGui::BulletText("Import sprite folder structure");
    }
    if (options_.create_backup)
        ImGui::BulletText("Create a backup of the original files");
}

void ImportGameDialog::DrawPageResult()
{
    if (last_result_.success)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 1.0f, 0.2f, 1.0f));
        ImGui::Text(ICON_LC_CIRCLE_CHECK " Import completed successfully!");
        ImGui::PopStyleColor();
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
        ImGui::Text(ICON_LC_CIRCLE_X " Import failed.");
        ImGui::PopStyleColor();
        if (!last_result_.error_message.empty())
        {
            ImGui::TextWrapped("Error: %s", last_result_.error_message.c_str());
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (last_result_.success)
    {
        ImGui::Text("Imported:");
        ImGui::BulletText("Characters: %d", last_result_.characters_imported);
        ImGui::BulletText("Views: %d", last_result_.views_imported);
        ImGui::BulletText("Dialogs: %d", last_result_.dialogs_imported);
        ImGui::BulletText("GUIs: %d", last_result_.guis_imported);
        ImGui::BulletText("Rooms: %d", last_result_.rooms_imported);
        ImGui::BulletText("Sprites: %d", last_result_.sprites_imported);

        if (last_result_.had_editor_dat)
        {
            ImGui::BulletText("Script modules: %d", last_result_.script_modules_imported);
            ImGui::BulletText("editor.dat: Yes");
        }
        else
        {
            ImGui::BulletText("editor.dat: Not available");
        }
    }

    // Warnings
    if (!last_result_.warnings.empty())
    {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.2f, 1.0f));
        ImGui::Text(ICON_LC_TRIANGLE_ALERT " Warnings:");
        ImGui::PopStyleColor();

        for (const auto& w : last_result_.warnings)
        {
            ImGui::BulletText("%s", w.c_str());
        }
    }
}

void ImportGameDialog::ShowResults(const ImportResult& result)
{
    last_result_ = result;
    show_results_ = true;
}

} // namespace AGSEditor
