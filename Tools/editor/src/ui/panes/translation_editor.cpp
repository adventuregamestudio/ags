// AGS Editor ImGui - Translation Editor implementation
#include "translation_editor.h"
#include "ui/editor_ui.h"
#include "ui/log_panel.h"
#include "ui/file_dialog.h"
#include "core/dpi_helper.h"
#include "project/project.h"
#include "project/game_data.h"
#include "app.h"

#include "imgui.h"

#include "game/tra_file.h"
#include "util/file.h"
#include "util/string.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <set>

namespace fs = std::filesystem;

namespace AGSEditor
{

// ---------------------------------------------------------------------------
// .trs file I/O — matches C# Translation.LoadData / SaveData
// Format: comment/tag lines start with "//", data lines alternate original/translation
// Special tags: //#NormalFont=, //#SpeechFont=, //#TextDirection=, //#Encoding=
// ---------------------------------------------------------------------------

bool TranslationEditor::LoadTrsFile(const std::string& path, TranslationLanguage& lang)
{
    std::ifstream f(path);
    if (!f.is_open()) return false;

    lang.entries.clear();
    lang.normal_font = -1;
    lang.speech_font = -1;
    lang.right_to_left = -1;
    lang.encoding.clear();

    std::string line;
    bool reading_original = true;
    std::string pending_original;

    while (std::getline(f, line))
    {
        // Remove trailing \r if present (Windows line endings)
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        // Check for comment/tag lines
        if (line.size() >= 2 && line[0] == '/' && line[1] == '/')
        {
            // Parse special tags
            if (line.rfind("//#NormalFont=", 0) == 0)
            {
                std::string val = line.substr(14);
                if (val == "DEFAULT" || val == "default" || val.empty())
                    lang.normal_font = -1;
                else
                    try { lang.normal_font = std::stoi(val); } catch (...) {}
            }
            else if (line.rfind("//#SpeechFont=", 0) == 0)
            {
                std::string val = line.substr(14);
                if (val == "DEFAULT" || val == "default" || val.empty())
                    lang.speech_font = -1;
                else
                    try { lang.speech_font = std::stoi(val); } catch (...) {}
            }
            else if (line.rfind("//#TextDirection=", 0) == 0)
            {
                std::string val = line.substr(17);
                if (val == "LEFT") lang.right_to_left = 0;
                else if (val == "RIGHT") lang.right_to_left = 1;
                else lang.right_to_left = -1; // DEFAULT
            }
            else if (line.rfind("//#Encoding=", 0) == 0)
            {
                lang.encoding = line.substr(12);
            }
            continue; // Skip all comment lines
        }

        // Data line: alternating original / translation
        if (reading_original)
        {
            pending_original = line;
            reading_original = false;
        }
        else
        {
            if (!pending_original.empty())
            {
                TranslationEntry entry;
                entry.original = pending_original;
                entry.translated = line;
                entry.modified = false;
                lang.entries.push_back(std::move(entry));
            }
            reading_original = true;
        }
    }

    lang.modified = false;
    return true;
}

bool TranslationEditor::SaveTrsFile(const std::string& path, const TranslationLanguage& lang)
{
    std::ofstream f(path);
    if (!f.is_open()) return false;

    // Write header comments
    f << "// AGS TRANSLATION SOURCE FILE\n";
    f << "// Format is alternating lines with original game text and replacement\n";
    f << "// Lines starting with // are comments - DO NOT translate them\n";
    f << "// Special tags:\n";

    // Write special tags
    if (lang.normal_font >= 0)
        f << "//#NormalFont=" << lang.normal_font << "\n";
    else
        f << "//#NormalFont=DEFAULT\n";

    if (lang.speech_font >= 0)
        f << "//#SpeechFont=" << lang.speech_font << "\n";
    else
        f << "//#SpeechFont=DEFAULT\n";

    if (lang.right_to_left == 0)
        f << "//#TextDirection=LEFT\n";
    else if (lang.right_to_left == 1)
        f << "//#TextDirection=RIGHT\n";
    else
        f << "//#TextDirection=DEFAULT\n";

    if (!lang.encoding.empty())
        f << "//#Encoding=" << lang.encoding << "\n";
    else
        f << "//#Encoding=UTF-8\n";

    f << "//\n";

    // Write entries as alternating lines
    for (const auto& entry : lang.entries)
    {
        if (entry.original.empty()) continue;
        f << entry.original << "\n";
        f << entry.translated << "\n";
    }

    return true;
}

void TranslationEditor::SaveAllTrsFiles()
{
    auto* project = editor_.GetApp().GetProject();
    if (!project || !project->IsLoaded()) return;
    std::string proj_dir = project->GetProjectDir();

    for (auto& lang : languages_)
    {
        if (!lang.modified) continue;
        std::string path = proj_dir + "/" + lang.filename;
        if (SaveTrsFile(path, lang))
        {
            lang.modified = false;
            editor_.GetLogPanel().AddLog("[Translation] Saved %s", lang.filename.c_str());
        }
        else
        {
            editor_.GetLogPanel().AddLog("[Translation] Failed to save %s", path.c_str());
        }
    }
}

// ---------------------------------------------------------------------------
// "Update from Game" — collect translatable strings and merge into languages
// ---------------------------------------------------------------------------

void TranslationEditor::CollectGameStrings(std::vector<std::string>& out)
{
    auto* project = editor_.GetApp().GetProject();
    auto* gd = project ? project->GetGameData() : nullptr;
    if (!gd) return;

    // Game settings
    if (!gd->game_title.empty()) out.push_back(gd->game_title);

    // Characters — real names
    for (const auto& ch : gd->characters)
    {
        if (!ch.real_name.empty())
            out.push_back(ch.real_name);
    }

    // Inventory items — descriptions
    for (const auto& inv : gd->inventory_items)
    {
        if (!inv.description.empty())
            out.push_back(inv.description);
    }

    // GUI controls — label/button text
    for (const auto& gui : gd->guis)
    {
        for (const auto& ctrl : gui.controls)
        {
            if (!ctrl.text.empty())
                out.push_back(ctrl.text);
        }
    }

    // Dialog options text
    for (const auto& dlg : gd->dialogs)
    {
        for (const auto& opt : dlg.options)
        {
            if (!opt.text.empty())
                out.push_back(opt.text);
        }
    }

    // Translated custom property default values
    for (const auto& schema : gd->custom_property_schemas)
    {
        if (schema.translated && schema.type == 2 && !schema.default_value.empty())
            out.push_back(schema.default_value);
    }

    // Remove duplicates while preserving order
    std::set<std::string> seen;
    std::vector<std::string> unique;
    unique.reserve(out.size());
    for (auto& s : out)
    {
        if (seen.insert(s).second)
            unique.push_back(std::move(s));
    }
    out = std::move(unique);
}

void TranslationEditor::UpdateFromGame()
{
    std::vector<std::string> game_strings;
    CollectGameStrings(game_strings);

    if (game_strings.empty())
    {
        editor_.GetLogPanel().AddLog("[Translation] No translatable strings found in game data.");
        return;
    }

    // For each language, merge in any new strings
    for (auto& lang : languages_)
    {
        // Build set of existing originals
        std::set<std::string> existing;
        for (const auto& e : lang.entries)
            existing.insert(e.original);

        int added = 0;
        for (const auto& s : game_strings)
        {
            if (existing.find(s) == existing.end())
            {
                TranslationEntry entry;
                entry.original = s;
                lang.entries.push_back(std::move(entry));
                existing.insert(s);
                added++;
            }
        }

        if (added > 0)
        {
            lang.modified = true;
            editor_.GetLogPanel().AddLog("[Translation] Added %d new strings to '%s' (total: %d)",
                added, lang.name.c_str(), (int)lang.entries.size());
        }
        else
        {
            editor_.GetLogPanel().AddLog("[Translation] '%s' is up to date (%d strings)",
                lang.name.c_str(), (int)lang.entries.size());
        }
    }
}

// ---------------------------------------------------------------------------
// "Make Default Language" — replace game text with translations from selected
// language, then clear the translations (since they are now the originals)
// ---------------------------------------------------------------------------

void TranslationEditor::MakeDefaultLanguage()
{
    if (selected_language_ < 0 || selected_language_ >= (int)languages_.size())
        return;

    auto* project = editor_.GetApp().GetProject();
    auto* gd = project ? project->GetGameData() : nullptr;
    if (!gd) return;

    const auto& lang = languages_[selected_language_];
    int replaced = 0;

    // Build lookup map: original -> translated
    std::map<std::string, std::string> dict;
    for (const auto& e : lang.entries)
    {
        if (!e.original.empty() && !e.translated.empty())
            dict[e.original] = e.translated;
    }

    // Replace character real names
    for (auto& ch : gd->characters)
    {
        auto it = dict.find(ch.real_name);
        if (it != dict.end())
        {
            ch.real_name = it->second;
            replaced++;
        }
    }

    // Replace inventory descriptions
    for (auto& inv : gd->inventory_items)
    {
        auto it = dict.find(inv.description);
        if (it != dict.end())
        {
            inv.description = it->second;
            replaced++;
        }
    }

    // Replace GUI control text
    for (auto& gui : gd->guis)
    {
        for (auto& ctrl : gui.controls)
        {
            auto it = dict.find(ctrl.text);
            if (it != dict.end())
            {
                ctrl.text = it->second;
                replaced++;
            }
        }
    }

    // Replace dialog option text
    for (auto& dlg : gd->dialogs)
    {
        for (auto& opt : dlg.options)
        {
            auto it = dict.find(opt.text);
            if (it != dict.end())
            {
                opt.text = it->second;
                replaced++;
            }
        }
    }

    // Replace game title
    {
        auto it = dict.find(gd->game_title);
        if (it != dict.end())
        {
            gd->game_title = it->second;
            replaced++;
        }
    }

    editor_.GetLogPanel().AddLog("[Translation] Made '%s' the default language: %d strings replaced in game data.",
        lang.name.c_str(), replaced);
}

// ---------------------------------------------------------------------------
// Compile .trs -> .tra binary
// ---------------------------------------------------------------------------

bool TranslationEditor::CompileTra(const TranslationLanguage& lang, const std::string& output_path)
{
    AGS::Common::Translation tra;
    tra.NormalFont = lang.normal_font;
    tra.SpeechFont = lang.speech_font;
    tra.RightToLeft = lang.right_to_left;

    auto* project = editor_.GetApp().GetProject();
    auto* gd = project ? project->GetGameData() : nullptr;
    if (gd)
    {
        tra.GameUid = 0;
        tra.GameName = AGS::Common::String(gd->game_title.c_str());
    }

    for (const auto& entry : lang.entries)
    {
        if (!entry.original.empty() && !entry.translated.empty())
        {
            tra.Dict[AGS::Common::String(entry.original.c_str())] =
                AGS::Common::String(entry.translated.c_str());
        }
    }

    auto out = AGS::Common::File::CreateFile(AGS::Common::String(output_path.c_str()));
    if (!out) return false;

    AGS::Common::WriteTraData(tra, std::move(out));
    return true;
}

// ---------------------------------------------------------------------------
// Constructor — load existing .trs files from project
// ---------------------------------------------------------------------------

TranslationEditor::TranslationEditor(EditorUI& editor)
    : editor_(editor)
{
    auto* project = editor_.GetApp().GetProject();
    if (project && project->IsLoaded())
    {
        // Scan project directory for .trs files and parse them
        std::string project_dir = project->GetProjectDir();
        if (!project_dir.empty())
        {
            try {
                for (const auto& entry : fs::directory_iterator(project_dir))
                {
                    if (entry.is_regular_file())
                    {
                        std::string ext = entry.path().extension().string();
                        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                        if (ext == ".trs")
                        {
                            TranslationLanguage lang;
                            lang.name = entry.path().stem().string();
                            lang.filename = entry.path().filename().string();
                            LoadTrsFile(entry.path().string(), lang);
                            languages_.push_back(std::move(lang));
                        }
                    }
                }
            } catch (...) {}
        }
    }
}

void TranslationEditor::Draw()
{
    DrawToolbar();
    ImGui::Separator();

    ImVec2 avail = ImGui::GetContentRegionAvail();
    float list_width = Dpi(200);

    // Left: language list
    ImGui::BeginChild("LangList", ImVec2(list_width, avail.y), ImGuiChildFlags_Borders);
    DrawLanguageList();
    ImGui::EndChild();

    ImGui::SameLine();

    // Right: translation table
    ImGui::BeginChild("TransTable", ImVec2(0, avail.y), ImGuiChildFlags_Borders);
    DrawTranslationTable();
    ImGui::EndChild();
}

void TranslationEditor::DrawToolbar()
{
    // Save buttons
    if (ImGui::Button("Save"))
    {
        if (selected_language_ >= 0 && selected_language_ < (int)languages_.size())
        {
            auto& lang = languages_[selected_language_];
            auto* project = editor_.GetApp().GetProject();
            if (project && project->IsLoaded())
            {
                std::string path = project->GetProjectDir() + "/" + lang.filename;
                if (SaveTrsFile(path, lang))
                {
                    lang.modified = false;
                    editor_.GetLogPanel().AddLog("[Translation] Saved %s", lang.filename.c_str());
                }
                else
                {
                    editor_.GetLogPanel().AddLog("[Translation] Failed to save %s", path.c_str());
                }
            }
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Save All"))
        SaveAllTrsFiles();

    ImGui::SameLine();
    ImGui::Text("|");
    ImGui::SameLine();

    // Add language
    if (ImGui::Button("Add Language"))
        ImGui::OpenPopup("AddLanguage");

    if (ImGui::BeginPopup("AddLanguage"))
    {
        ImGui::InputText("Name", new_language_name_, sizeof(new_language_name_));
        if (ImGui::Button("Create") && new_language_name_[0] != '\0')
        {
            TranslationLanguage lang;
            lang.name = new_language_name_;
            lang.filename = std::string(new_language_name_) + ".trs";
            lang.modified = true;

            // Populate with game strings so there's something to translate
            {
                std::vector<std::string> game_strings;
                CollectGameStrings(game_strings);
                for (auto& s : game_strings)
                {
                    TranslationEntry entry;
                    entry.original = std::move(s);
                    lang.entries.push_back(std::move(entry));
                }
            }

            languages_.push_back(std::move(lang));
            selected_language_ = (int)languages_.size() - 1;
            new_language_name_[0] = '\0';
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    ImGui::SameLine();

    if (ImGui::Button("Remove Language") && selected_language_ >= 0 &&
        selected_language_ < (int)languages_.size())
    {
        languages_.erase(languages_.begin() + selected_language_);
        if (selected_language_ >= (int)languages_.size())
            selected_language_ = (int)languages_.size() - 1;
    }

    ImGui::SameLine();
    ImGui::Text("|");
    ImGui::SameLine();

    // Update from Game — rescan all translatable text
    if (ImGui::Button("Update from Game"))
        UpdateFromGame();

    ImGui::SameLine();

    // Make Default Language
    if (ImGui::Button("Make Default"))
    {
        if (selected_language_ >= 0 && selected_language_ < (int)languages_.size())
            ImGui::OpenPopup("ConfirmMakeDefault");
        else
            editor_.GetLogPanel().AddLog("[Translation] No language selected.");
    }
    if (ImGui::BeginPopupModal("ConfirmMakeDefault", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Replace game text with translations from '%s'?",
            (selected_language_ >= 0 && selected_language_ < (int)languages_.size())
                ? languages_[selected_language_].name.c_str() : "?");
        ImGui::Text("This replaces original game text and cannot be undone!");
        ImGui::Separator();
        if (ImGui::Button("OK", ImVec2(Dpi(120), 0)))
        {
            MakeDefaultLanguage();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(Dpi(120), 0)))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    ImGui::SameLine();
    ImGui::Text("|");
    ImGui::SameLine();

    // Compile .tra (binary) export
    if (ImGui::Button("Compile .tra"))
    {
        if (selected_language_ < 0 || selected_language_ >= (int)languages_.size())
        {
            editor_.GetLogPanel().AddLog("[Translation] No language selected for compile.");
        }
        else
        {
            auto* project = editor_.GetApp().GetProject();
            std::string default_dir = (project && project->IsLoaded()) ? project->GetProjectDir() : ".";

            FileDialog::Open(FileDialogType::SaveFile, "Compile Translation (.tra)",
                ".tra{Compiled Translation}",
                default_dir,
                [this](const std::string& path) {
                    auto& lang = languages_[selected_language_];

                    AGS::Common::Translation tra;
                    tra.NormalFont = lang.normal_font;
                    tra.SpeechFont = lang.speech_font;
                    tra.RightToLeft = lang.right_to_left;

                    auto* project2 = editor_.GetApp().GetProject();
                    auto* gd = project2 ? project2->GetGameData() : nullptr;
                    if (gd)
                    {
                        tra.GameUid = 0;
                        tra.GameName = AGS::Common::String(gd->game_title.c_str());
                    }

                    for (const auto& entry : lang.entries)
                    {
                        if (!entry.original.empty() && !entry.translated.empty())
                        {
                            tra.Dict[AGS::Common::String(entry.original.c_str())] =
                                AGS::Common::String(entry.translated.c_str());
                        }
                    }

                    auto out = AGS::Common::File::CreateFile(AGS::Common::String(path.c_str()));
                    if (out)
                    {
                        AGS::Common::WriteTraData(tra, std::move(out));
                        editor_.GetLogPanel().AddLog("[Translation] Compiled '%s' -> %s (%d entries)",
                            lang.name.c_str(), path.c_str(), (int)tra.Dict.size());
                    }
                    else
                    {
                        editor_.GetLogPanel().AddLog("[Translation] Failed to create file: %s", path.c_str());
                    }
                });
        }
    }

    ImGui::SameLine();

    // Import .tra (binary) into a new language
    if (ImGui::Button("Import .tra"))
    {
        auto* project = editor_.GetApp().GetProject();
        std::string default_dir = (project && project->IsLoaded()) ? project->GetProjectDir() : ".";

        FileDialog::Open(FileDialogType::OpenFile, "Import Compiled Translation",
            ".tra{Compiled Translation}",
            default_dir,
            [this](const std::string& path) {
                auto in = AGS::Common::File::OpenFileRead(AGS::Common::String(path.c_str()));
                if (!in)
                {
                    editor_.GetLogPanel().AddLog("[Translation] Failed to open file: %s", path.c_str());
                    return;
                }

                AGS::Common::Translation tra;
                auto err = AGS::Common::ReadTraData(tra, std::move(in));
                if (!err)
                {
                    editor_.GetLogPanel().AddLog("[Translation] Failed to read compiled translation: %s",
                        err->FullMessage().GetCStr());
                    return;
                }

                TranslationLanguage lang;
                fs::path fpath(path);
                lang.name = fpath.stem().string();
                lang.filename = lang.name + ".trs";

                lang.normal_font = tra.NormalFont;
                lang.speech_font = tra.SpeechFont;
                lang.right_to_left = tra.RightToLeft;
                lang.modified = true;

                for (const auto& kv : tra.Dict)
                {
                    TranslationEntry entry;
                    entry.original = kv.first.GetCStr();
                    entry.translated = kv.second.GetCStr();
                    entry.modified = false;
                    lang.entries.push_back(std::move(entry));
                }

                languages_.push_back(std::move(lang));
                selected_language_ = (int)languages_.size() - 1;
                editor_.GetLogPanel().AddLog("[Translation] Imported '%s' with %d entries from compiled .tra",
                    languages_.back().name.c_str(), (int)languages_.back().entries.size());
            });
    }

    // Second toolbar row
    ImGui::SetNextItemWidth(Dpi(200));
    ImGui::InputTextWithHint("##filter", "Filter text...", filter_text_, sizeof(filter_text_));
    ImGui::SameLine();
    ImGui::Checkbox("Untranslated only", &show_untranslated_only_);

    // Stats
    if (selected_language_ >= 0 && selected_language_ < (int)languages_.size())
    {
        const auto& lang = languages_[selected_language_];
        int total = (int)lang.entries.size();
        int translated = 0;
        for (const auto& e : lang.entries)
            if (!e.translated.empty()) translated++;

        ImGui::SameLine();
        ImGui::Text("| %d/%d translated (%.0f%%)",
            translated, total, total > 0 ? (100.0f * translated / total) : 0.0f);
    }
}

void TranslationEditor::DrawLanguageList()
{
    ImGui::Text("Languages");
    ImGui::Separator();

    for (int i = 0; i < (int)languages_.size(); i++)
    {
        bool selected = (i == selected_language_);
        std::string label = languages_[i].name;
        if (languages_[i].modified)
            label += " *";

        if (ImGui::Selectable(label.c_str(), selected))
            selected_language_ = i;

        // Right-click context menu for rename/delete
        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Rename"))
            {
                selected_language_ = i;
                std::strncpy(rename_buf_, languages_[i].name.c_str(), sizeof(rename_buf_) - 1);
                rename_buf_[sizeof(rename_buf_) - 1] = '\0';
                show_rename_popup_ = true;
            }
            ImGui::EndPopup();
        }
    }

    // Rename popup
    if (show_rename_popup_)
    {
        ImGui::OpenPopup("Rename Translation");
        show_rename_popup_ = false;
    }
    if (ImGui::BeginPopupModal("Rename Translation", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Enter new name for translation:");
        ImGui::InputText("##RenameInput", rename_buf_, sizeof(rename_buf_));

        std::string new_name = rename_buf_;
        std::string error_msg;

        if (new_name.empty())
            error_msg = "Name cannot be empty.";
        else if (selected_language_ >= 0 && new_name == languages_[selected_language_].name)
            error_msg = "Same as current name.";
        else
        {
            // Check for duplicate names
            for (int i = 0; i < (int)languages_.size(); i++)
            {
                if (i != selected_language_ && languages_[i].name == new_name)
                {
                    error_msg = "A translation with this name already exists.";
                    break;
                }
            }
            // Check if target .trs file already exists
            if (error_msg.empty())
            {
                auto* project = editor_.GetApp().GetProject();
                if (project && project->IsLoaded())
                {
                    std::string proj_dir = project->GetProjectDir();
                    std::string new_trs = proj_dir + "/" + new_name + ".trs";
                    if (fs::exists(new_trs))
                        error_msg = "File " + new_name + ".trs already exists on disk.";
                }
            }
        }

        if (!error_msg.empty())
            ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "%s", error_msg.c_str());

        bool can_apply = error_msg.empty();
        if (!can_apply) ImGui::BeginDisabled();
        if (ImGui::Button("OK", ImVec2(Dpi(120), 0)))
        {
            if (selected_language_ >= 0 && selected_language_ < (int)languages_.size())
            {
                auto& lang = languages_[selected_language_];
                auto* project = editor_.GetApp().GetProject();
                if (project && project->IsLoaded())
                {
                    std::string proj_dir = project->GetProjectDir();
                    std::string old_path = proj_dir + "/" + lang.filename;
                    std::string new_filename = new_name + ".trs";
                    std::string new_path = proj_dir + "/" + new_filename;

                    std::error_code ec;
                    if (fs::exists(old_path))
                    {
                        fs::rename(old_path, new_path, ec);
                        if (ec)
                        {
                            editor_.GetLogPanel().AddLog("[Translation] Failed to rename %s: %s",
                                old_path.c_str(), ec.message().c_str());
                        }
                        else
                        {
                            editor_.GetLogPanel().AddLog("[Translation] Renamed %s -> %s",
                                lang.filename.c_str(), new_filename.c_str());
                        }
                    }

                    if (!ec)
                    {
                        lang.name = new_name;
                        lang.filename = new_filename;
                    }
                }
            }
            ImGui::CloseCurrentPopup();
        }
        if (!can_apply) ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(Dpi(120), 0)))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

void TranslationEditor::DrawTranslationTable()
{
    if (selected_language_ < 0 || selected_language_ >= (int)languages_.size())
    {
        ImGui::TextDisabled("Select a language to edit translations.");
        return;
    }

    auto& lang = languages_[selected_language_];
    ImGui::Text("Editing: %s", lang.name.c_str());
    ImGui::Separator();

    // Per-translation settings (font overrides and RTL)
    if (ImGui::CollapsingHeader("Translation Settings"))
    {
        auto* project = editor_.GetApp().GetProject();
        auto* gd = project ? project->GetGameData() : nullptr;

        // Normal font combo
        ImGui::SetNextItemWidth(Dpi(200));
        const char* normal_preview = "Default";
        if (gd && lang.normal_font >= 0 && lang.normal_font < (int)gd->fonts.size())
            normal_preview = gd->fonts[lang.normal_font].name.c_str();
        else if (lang.normal_font >= 0)
        {
            static char nbuf[32];
            snprintf(nbuf, sizeof(nbuf), "Font %d", lang.normal_font);
            normal_preview = nbuf;
        }
        if (ImGui::BeginCombo("Normal Font", normal_preview))
        {
            if (ImGui::Selectable("Default", lang.normal_font < 0))
            {
                lang.normal_font = -1;
                lang.modified = true;
            }
            if (gd)
            {
                for (int i = 0; i < (int)gd->fonts.size(); i++)
                {
                    char label[128];
                    snprintf(label, sizeof(label), "%d: %s", i, gd->fonts[i].name.c_str());
                    if (ImGui::Selectable(label, lang.normal_font == i))
                    {
                        lang.normal_font = i;
                        lang.modified = true;
                    }
                }
            }
            ImGui::EndCombo();
        }

        // Speech font combo
        ImGui::SetNextItemWidth(Dpi(200));
        const char* speech_preview = "Default";
        if (gd && lang.speech_font >= 0 && lang.speech_font < (int)gd->fonts.size())
            speech_preview = gd->fonts[lang.speech_font].name.c_str();
        else if (lang.speech_font >= 0)
        {
            static char sbuf[32];
            snprintf(sbuf, sizeof(sbuf), "Font %d", lang.speech_font);
            speech_preview = sbuf;
        }
        if (ImGui::BeginCombo("Speech Font", speech_preview))
        {
            if (ImGui::Selectable("Default", lang.speech_font < 0))
            {
                lang.speech_font = -1;
                lang.modified = true;
            }
            if (gd)
            {
                for (int i = 0; i < (int)gd->fonts.size(); i++)
                {
                    char label[128];
                    snprintf(label, sizeof(label), "%d: %s", i, gd->fonts[i].name.c_str());
                    if (ImGui::Selectable(label, lang.speech_font == i))
                    {
                        lang.speech_font = i;
                        lang.modified = true;
                    }
                }
            }
            ImGui::EndCombo();
        }

        // Right-to-Left combo
        ImGui::SetNextItemWidth(Dpi(200));
        const char* rtl_items[] = { "Default", "Left-to-Right", "Right-to-Left" };
        int rtl_idx = 0;
        if (lang.right_to_left == 0) rtl_idx = 1;
        else if (lang.right_to_left == 1) rtl_idx = 2;
        else rtl_idx = 0; // -1 = default
        if (ImGui::Combo("Text Direction", &rtl_idx, rtl_items, 3))
        {
            if (rtl_idx == 0) lang.right_to_left = -1;
            else if (rtl_idx == 1) lang.right_to_left = 0;
            else lang.right_to_left = 1;
            lang.modified = true;
        }

        // Encoding
        ImGui::SetNextItemWidth(Dpi(200));
        const char* enc_items[] = { "UTF-8", "ASCII" };
        int enc_idx = 0;
        if (lang.encoding == "ASCII") enc_idx = 1;
        if (ImGui::Combo("Encoding", &enc_idx, enc_items, 2))
        {
            lang.encoding = enc_items[enc_idx];
            lang.modified = true;
        }
    }

    // Detect duplicate original strings for conflict highlighting
    std::set<std::string> seen_originals;
    std::set<std::string> duplicate_originals;
    for (const auto& entry : lang.entries)
    {
        if (!entry.original.empty())
        {
            if (!seen_originals.insert(entry.original).second)
                duplicate_originals.insert(entry.original);
        }
    }

    if (!duplicate_originals.empty())
    {
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f),
            "Warning: %d duplicate source strings detected (highlighted in orange)",
            (int)duplicate_originals.size());
    }

    if (ImGui::BeginTable("TranslationTable", 3,
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
        ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY))
    {
        ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, Dpi(40));
        ImGui::TableSetupColumn("Original Text", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Translation", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();

        for (int i = 0; i < (int)lang.entries.size(); i++)
        {
            auto& entry = lang.entries[i];

            // Apply filters
            if (show_untranslated_only_ && !entry.translated.empty())
                continue;

            if (filter_text_[0] != '\0')
            {
                // Case-insensitive search in original and translated
                std::string lower_orig = entry.original;
                std::string lower_trans = entry.translated;
                std::string lower_filter = filter_text_;
                std::transform(lower_orig.begin(), lower_orig.end(), lower_orig.begin(), ::tolower);
                std::transform(lower_trans.begin(), lower_trans.end(), lower_trans.begin(), ::tolower);
                std::transform(lower_filter.begin(), lower_filter.end(), lower_filter.begin(), ::tolower);

                if (lower_orig.find(lower_filter) == std::string::npos &&
                    lower_trans.find(lower_filter) == std::string::npos)
                    continue;
            }

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%d", i + 1);

            ImGui::TableSetColumnIndex(1);
            ImGui::TextWrapped("%s", entry.original.c_str());

            ImGui::TableSetColumnIndex(2);
            // Inline editing
            char buf[1024];
            snprintf(buf, sizeof(buf), "%s", entry.translated.c_str());
            ImGui::PushID(i);
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputText("##trans", buf, sizeof(buf)))
            {
                entry.translated = buf;
                entry.modified = true;
                lang.modified = true;
            }

            // Highlight untranslated
            if (entry.translated.empty())
            {
                ImU32 bg = IM_COL32(255, 200, 100, 40);
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, bg);
            }
            // Highlight duplicates
            else if (duplicate_originals.count(entry.original))
            {
                ImU32 bg = IM_COL32(255, 150, 50, 50);
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, bg);
            }

            ImGui::PopID();
        }
        ImGui::EndTable();
    }
}

} // namespace AGSEditor
