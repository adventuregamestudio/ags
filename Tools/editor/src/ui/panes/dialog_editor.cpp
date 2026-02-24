// AGS Editor ImGui - Dialog Editor
// Uses real dialog data from GameData.
#include "dialog_editor.h"
#include "ui/editor_ui.h"
#include "ui/log_panel.h"
#include "ui/project_panel.h"
#include "ui/folder_tree_widget.h"
#include "core/dpi_helper.h"
#include "project/project.h"
#include "project/game_data.h"
#include "project/script_api_data.h"
#include "core/script_name_validator.h"
#include "app.h"
#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"
#include "TextEditor.h"
#include <cstring>
#include <cstdio>
#include <algorithm>

namespace AGSEditor
{

static TextEditor::LanguageDefinition CreateDialogLanguageDefinition()
{
    TextEditor::LanguageDefinition langDef;
    langDef.mName = "Dialog Script";
    langDef.mAutoIndentation = true;
    langDef.mCaseSensitive = false;
    langDef.mCommentStart = "/*";
    langDef.mCommentEnd = "*/";
    langDef.mSingleLineComment = "//";

    // Dialog-specific keywords
    static const char* const keywords[] = {
        "return", "stop", "goto-dialog", "goto-previous", "run-script",
        "option-on", "option-off", "option-off-forever",
        "set-globalint", "set-speech-view", "set-text-parsable", "set-no-text-parsable",
        "new-room", "give-score", "play-sound", "play-music",
        "lose-inventory", "add-inv", "add-inventory", "lose-inv",
        "if", "else"
    };
    for (auto& k : keywords)
        langDef.mKeywords.insert(std::string(k));

    // Token regex rules
    langDef.mTokenRegexStrings.push_back(
        std::make_pair<std::string, TextEditor::PaletteIndex>(
            "@[0-9]+", TextEditor::PaletteIndex::Preprocessor));
    langDef.mTokenRegexStrings.push_back(
        std::make_pair<std::string, TextEditor::PaletteIndex>(
            "L?\\\"(\\\\.|[^\\\"])*\\\"", TextEditor::PaletteIndex::String));
    langDef.mTokenRegexStrings.push_back(
        std::make_pair<std::string, TextEditor::PaletteIndex>(
            "[+-]?[0-9]+", TextEditor::PaletteIndex::Number));
    langDef.mTokenRegexStrings.push_back(
        std::make_pair<std::string, TextEditor::PaletteIndex>(
            "[a-zA-Z_][a-zA-Z0-9_\\-]*", TextEditor::PaletteIndex::Identifier));

    return langDef;
}

DialogEditor::DialogEditor(EditorUI& editor, int dialog_id)
    : editor_(editor)
    , title_("Dialogs")
{
    if (dialog_id >= 0)
        selected_dialog_ = dialog_id;

    // Initialize the TextEditor for dialog scripts
    script_editor_ = std::make_unique<TextEditor>();
    static auto lang = CreateDialogLanguageDefinition();
    script_editor_->SetLanguageDefinition(lang);
    script_editor_->SetTabSize(2);
    script_editor_->SetShowWhitespaces(false);
}

DialogEditor::~DialogEditor() = default;

void DialogEditor::Draw()
{
    auto* project = editor_.GetApp().GetProject();
    if (!project || !project->IsLoaded())
    {
        ImGui::TextDisabled("No project loaded. Open a game to view dialogs.");
        return;
    }

    auto* gd = project->GetGameData();
    if (!gd)
    {
        ImGui::TextDisabled("No game data available.");
        return;
    }

    float list_width = Dpi(180);
    float tree_width = show_folder_tree_ ? Dpi(160) : 0.0f;
    ImVec2 avail = ImGui::GetContentRegionAvail();

    // Folder tree panel
    if (show_folder_tree_)
    {
        ImGui::BeginChild("##DlgFolders", ImVec2(tree_width, avail.y), ImGuiChildFlags_Borders);
        const auto* sel = static_cast<const FolderInfo*>(selected_folder_);
        const auto* new_sel = DrawFolderTreeWidget(sel, gd->dialog_folders, "All Dialogs", &gd->dialog_folders);
        if (new_sel != sel)
            selected_folder_ = new_sel;
        ImGui::EndChild();
        ImGui::SameLine();
    }

    // Dialog list
    ImGui::BeginChild("DlgList", ImVec2(list_width, avail.y), ImGuiChildFlags_Borders);
    DrawDialogList();
    ImGui::EndChild();

    ImGui::SameLine();

    // Dialog properties + options (split vertically)
    ImGui::BeginChild("DlgContent", ImVec2(0, avail.y), ImGuiChildFlags_Borders);

    float half_h = ImGui::GetContentRegionAvail().y * 0.5f;

    ImGui::BeginChild("DlgProps", ImVec2(0, half_h));
    DrawDialogProperties();
    ImGui::EndChild();

    ImGui::BeginChild("DlgOpts", ImVec2(0, 0));
    DrawDialogOptions();
    ImGui::EndChild();

    ImGui::EndChild();
}

void DialogEditor::DrawDialogList()
{
    auto* gd = editor_.GetApp().GetProject()->GetGameData();
    if (!gd) return;

    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "%d dialogs", (int)gd->dialogs.size());
    ImGui::SameLine();
    ImGui::Checkbox("Folders", &show_folder_tree_);

    if (ImGui::Button("+ New Dialog"))
    {
        GameData::DialogInfo new_dlg;
        new_dlg.id = (int)gd->dialogs.size();
        char sname[32];
        std::snprintf(sname, sizeof(sname), "dDialog%d", new_dlg.id);
        new_dlg.name = sname;
        new_dlg.option_count = 0;
        gd->dialogs.push_back(new_dlg);
        selected_dialog_ = (int)gd->dialogs.size() - 1;
    }

    ImGui::Separator();

    // Build folder filter set
    std::set<int> folder_ids;
    if (selected_folder_)
        CollectFolderItemIds(*static_cast<const FolderInfo*>(selected_folder_), folder_ids);

    for (int i = 0; i < (int)gd->dialogs.size(); i++)
    {
        auto& dlg = gd->dialogs[i];
        if (selected_folder_ && folder_ids.find(dlg.id) == folder_ids.end())
            continue;
        char label[128];
        std::snprintf(label, sizeof(label), "%d: %s", dlg.id, dlg.name.c_str());
        if (ImGui::Selectable(label, selected_dialog_ == i))
        {
            selected_dialog_ = i;
            selected_option_ = -1;
        }
        BeginItemDragSource(dlg.id, label);

        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Change Dialog ID..."))
            {
                selected_dialog_ = i;
                change_id_target_ = i;
                show_change_id_ = true;
            }
            if (ImGui::MenuItem("Duplicate"))
            {
                GameData::DialogInfo dup = dlg;
                dup.id = (int)gd->dialogs.size();
                dup.name += "_copy";
                gd->dialogs.push_back(dup);
                editor_.GetProjectPanel().MarkTreeDirty();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Delete..."))
            {
                selected_dialog_ = i;
                confirm_delete_ = true;
            }
            ImGui::EndPopup();
        }
    }
}

void DialogEditor::DrawDialogProperties()
{
    auto* gd = editor_.GetApp().GetProject()->GetGameData();
    if (!gd || gd->dialogs.empty() || selected_dialog_ < 0 || selected_dialog_ >= (int)gd->dialogs.size())
    {
        ImGui::TextDisabled("No dialog selected.");
        return;
    }

    // Deletion confirmation
    if (confirm_delete_)
    {
        ImGui::OpenPopup("Confirm Delete Dialog");
        confirm_delete_ = false;
    }
    if (ImGui::BeginPopupModal("Confirm Delete Dialog", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (selected_dialog_ >= 0 && selected_dialog_ < (int)gd->dialogs.size())
        {
            ImGui::Text("Delete dialog '%s'?", gd->dialogs[selected_dialog_].name.c_str());
            ImGui::Text("This action cannot be undone.");
            ImGui::Separator();
            if (ImGui::Button("Delete", ImVec2(Dpi(120), 0)))
            {
                gd->dialogs.erase(gd->dialogs.begin() + selected_dialog_);
                for (int i = 0; i < (int)gd->dialogs.size(); i++) gd->dialogs[i].id = i;
                if (selected_dialog_ >= (int)gd->dialogs.size())
                    selected_dialog_ = std::max(0, (int)gd->dialogs.size() - 1);
                editor_.GetProjectPanel().MarkTreeDirty();
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(Dpi(120), 0)))
                ImGui::CloseCurrentPopup();
        }
        else
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
        return;
    }

    // Change Dialog ID popup
    if (show_change_id_)
    {
        ImGui::OpenPopup("Change Dialog ID");
        show_change_id_ = false;
    }
    if (ImGui::BeginPopupModal("Change Dialog ID", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (selected_dialog_ >= 0 && selected_dialog_ < (int)gd->dialogs.size())
        {
            ImGui::Text("Change '%s' (ID %d) to new position:",
                gd->dialogs[selected_dialog_].name.c_str(), selected_dialog_);
            ImGui::InputInt("New ID", &change_id_target_);
            change_id_target_ = std::max(0, std::min(change_id_target_, (int)gd->dialogs.size() - 1));
            ImGui::Separator();

            if (ImGui::Button("Swap", ImVec2(Dpi(120), 0)))
            {
                if (change_id_target_ != selected_dialog_)
                {
                    std::swap(gd->dialogs[selected_dialog_], gd->dialogs[change_id_target_]);
                    for (int i = 0; i < (int)gd->dialogs.size(); i++) gd->dialogs[i].id = i;
                    selected_dialog_ = change_id_target_;
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
        return;
    }

    auto& dlg = gd->dialogs[selected_dialog_];

    ImGui::Text("Dialog ID: %d", dlg.id);
    ImGui::Separator();

    float field_w = Dpi(200);

    if (ImGui::CollapsingHeader("Properties", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SetNextItemWidth(field_w);
        ImGui::InputText("Name", &dlg.name);
        {
            std::string err = ValidateScriptName(*gd, dlg.name, "Dialog", dlg.id);
            if (!err.empty())
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", err.c_str());
        }
        ImGui::Text("Number of Options: %d", (int)dlg.options.size());
        ImGui::Checkbox("Show Text Parser", &dlg.show_text_parser);
    }

    if (ImGui::CollapsingHeader("Script"))
    {
        if (dlg.script.empty())
        {
            ImGui::TextDisabled("No dialog script loaded.");
        }
        else
        {
            // Load script into TextEditor when dialog selection changes
            if (script_dialog_idx_ != selected_dialog_)
            {
                script_editor_->SetText(dlg.script);
                script_dialog_idx_ = selected_dialog_;
            }

            ImVec2 avail = ImGui::GetContentRegionAvail();
            float script_h = std::max(avail.y - Dpi(8), Dpi(120));
            script_editor_->Render("##DlgScriptEditor",
                ImVec2(-FLT_MIN, script_h));

            // Sync changes back to game data
            if (script_editor_->IsTextChanged())
            {
                dlg.script = script_editor_->GetText();

                // Trigger autocomplete on dot or after typing 3+ chars
                auto pos = script_editor_->GetCursorPosition();
                auto lines = script_editor_->GetTextLines();
                if (pos.mLine < (int)lines.size())
                {
                    const auto& line = lines[pos.mLine];
                    if (pos.mColumn > 0 && pos.mColumn <= (int)line.size())
                    {
                        char last_char = line[pos.mColumn - 1];
                        if (last_char == '.')
                        {
                            autocomplete_prefix_ = "";
                            show_autocomplete_ = true;
                            autocomplete_selected_ = 0;
                        }
                        else if (isalnum(last_char) || last_char == '_')
                        {
                            int start = pos.mColumn - 1;
                            while (start > 0 && (isalnum(line[start - 1]) || line[start - 1] == '_'))
                                start--;
                            std::string word = line.substr(start, pos.mColumn - start);
                            if (word.size() >= 3)
                            {
                                autocomplete_prefix_ = word;
                                show_autocomplete_ = true;
                                autocomplete_selected_ = 0;
                            }
                        }
                        else
                        {
                            show_autocomplete_ = false;
                        }
                    }
                }
            }

            if (show_autocomplete_ && ImGui::IsKeyPressed(ImGuiKey_Escape))
                show_autocomplete_ = false;

            if (show_autocomplete_)
                DrawAutoCompletePopup();
        }
    }
}

void DialogEditor::DrawDialogOptions()
{
    auto* gd = editor_.GetApp().GetProject()->GetGameData();
    if (!gd || gd->dialogs.empty() || selected_dialog_ < 0 || selected_dialog_ >= (int)gd->dialogs.size())
        return;

    auto& dlg = gd->dialogs[selected_dialog_];

    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Dialog Options");

    if (ImGui::Button("+ Add Option"))
    {
        GameData::DialogInfo::DialogOption opt;
        char txt[32];
        std::snprintf(txt, sizeof(txt), "Option %d", (int)dlg.options.size() + 1);
        opt.text = txt;
        opt.show = true;
        opt.say = true;
        dlg.options.push_back(opt);
        dlg.option_count = (int)dlg.options.size();
        selected_option_ = (int)dlg.options.size() - 1;
    }

    ImGui::Separator();

    float field_w = Dpi(300);

    for (int i = 0; i < (int)dlg.options.size(); i++)
    {
        auto& opt = dlg.options[i];

        ImGui::PushID(i);

        char hdr[64];
        std::snprintf(hdr, sizeof(hdr), "Option %d", i + 1);
        bool open = ImGui::CollapsingHeader(hdr, ImGuiTreeNodeFlags_DefaultOpen);

        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Move Up") && i > 0)
                std::swap(dlg.options[i], dlg.options[i - 1]);
            if (ImGui::MenuItem("Move Down") && i < (int)dlg.options.size() - 1)
                std::swap(dlg.options[i], dlg.options[i + 1]);
            ImGui::Separator();
            if (ImGui::MenuItem("Delete Option"))
            {
                confirm_delete_option_ = true;
                delete_option_index_ = i;
            }
            ImGui::EndPopup();
        }

        if (open)
        {
            ImGui::SetNextItemWidth(field_w);
            ImGui::InputText("Text", &opt.text);
            ImGui::Checkbox("Show", &opt.show);
            ImGui::SameLine();
            ImGui::Checkbox("Say", &opt.say);
        }

        ImGui::PopID();
    }

    // Confirmation dialog for option deletion
    if (confirm_delete_option_)
    {
        ImGui::OpenPopup("Confirm Delete Option");
        confirm_delete_option_ = false;
    }
    if (ImGui::BeginPopupModal("Confirm Delete Option", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Are you sure you want to delete option %d?", delete_option_index_ + 1);
        ImGui::Separator();
        if (ImGui::Button("Delete", ImVec2(Dpi(120), 0)))
        {
            if (delete_option_index_ >= 0 && delete_option_index_ < (int)dlg.options.size())
            {
                dlg.options.erase(dlg.options.begin() + delete_option_index_);
                dlg.option_count = (int)dlg.options.size();
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(Dpi(120), 0)))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

void DialogEditor::DrawAutoCompletePopup()
{
    auto* project = editor_.GetApp().GetProject();
    if (!project) return;
    auto* api_data = project->GetScriptAPIData();
    if (!api_data) return;

    if (!show_autocomplete_ || autocomplete_prefix_.empty())
        return;

    auto matches = api_data->FindMatches(autocomplete_prefix_, 15);
    if (matches.empty())
    {
        show_autocomplete_ = false;
        return;
    }

    ImGui::SetNextWindowPos(ImGui::GetCursorScreenPos());
    if (ImGui::Begin("##dlg_autocomplete", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing))
    {
        for (int i = 0; i < (int)matches.size(); i++)
        {
            bool selected = (autocomplete_selected_ == i);
            if (ImGui::Selectable(matches[i]->name.c_str(), selected))
            {
                std::string remainder = matches[i]->name.substr(autocomplete_prefix_.size());
                script_editor_->InsertText(remainder);
                show_autocomplete_ = false;
            }

            if (ImGui::IsItemHovered() && !matches[i]->declaration.empty())
            {
                ImGui::SetTooltip("%s", matches[i]->declaration.c_str());
            }
        }
    }
    ImGui::End();
}

} // namespace AGSEditor
