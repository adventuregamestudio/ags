// AGS Editor ImGui - Menu Bar implementation
#include "menu_bar.h"
#include "editor_ui.h"
#include "log_panel.h"
#include "file_dialog.h"
#include "dialogs/new_game_dialog.h"
#include "dialogs/import_game_dialog.h"
#include "panes/room_editor.h"
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
#include "panes/palette_editor.h"
#include "panes/build_pane.h"
#include "panes/preferences_pane.h"
#include "panes/translation_editor.h"
#include "panes/global_variables_editor.h"
#include "panes/custom_properties_editor.h"
#include "panes/text_parser_editor.h"
#include "panes/lip_sync_editor.h"
#include "panes/debug_log_pane.h"
#include "panes/script_editor.h"

#include "core/dpi_helper.h"
#include "project/project.h"
#include "project/game_data.h"
#include "project/old_game_importer.h"
#include "project/sprite_loader.h"
#include "project/texture_cache.h"
#include "core/undo_manager.h"
#include "core/clipboard_manager.h"
#include "project/template_manager.h"
#include "pipeline/speech_tools.h"
#include "app.h"

#include "imgui.h"
#include <SDL.h>

#include "ac/spritefile.h"
#include "gfx/bitmap.h"
#include "gfx/bitmapdata.h"

#include <filesystem>

namespace AGSEditor
{

MenuBar::MenuBar(EditorUI& editor)
    : editor_(editor)
{
}

void MenuBar::Draw()
{
    if (ImGui::BeginMainMenuBar())
    {
        DrawFileMenu();
        DrawEditMenu();
        DrawBuildMenu();
        DrawToolsMenu();
        DrawWindowMenu();
        DrawHelpMenu();

        // Right-aligned status
        float right_margin = ImGui::GetContentRegionAvail().x;
        if (right_margin > Dpi(200))
        {
            ImGui::SameLine(ImGui::GetWindowWidth() - Dpi(180));
            ImGui::TextDisabled("AGS v3.6.3");
        }

        ImGui::EndMainMenuBar();
    }

    // Draw file dialog (if active)
    FileDialog::Draw();

    // Draw new game wizard (if active)
    NewGameDialog::Draw();

    // Draw import dialog (if active)
    ImportGameDialog::Draw();

    // About dialog
    if (show_about_)
    {
        ImGui::OpenPopup("About AGS Editor");
        show_about_ = false;
    }

    // Game Statistics dialog
    if (show_game_stats_)
    {
        ImGui::OpenPopup("Game Statistics");
        show_game_stats_ = false;
    }
    if (ImGui::BeginPopupModal("Game Statistics", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        auto* project = editor_.GetApp().GetProject();
        auto* gd = (project && project->IsLoaded()) ? project->GetGameData() : nullptr;
        if (gd)
        {
            ImGui::Text("Game: %s", gd->game_title.c_str());
            ImGui::Separator();
            ImGui::Text("Sprites:          %d", (int)gd->sprites.size());
            ImGui::Text("Views:            %d", (int)gd->views.size());
            ImGui::Text("Characters:       %d", (int)gd->characters.size());
            ImGui::Text("Dialogs:          %d", (int)gd->dialogs.size());
            ImGui::Text("GUIs:             %d", (int)gd->guis.size());
            ImGui::Text("Inventory Items:  %d", (int)gd->inventory_items.size());
            ImGui::Text("Cursors:          %d", (int)gd->cursors.size());
            ImGui::Text("Fonts:            %d", (int)gd->fonts.size());
            ImGui::Text("Audio Clips:      %d", (int)gd->audio_clips.size());
            ImGui::Text("Rooms:            %d", (int)gd->rooms.size());
            ImGui::Text("Script Modules:   %d", (int)gd->script_modules.size());
        }
        else
        {
            ImGui::TextDisabled("No project loaded.");
        }
        ImGui::Separator();
        if (ImGui::Button("OK", ImVec2(Dpi(120), 0)))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopupModal("About AGS Editor", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.7f, 1.0f, 1.0f));
        ImGui::Text("Adventure Game Studio");
        ImGui::PopStyleColor();
        ImGui::Text("Cross-Platform Editor");
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::Text("Version:  3.6.3 (ImGui Edition)");
        ImGui::Text("UI:       Dear ImGui + SDL2");
        ImGui::Text("Platform: %s", SDL_GetPlatform());
        ImGui::Spacing();
        ImGui::TextWrapped(
            "AGS is an open-source adventure game engine, originally "
            "created by Chris Jones. Licensed under the Artistic License 2.0.");
        ImGui::Spacing();
        ImGui::TextDisabled("https://www.adventuregamestudio.co.uk");
        ImGui::Separator();

        if (ImGui::Button("OK", ImVec2(Dpi(120), 0)))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    // Ctrl+G shortcut for Go to Room
    if (!ImGui::GetIO().WantTextInput &&
        ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_G))
    {
        auto* project = editor_.GetApp().GetProject();
        if (project && project->IsLoaded())
        {
            show_go_to_room_ = true;
            go_to_room_number_ = 0;
        }
    }

    // F4 shortcut: Go to Events section in active pane
    if (!ImGui::GetIO().WantTextInput && ImGui::IsKeyPressed(ImGuiKey_F4))
    {
        int idx = editor_.GetActivePaneIndex();
        if (idx >= 0)
        {
            auto& panes = editor_.GetPanes();
            if (idx < (int)panes.size())
                panes[idx]->FocusEvents();
        }
    }

    // F5 shortcut: Run game
    {
        auto* project = editor_.GetApp().GetProject();
        bool has_project = project && project->IsLoaded();
        if (has_project && !ImGui::GetIO().WantTextInput)
        {
            if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_F5))
            {
                // Ctrl+F5: Debug game
                auto* bp = editor_.OpenOrFocusPane<BuildPane>(editor_);
                bp->StartRun(true);
            }
            else if (ImGui::GetIO().KeyShift && ImGui::IsKeyPressed(ImGuiKey_F5))
            {
                // Shift+F5: Stop game
                auto* bp = editor_.FindPane<BuildPane>();
                if (bp) bp->StopRun();
            }
            else if (ImGui::IsKeyPressed(ImGuiKey_F5))
            {
                // F5: Run game
                auto* bp = editor_.OpenOrFocusPane<BuildPane>(editor_);
                bp->StartRun(false);
            }
        }
    }

    // F7 shortcut: Build game
    {
        auto* project = editor_.GetApp().GetProject();
        bool has_project = project && project->IsLoaded();
        if (has_project && !ImGui::GetIO().WantTextInput && ImGui::IsKeyPressed(ImGuiKey_F7))
        {
            auto* bp = editor_.OpenOrFocusPane<BuildPane>(editor_);
            bp->StartBuild();
        }
    }

    // Go to Room dialog
    if (show_go_to_room_)
    {
        ImGui::OpenPopup("Go to Room");
        show_go_to_room_ = false;
    }
    if (ImGui::BeginPopupModal("Go to Room", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        auto* project = editor_.GetApp().GetProject();
        auto* gd = (project && project->IsLoaded()) ? project->GetGameData() : nullptr;

        ImGui::Text("Enter room number:");
        ImGui::SetNextItemWidth(Dpi(120));
        ImGui::InputInt("##RoomNum", &go_to_room_number_, 1, 10);
        bool enter_pressed = ImGui::IsItemDeactivatedAfterEdit();

        // Auto-focus the input on first frame
        if (ImGui::IsWindowAppearing())
            ImGui::SetKeyboardFocusHere(-1);

        // Show available rooms as a scrollable list
        if (gd && !gd->rooms.empty())
        {
            ImGui::Separator();
            ImGui::Text("Available rooms:");
            ImGui::BeginChild("##RoomList", ImVec2(Dpi(300), Dpi(200)), ImGuiChildFlags_Borders);
            for (const auto& room : gd->rooms)
            {
                char label[128];
                if (room.description.empty())
                    std::snprintf(label, sizeof(label), "Room %d", room.number);
                else
                    std::snprintf(label, sizeof(label), "Room %d - %s", room.number, room.description.c_str());

                bool selected = (room.number == go_to_room_number_);
                if (ImGui::Selectable(label, selected))
                    go_to_room_number_ = room.number;
            }
            ImGui::EndChild();
        }

        // Validate room number
        bool room_exists = false;
        if (gd)
        {
            for (const auto& room : gd->rooms)
            {
                if (room.number == go_to_room_number_)
                {
                    room_exists = true;
                    break;
                }
            }
        }

        if (!room_exists && go_to_room_number_ >= 0)
            ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "Room %d not found", go_to_room_number_);

        ImGui::Separator();
        if ((ImGui::Button("Go", ImVec2(Dpi(80), 0)) || enter_pressed) && room_exists)
        {
            editor_.OpenRoomEditor(go_to_room_number_);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(Dpi(80), 0)))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }

    // Auto-Number Speech Lines result dialog
    if (show_auto_number_result_)
    {
        ImGui::OpenPopup("Auto-Number Speech Lines");
        show_auto_number_result_ = false;
    }
    if (ImGui::BeginPopupModal("Auto-Number Speech Lines", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextWrapped("%s", speech_result_message_.c_str());
        ImGui::Separator();
        if (ImGui::Button("OK", ImVec2(Dpi(120), 0)))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    // Create Voice Acting Script result dialog
    if (show_voice_script_result_)
    {
        ImGui::OpenPopup("Create Voice Acting Script");
        show_voice_script_result_ = false;
    }
    if (ImGui::BeginPopupModal("Create Voice Acting Script", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextWrapped("%s", speech_result_message_.c_str());
        ImGui::Separator();
        if (ImGui::Button("OK", ImVec2(Dpi(120), 0)))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    // Export Global Messages confirmation dialog
    if (show_export_global_messages_)
    {
        ImGui::OpenPopup("Export Global Messages");
        show_export_global_messages_ = false;
    }
    if (ImGui::BeginPopupModal("Export Global Messages", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        auto* project = editor_.GetApp().GetProject();
        auto* gd = (project && project->IsLoaded()) ? project->GetGameData() : nullptr;
        if (gd)
        {
            int count = 0;
            for (int i = 0; i < GameData::NUM_GLOBAL_MESSAGES; i++)
                if (!gd->global_messages[i].empty()) count++;

            ImGui::Text("Found %d non-empty Global Messages.", count);
            ImGui::TextWrapped("This will create a new script module '_GlobalMessages' "
                               "containing all messages as a String array, and then clear "
                               "the Global Messages from the game data.");
            ImGui::Separator();
            if (ImGui::Button("Export & Clear", ImVec2(Dpi(140), 0)))
            {
                // Generate script header
                std::string header;
                header += "// AUTOGENERATED SCRIPT, but is safe to remove if you are not using it.\n";
                header += "// GlobalMessages migrated from AGS 2.x game data\n";
                header += "import String GlobalMessages[500];\n";
                header += "// Gets global message by a classic message number (where they begin at 500th index)\n";
                header += "import String GetMessageByNumber(int message_number);\n";

                // Generate script body
                std::string script;
                script += "// AUTOGENERATED SCRIPT, but is safe to remove if you are not using it.\n";
                script += "// GlobalMessages migrated from AGS 2.x game data\n";
                script += "String GlobalMessages[500];\nexport GlobalMessages;\n";
                script += "String GetMessageByNumber(int message_number){\n";
                script += "  return GlobalMessages[message_number - 500];\n";
                script += "}\n";
                script += "function game_start(){\n";
                script += "  // Initialize global messages\n";
                for (int i = 0; i < GameData::NUM_GLOBAL_MESSAGES; i++)
                {
                    if (!gd->global_messages[i].empty())
                    {
                        // Escape quotes in message text
                        std::string escaped = gd->global_messages[i];
                        for (size_t p = 0; (p = escaped.find('"', p)) != std::string::npos; p += 2)
                            escaped.insert(p, "\\");
                        char buf[64];
                        std::snprintf(buf, sizeof(buf), "  GlobalMessages[%d] = \"", i);
                        script += buf;
                        script += escaped;
                        script += "\";\n";
                    }
                }
                script += "}\n";

                // Find a unique module name
                std::string mod_name = "_GlobalMessages";
                int suffix = 0;
                bool name_taken = true;
                while (name_taken)
                {
                    name_taken = false;
                    for (const auto& m : gd->script_modules)
                    {
                        if (m.name == mod_name)
                        {
                            name_taken = true;
                            suffix++;
                            mod_name = "_GlobalMessages" + std::to_string(suffix);
                            break;
                        }
                    }
                }

                // Create script module
                GameData::ScriptModule mod;
                mod.name = mod_name;
                // Write the script files to disk
                std::string dir = project->GetProjectDir();
                std::string header_file = mod_name + ".ash";
                std::string script_file = mod_name + ".asc";
                {
                    FILE* f = fopen((dir + "/" + header_file).c_str(), "w");
                    if (f) { fputs(header.c_str(), f); fclose(f); }
                }
                {
                    FILE* f = fopen((dir + "/" + script_file).c_str(), "w");
                    if (f) { fputs(script.c_str(), f); fclose(f); }
                }
                mod.header_file = header_file;
                mod.script_file = script_file;
                // Insert at the beginning of the script modules list
                gd->script_modules.insert(gd->script_modules.begin(), mod);

                // Clear global messages
                for (int i = 0; i < GameData::NUM_GLOBAL_MESSAGES; i++)
                    gd->global_messages[i].clear();

                editor_.GetLogPanel().AddLog("[Info] Exported %d Global Messages to script module '%s'.",
                                            count, mod_name.c_str());
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(Dpi(100), 0)))
                ImGui::CloseCurrentPopup();
        }
        else
        {
            ImGui::Text("No project loaded.");
            if (ImGui::Button("OK", ImVec2(Dpi(120), 0)))
                ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // Remove Global Messages confirmation dialog
    if (show_remove_global_messages_)
    {
        ImGui::OpenPopup("Remove Global Messages");
        show_remove_global_messages_ = false;
    }
    if (ImGui::BeginPopupModal("Remove Global Messages", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        auto* project = editor_.GetApp().GetProject();
        auto* gd = (project && project->IsLoaded()) ? project->GetGameData() : nullptr;
        if (gd)
        {
            int count = 0;
            for (int i = 0; i < GameData::NUM_GLOBAL_MESSAGES; i++)
                if (!gd->global_messages[i].empty()) count++;

            ImGui::Text("This will remove all %d Global Messages from the game data.", count);
            ImGui::Text("This action cannot be undone.");
            ImGui::Separator();
            if (ImGui::Button("Remove", ImVec2(Dpi(120), 0)))
            {
                for (int i = 0; i < GameData::NUM_GLOBAL_MESSAGES; i++)
                    gd->global_messages[i].clear();
                editor_.GetLogPanel().AddLog("[Info] Removed %d Global Messages.", count);
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(Dpi(100), 0)))
                ImGui::CloseCurrentPopup();
        }
        else
        {
            ImGui::Text("No project loaded.");
            if (ImGui::Button("OK", ImVec2(Dpi(120), 0)))
                ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void MenuBar::DrawFileMenu()
{
    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("New Game...", "Ctrl+N"))
        {
            auto& app = editor_.GetApp();
            const auto* templates = &app.GetTemplateManager().GetTemplates();
            NewGameDialog::Open([&app, this](const NewGameSettings& settings) {
                std::string project_dir = settings.project_dir;
                std::string project_path = project_dir + "/Game.agf";

                // If a real template is selected, extract it first
                if (settings.selected_template && settings.selected_template->valid)
                {
                    if (!app.GetTemplateManager().ExtractTemplate(
                            *settings.selected_template, project_dir))
                    {
                        editor_.GetLogPanel().AddLog("[Error] Failed to extract template: %s",
                            settings.selected_template->friendly_name.c_str());
                        return;
                    }
                    editor_.GetLogPanel().AddLog("[Info] Extracted template: %s",
                        settings.selected_template->friendly_name.c_str());

                    // Open the extracted project
                    if (app.GetProject()->OpenProject(project_path))
                    {
                        // Override game title from dialog settings
                        app.GetProject()->SetGameTitle(settings.game_title);
                        app.LoadGameFonts();
                        editor_.GetLogPanel().AddLog("[Info] Created new game from template: %s (%s)",
                            settings.game_title, settings.selected_template->friendly_name.c_str());
                        editor_.NotifyProjectLoaded();
                        editor_.OpenWelcomePane();
                    }
                    else
                    {
                        editor_.GetLogPanel().AddLog("[Error] Failed to open extracted template project.");
                    }
                }
                else
                {
                    // No template: create a blank project
                    if (app.GetProject()->NewProject(
                            project_path,
                            settings.game_title,
                            settings.GetWidth(),
                            settings.GetHeight(),
                            settings.GetColorDepthBits()))
                    {
                        editor_.GetLogPanel().AddLog("[Info] Created new game: %s (%dx%d, %d-bit)",
                            settings.game_title, settings.GetWidth(), settings.GetHeight(),
                            settings.GetColorDepthBits());
                        editor_.NotifyProjectLoaded();
                        editor_.OpenWelcomePane();
                    }
                    else
                    {
                        editor_.GetLogPanel().AddLog("[Error] Failed to create new game project.");
                    }
                }
            }, templates);
        }
        if (ImGui::MenuItem("Open Game...", "Ctrl+O"))
        {
            auto& app = editor_.GetApp();
            FileDialog::Open(FileDialogType::OpenFile, "Open AGS Game",
                ".agf,.ags,.dta{AGS Game Files}",
                ".",
                [&app, this](const std::string& path) {
                    if (app.GetProject()->OpenProject(path))
                    {
                        app.LoadGameFonts();
                        editor_.GetLogPanel().AddLog("[Info] Opened game: %s",
                            app.GetProject()->GetGameTitle().c_str());
                        editor_.NotifyProjectLoaded();
                        editor_.OpenWelcomePane();
                    }
                    else
                    {
                        editor_.GetLogPanel().AddLog("[Error] Failed to open game: %s", path.c_str());
                    }
                });
        }
        if (ImGui::MenuItem("Import Old Game..."))
        {
            auto& app = editor_.GetApp();
            ImportGameDialog::Open([&app, this](const ImportOptions& options) {
                if (app.GetProject()->ImportOldGame(
                        options.game_file_path, options.create_backup,
                        options.backup_dir, options.import_editor_dat))
                {
                    app.LoadGameFonts();
                    editor_.GetLogPanel().AddLog("[Info] Imported old game: %s",
                        app.GetProject()->GetGameTitle().c_str());
                    editor_.NotifyProjectLoaded();
                    editor_.OpenWelcomePane();

                    // Show results
                    ImportResult res;
                    res.success = true;
                    auto* gd = app.GetProject()->GetGameData();
                    if (gd) {
                        res.characters_imported = static_cast<int>(gd->characters.size());
                        res.views_imported = static_cast<int>(gd->views.size());
                        res.dialogs_imported = static_cast<int>(gd->dialogs.size());
                        res.guis_imported = static_cast<int>(gd->guis.size());
                        res.rooms_imported = static_cast<int>(gd->rooms.size());
                        res.sprites_imported = static_cast<int>(gd->sprites.size());
                        res.script_modules_imported = static_cast<int>(gd->script_modules.size());
                    }
                    ImportGameDialog::ShowResults(res);
                }
                else
                {
                    ImportResult res;
                    res.error_message = "Failed to import game.";
                    ImportGameDialog::ShowResults(res);
                    editor_.GetLogPanel().AddLog("[Error] Failed to import old game.");
                }
            });
        }

        ImGui::Separator();

        auto* project = editor_.GetApp().GetProject();
        bool has_project = project && project->IsLoaded();

        if (ImGui::MenuItem("Save", "Ctrl+S", false, has_project))
        {
            if (project->SaveProject())
                editor_.GetLogPanel().AddLog("[Info] Project saved.");
            else
                editor_.GetLogPanel().AddLog("[Error] Failed to save project.");
        }
        if (ImGui::MenuItem("Save As...", nullptr, false, has_project))
        {
            FileDialog::Open(FileDialogType::SaveFile, "Save AGS Game As",
                ".agf{AGS Game File}",
                has_project ? project->GetProjectDir() : ".",
                [project, this](const std::string& path) {
                    if (project->SaveProjectAs(path))
                        editor_.GetLogPanel().AddLog("[Info] Project saved as: %s", path.c_str());
                    else
                        editor_.GetLogPanel().AddLog("[Error] Failed to save project.");
                });
        }

        if (has_project && ImGui::MenuItem("Make Template..."))
        {
            FileDialog::Open(FileDialogType::SaveFile, "Save Game Template",
                ".agt{AGS Template File}",
                project->GetProjectDir(),
                [project, this](const std::string& path) {
                    // Save the project first
                    project->SaveProject();
                    auto& tmpl_mgr = editor_.GetApp().GetTemplateManager();
                    if (tmpl_mgr.MakeTemplate(project->GetProjectDir(), path))
                        editor_.GetLogPanel().AddLog("[Info] Template created: %s", path.c_str());
                    else
                        editor_.GetLogPanel().AddLog("[Error] Failed to create template.");
                });
        }

        // Global Messages (legacy AGS 2.x) submenu
        if (has_project)
        {
            auto* gd = project->GetGameData();
            bool has_global_messages = false;
            if (gd)
            {
                for (int i = 0; i < GameData::NUM_GLOBAL_MESSAGES; i++)
                {
                    if (!gd->global_messages[i].empty())
                    {
                        has_global_messages = true;
                        break;
                    }
                }
            }

            if (ImGui::BeginMenu("Global Messages", has_global_messages))
            {
                if (ImGui::MenuItem("Export Global Messages to Script..."))
                {
                    show_export_global_messages_ = true;
                }
                if (ImGui::MenuItem("Remove Global Messages"))
                {
                    show_remove_global_messages_ = true;
                }
                ImGui::EndMenu();
            }
        }

        ImGui::Separator();

        // Recent files submenu
        if (project && ImGui::BeginMenu("Recent Games"))
        {
            auto& recent = project->GetRecentFiles().GetFiles();
            if (recent.empty())
            {
                ImGui::MenuItem("(No recent files)", nullptr, false, false);
            }
            else
            {
                for (const auto& path : recent)
                {
                    // Show just the filename for display
                    std::string display = path;
                    size_t sep = path.find_last_of("/\\");
                    if (sep != std::string::npos)
                        display = path.substr(sep + 1) + " - " + path;

                    if (ImGui::MenuItem(display.c_str()))
                    {
                        if (project->OpenProject(path))
                        {
                            editor_.GetApp().LoadGameFonts();
                            editor_.GetLogPanel().AddLog("[Info] Opened recent game: %s",
                                project->GetGameTitle().c_str());
                            editor_.NotifyProjectLoaded();
                            editor_.OpenWelcomePane();
                        }
                    }
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Clear Recent Files"))
                {
                    project->GetRecentFiles().Clear();
                }
            }
            ImGui::EndMenu();
        }

        ImGui::Separator();

        if (has_project && ImGui::MenuItem("Game Statistics", "Ctrl+F2"))
        {
            show_game_stats_ = true;
        }

        if (has_project && ImGui::MenuItem("Open Project in File Explorer"))
        {
            std::string dir = project->GetProjectDir();
#if defined(__linux__)
            std::string cmd = "xdg-open \"" + dir + "\" &";
            system(cmd.c_str());
#elif defined(__APPLE__)
            std::string cmd = "open \"" + dir + "\"";
            system(cmd.c_str());
#elif defined(_WIN32)
            std::string cmd = "explorer \"" + dir + "\"";
            system(cmd.c_str());
#endif
            editor_.GetLogPanel().AddLog("[Info] Opened project folder: %s", dir.c_str());
        }

        ImGui::Separator();

        if (has_project && ImGui::MenuItem("Close Project"))
        {
            project->CloseProject();
            editor_.NotifyProjectClosed();
            editor_.GetLogPanel().AddLog("[Info] Project closed.");
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Exit", "Alt+F4"))
        {
            SDL_Event quit_event;
            quit_event.type = SDL_QUIT;
            SDL_PushEvent(&quit_event);
        }

        ImGui::EndMenu();
    }
}

void MenuBar::DrawEditMenu()
{
    if (ImGui::BeginMenu("Edit"))
    {
        auto& undo = editor_.GetApp().GetUndoManager();
        auto& clipboard = editor_.GetApp().GetClipboard();

        // Undo with description
        std::string undo_label = "Undo";
        if (undo.CanUndo())
            undo_label += " " + undo.GetUndoDescription();
        if (ImGui::MenuItem(undo_label.c_str(), "Ctrl+Z", false, undo.CanUndo()))
            undo.Undo();

        std::string redo_label = "Redo";
        if (undo.CanRedo())
            redo_label += " " + undo.GetRedoDescription();
        if (ImGui::MenuItem(redo_label.c_str(), "Ctrl+Y", false, undo.CanRedo()))
            undo.Redo();

        ImGui::Separator();

        if (ImGui::MenuItem("Cut", "Ctrl+X"))
        {
            // ImGui handles Ctrl+X for active InputText/TextEditor widgets
            // Push the shortcut event so the focused widget picks it up
            ImGuiIO& io = ImGui::GetIO();
            io.AddKeyEvent(ImGuiKey_X, true);
            io.AddKeyEvent(ImGuiMod_Ctrl, true);
        }
        if (ImGui::MenuItem("Copy", "Ctrl+C"))
        {
            ImGuiIO& io = ImGui::GetIO();
            io.AddKeyEvent(ImGuiKey_C, true);
            io.AddKeyEvent(ImGuiMod_Ctrl, true);
        }
        if (ImGui::MenuItem("Paste", "Ctrl+V", false, clipboard.HasText() || clipboard.HasData()))
        {
            ImGuiIO& io = ImGui::GetIO();
            io.AddKeyEvent(ImGuiKey_V, true);
            io.AddKeyEvent(ImGuiMod_Ctrl, true);
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Find...", "Ctrl+F"))
        {
            // Focus / open a script editor with search active
            auto* se = editor_.FindPane<ScriptEditor>();
            if (se)
                editor_.GetLogPanel().AddLog("[Info] Find: use Ctrl+F in the Script Editor.");
            else
                editor_.OpenScriptFile("GlobalScript.asc");
        }
        if (ImGui::MenuItem("Replace...", "Ctrl+H"))
        {
            auto* se = editor_.FindPane<ScriptEditor>();
            if (se)
                editor_.GetLogPanel().AddLog("[Info] Replace: use Ctrl+H in the Script Editor.");
            else
                editor_.OpenScriptFile("GlobalScript.asc");
        }

        ImGui::Separator();

        bool has_proj = editor_.GetApp().GetProject() && editor_.GetApp().GetProject()->IsLoaded();
        if (ImGui::MenuItem("Go to Room...", "Ctrl+G", false, has_proj))
        {
            show_go_to_room_ = true;
            go_to_room_number_ = 0;
        }
        if (ImGui::MenuItem("Go to Events", "F4", false, has_proj))
        {
            int idx = editor_.GetActivePaneIndex();
            if (idx >= 0)
            {
                auto& panes = editor_.GetPanes();
                if (idx < (int)panes.size())
                    panes[idx]->FocusEvents();
            }
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Preferences..."))
        {
            editor_.OpenOrFocusPane<PreferencesPane>(editor_);
        }

        ImGui::EndMenu();
    }
}

void MenuBar::DrawBuildMenu()
{
    if (ImGui::BeginMenu("Build"))
    {
        auto* project = editor_.GetApp().GetProject();
        bool has_project = project && project->IsLoaded();

        if (ImGui::MenuItem("Build Game", "F7", false, has_project))
        {
            auto* bp = editor_.OpenOrFocusPane<BuildPane>(editor_);
            bp->StartBuild();
        }
        if (ImGui::MenuItem("Run Game", "F5", false, has_project))
        {
            auto* bp = editor_.OpenOrFocusPane<BuildPane>(editor_);
            bp->StartRun(false);
        }
        if (ImGui::MenuItem("Debug Game", "Ctrl+F5", false, has_project))
        {
            auto* bp = editor_.OpenOrFocusPane<BuildPane>(editor_);
            bp->StartRun(true);
        }
        if (ImGui::MenuItem("Stop Game", "Shift+F5", false, has_project))
        {
            // Find existing build pane to stop — or just log
            editor_.GetLogPanel().AddLog("[Info] Stop requested.");
        }
        ImGui::Separator();
        if (has_project && ImGui::MenuItem("Run game setup..."))
        {
            auto* bp = editor_.OpenOrFocusPane<BuildPane>(editor_);
            bp->GetBuildSystem().RunSetup(*project, bp->GetBuildSystem().GetConfig());
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Build Settings..."))
        {
            editor_.OpenOrFocusPane<BuildPane>(editor_);
        }

        if (has_project && ImGui::MenuItem("Open Build Folder in File Explorer"))
        {
            std::string dir = project->GetProjectDir() + "/Compiled";
#if defined(__linux__)
            std::string cmd = "xdg-open \"" + dir + "\" &";
            system(cmd.c_str());
#elif defined(__APPLE__)
            std::string cmd = "open \"" + dir + "\"";
            system(cmd.c_str());
#elif defined(_WIN32)
            std::string cmd = "explorer \"" + dir + "\"";
            system(cmd.c_str());
#endif
            editor_.GetLogPanel().AddLog("[Info] Opened build folder: %s", dir.c_str());
        }

        ImGui::Separator();

        if (has_project && ImGui::MenuItem("Auto-Number Speech Lines..."))
        {
            auto* gd = project->GetGameData();
            if (gd)
            {
                auto res = SpeechTools::AutoNumberSpeechLines(
                    project->GetProjectDir(), *gd, false, false, false);
                if (res.success)
                {
                    speech_result_message_ =
                        "Speech lines numbered: " + std::to_string(res.lines_numbered) +
                        "\nScripts modified: " + std::to_string(res.scripts_modified) +
                        "\nReference file: speechref.txt";
                    show_auto_number_result_ = true;
                    editor_.GetLogPanel().AddLog("[Info] Auto-numbered %d speech lines in %d scripts.",
                        res.lines_numbered, res.scripts_modified);
                }
                else
                {
                    speech_result_message_ = "Error: " + res.error_message;
                    show_auto_number_result_ = true;
                    editor_.GetLogPanel().AddLog("[Error] %s", res.error_message.c_str());
                }
            }
        }

        if (has_project && ImGui::MenuItem("Create Voice Acting Script..."))
        {
            auto* gd = project->GetGameData();
            if (gd)
            {
                std::string output = project->GetProjectDir() + "/VoiceActingScript.txt";
                auto res = SpeechTools::CreateVoiceActingScript(
                    project->GetProjectDir(), *gd, output);
                if (res.success)
                {
                    speech_result_message_ =
                        "Voice acting script created!\n"
                        "Total lines: " + std::to_string(res.total_lines) +
                        "\nCharacters: " + std::to_string(res.characters_found) +
                        "\nOutput: VoiceActingScript.txt";
                    show_voice_script_result_ = true;
                    editor_.GetLogPanel().AddLog("[Info] Voice acting script: %d lines, %d characters.",
                        res.total_lines, res.characters_found);
                }
                else
                {
                    speech_result_message_ = "Error: " + res.error_message;
                    show_voice_script_result_ = true;
                    editor_.GetLogPanel().AddLog("[Error] %s", res.error_message.c_str());
                }
            }
        }

        ImGui::EndMenu();
    }
}

void MenuBar::DrawToolsMenu()
{
    if (ImGui::BeginMenu("Tools"))
    {
        auto* project = editor_.GetApp().GetProject();
        bool has_project = project && project->IsLoaded();

        if (has_project && ImGui::MenuItem("Restore All Sprites from Sources..."))
        {
            RestoreSpritesFromSources();
        }

        ImGui::EndMenu();
    }

    // Restore sprites result dialog
    if (show_restore_sprites_result_)
    {
        ImGui::OpenPopup("Restore Sprites from Sources");
        show_restore_sprites_result_ = false;
    }
    if (ImGui::BeginPopupModal("Restore Sprites from Sources", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextWrapped("%s", restore_sprites_message_.c_str());
        ImGui::Separator();
        if (ImGui::Button("OK", ImVec2(Dpi(120), 0)))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

void MenuBar::RestoreSpritesFromSources()
{
    auto* project = editor_.GetApp().GetProject();
    if (!project || !project->IsLoaded()) return;

    auto* gd = project->GetGameData();
    auto* loader = project->GetSpriteLoader();
    if (!gd || !loader || !loader->IsOpen() || !loader->GetSpriteFile()) return;

    std::string project_dir = project->GetProjectDir();
    int restored = 0;
    int failed = 0;
    int skipped = 0;
    int total = (int)gd->sprites.size();
    std::string errors;

    // Build the sparse replacement vector
    int max_slot = loader->GetTopmostSprite();
    std::vector<std::pair<bool, AGS::Common::BitmapData>> sprite_data(max_slot + 1);
    // Keep bitmaps alive until we save the sprite file
    std::vector<AGS::Common::Bitmap*> loaded_bitmaps;

    for (auto& spr : gd->sprites)
    {
        if (spr.id < 0 || spr.id > max_slot) continue;

        if (!spr.source.HasSource())
        {
            skipped++;
            continue;
        }

        // Resolve source file path relative to project directory
        std::string src_path = spr.source.source_file;
        if (!std::filesystem::path(src_path).is_absolute())
            src_path = project_dir + "/" + src_path;

        if (!std::filesystem::exists(src_path))
        {
            // Source file not found — skip, keep existing sprite
            errors += "Sprite " + std::to_string(spr.id) +
                      ": source not found: " + spr.source.source_file + "\n";
            failed++;
            continue;
        }

        auto* bmp = AGS::Common::BitmapHelper::LoadFromFile(src_path.c_str());
        if (!bmp)
        {
            errors += "Sprite " + std::to_string(spr.id) +
                      ": failed to load: " + spr.source.source_file + "\n";
            failed++;
            continue;
        }

        // Apply import cropping if specified
        if (spr.source.import_width > 0 && spr.source.import_height > 0 &&
            (spr.source.offset_x > 0 || spr.source.offset_y > 0 ||
             spr.source.import_width < bmp->GetWidth() ||
             spr.source.import_height < bmp->GetHeight()))
        {
            int crop_w = std::min(spr.source.import_width, bmp->GetWidth() - spr.source.offset_x);
            int crop_h = std::min(spr.source.import_height, bmp->GetHeight() - spr.source.offset_y);
            if (crop_w > 0 && crop_h > 0)
            {
                auto* cropped = AGS::Common::BitmapHelper::CreateBitmap(
                    crop_w, crop_h, bmp->GetColorDepth());
                cropped->Blit(bmp, spr.source.offset_x, spr.source.offset_y,
                             0, 0, crop_w, crop_h);
                delete bmp;
                bmp = cropped;
            }
        }

        sprite_data[spr.id] = { true, bmp->GetBitmapData() };
        loaded_bitmaps.push_back(bmp);

        // Update metrics
        spr.width = bmp->GetWidth();
        spr.height = bmp->GetHeight();
        spr.color_depth = bmp->GetColorDepth();
        restored++;
    }

    if (restored == 0)
    {
        for (auto* b : loaded_bitmaps) delete b;

        if (skipped == total)
            restore_sprites_message_ = "No sprites have source file information.\n"
                "Source metadata is stored in the AGF project file's <Sprites> section.";
        else
            restore_sprites_message_ = "No sprites could be restored.\n" +
                std::to_string(failed) + " source file(s) not found.\n\n" + errors;
        show_restore_sprites_result_ = true;
        return;
    }

    // Save the sprite file with all replacements
    std::string spr_path = project_dir + "/acsprset.spr";
    AGS::Common::SpriteFileIndex index;
    auto err = AGS::Common::SaveSpriteFile(
        AGS::Common::String(spr_path.c_str()),
        sprite_data, loader->GetSpriteFile(),
        loader->GetSpriteFile()->GetStoreFlags(),
        loader->GetSpriteFile()->GetSpriteCompression(),
        index);

    if (err)
    {
        // Save the updated index file
        std::string idx_path = project_dir + "/sprindex.dat";
        AGS::Common::SaveSpriteIndex(
            AGS::Common::String(idx_path.c_str()), index);

        // Evict all restored sprites from texture cache
        auto& tex_cache = editor_.GetApp().GetTextureCache();
        for (const auto& spr : gd->sprites)
        {
            if (spr.source.HasSource())
                tex_cache.EvictSprite(spr.id);
        }

        // Reload the sprite file
        loader->Close();
        loader->Open(project_dir);

        restore_sprites_message_ =
            "Sprites restored from sources!\n\n"
            "Restored: " + std::to_string(restored) + "\n"
            "Skipped (no source): " + std::to_string(skipped) + "\n"
            "Failed: " + std::to_string(failed);
        if (!errors.empty())
            restore_sprites_message_ += "\n\n" + errors;

        editor_.GetLogPanel().AddLog("[Sprite] Restored %d sprites from sources (%d skipped, %d failed).",
            restored, skipped, failed);
    }
    else
    {
        restore_sprites_message_ = "Failed to save sprite file after restore operation.";
        editor_.GetLogPanel().AddLog("[Error] Failed to save sprite file during restore.");
    }

    show_restore_sprites_result_ = true;

    // Clean up loaded bitmaps
    for (auto* b : loaded_bitmaps)
        delete b;
}

void MenuBar::DrawWindowMenu()
{
    if (ImGui::BeginMenu("Window"))
    {
        if (ImGui::MenuItem("Script Editor"))
        {
            editor_.OpenScriptFile("GlobalScript.asc");
        }
        if (ImGui::MenuItem("Room Editor"))
        {
            editor_.OpenOrFocusPane<RoomEditor>(editor_);
        }
        if (ImGui::MenuItem("Sprite Manager"))
        {
            editor_.OpenOrFocusPane<SpriteManager>(editor_);
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Character Editor"))
        {
            editor_.OpenOrFocusPane<CharacterEditor>(editor_);
        }
        if (ImGui::MenuItem("Dialog Editor"))
        {
            editor_.OpenOrFocusPane<DialogEditor>(editor_);
        }
        if (ImGui::MenuItem("View Editor"))
        {
            editor_.OpenOrFocusPane<ViewEditor>(editor_);
        }
        if (ImGui::MenuItem("GUI Editor"))
        {
            editor_.OpenOrFocusPane<GUIEditor>(editor_);
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Font Editor"))
        {
            editor_.OpenOrFocusPane<FontEditor>(editor_);
        }
        if (ImGui::MenuItem("Audio Manager"))
        {
            editor_.OpenOrFocusPane<AudioManager>(editor_);
        }
        if (ImGui::MenuItem("Inventory Editor"))
        {
            editor_.OpenOrFocusPane<InventoryEditor>(editor_);
        }
        if (ImGui::MenuItem("Cursor Editor"))
        {
            editor_.OpenOrFocusPane<CursorEditor>(editor_);
        }
        if (ImGui::MenuItem("Palette Editor"))
        {
            editor_.OpenOrFocusPane<PaletteEditor>(editor_);
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Translation Editor"))
        {
            editor_.OpenOrFocusPane<TranslationEditor>(editor_);
        }
        if (ImGui::MenuItem("Global Variables"))
        {
            editor_.OpenOrFocusPane<GlobalVariablesEditor>(editor_);
        }
        if (ImGui::MenuItem("Custom Properties"))
        {
            editor_.OpenOrFocusPane<CustomPropertiesEditor>(editor_);
        }
        if (ImGui::MenuItem("Text Parser"))
        {
            editor_.OpenOrFocusPane<TextParserEditor>(editor_);
        }
        if (ImGui::MenuItem("Lip Sync Editor"))
        {
            editor_.OpenOrFocusPane<LipSyncEditor>(editor_);
        }
        if (ImGui::MenuItem("Debug Log"))
        {
            editor_.OpenOrFocusPane<DebugLogPane>(editor_);
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Settings"))
        {
            editor_.OpenOrFocusPane<SettingsPane>(editor_);
        }
        if (ImGui::MenuItem("Preferences"))
        {
            editor_.OpenOrFocusPane<PreferencesPane>(editor_);
        }
        ImGui::EndMenu();
    }
}

void MenuBar::DrawHelpMenu()
{
    if (ImGui::BeginMenu("Help"))
    {
        if (ImGui::MenuItem("AGS Manual"))
        {
#if defined(__linux__)
            system("xdg-open https://adventuregamestudio.github.io/ags-manual/ &");
#elif defined(__APPLE__)
            system("open https://adventuregamestudio.github.io/ags-manual/");
#elif defined(_WIN32)
            system("start https://adventuregamestudio.github.io/ags-manual/");
#endif
            editor_.GetLogPanel().AddLog("[Info] Opening AGS manual in browser.");
        }
        if (ImGui::MenuItem("Scripting Reference"))
        {
#if defined(__linux__)
            system("xdg-open https://adventuregamestudio.github.io/ags-manual/Scripting.html &");
#elif defined(__APPLE__)
            system("open https://adventuregamestudio.github.io/ags-manual/Scripting.html");
#elif defined(_WIN32)
            system("start https://adventuregamestudio.github.io/ags-manual/Scripting.html");
#endif
        }
        if (ImGui::MenuItem("Script Keyword Index"))
        {
#if defined(__linux__)
            system("xdg-open https://adventuregamestudio.github.io/ags-manual/ScriptKeywords.html &");
#elif defined(__APPLE__)
            system("open https://adventuregamestudio.github.io/ags-manual/ScriptKeywords.html");
#elif defined(_WIN32)
            system("start https://adventuregamestudio.github.io/ags-manual/ScriptKeywords.html");
#endif
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Visit AGS Website"))
        {
#if defined(__linux__)
            system("xdg-open https://www.adventuregamestudio.co.uk &");
#elif defined(__APPLE__)
            system("open https://www.adventuregamestudio.co.uk");
#elif defined(_WIN32)
            system("start https://www.adventuregamestudio.co.uk");
#endif
        }
        if (ImGui::MenuItem("Visit AGS Forums"))
        {
#if defined(__linux__)
            system("xdg-open https://www.adventuregamestudio.co.uk/forums/ &");
#elif defined(__APPLE__)
            system("open https://www.adventuregamestudio.co.uk/forums/");
#elif defined(_WIN32)
            system("start https://www.adventuregamestudio.co.uk/forums/");
#endif
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Check for Updates..."))
        {
#if defined(__linux__)
            system("xdg-open https://github.com/adventuregamestudio/ags/releases &");
#elif defined(__APPLE__)
            system("open https://github.com/adventuregamestudio/ags/releases");
#elif defined(_WIN32)
            system("start https://github.com/adventuregamestudio/ags/releases");
#endif
            editor_.GetLogPanel().AddLog("[Info] Opening AGS releases page to check for updates.");
        }
        ImGui::Separator();
        if (ImGui::MenuItem("About AGS Editor..."))
        {
            show_about_ = true;
        }
        ImGui::EndMenu();
    }
}

} // namespace AGSEditor
