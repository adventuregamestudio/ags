// AGS Editor ImGui - Global Variables Editor implementation
// Operates directly on GameData::global_variables (no copy-and-sync).
// The edit dialog uses a staging copy for transactional OK/Cancel.
#include "global_variables_editor.h"
#include "ui/editor_ui.h"
#include "ui/log_panel.h"
#include "core/dpi_helper.h"
#include "project/project.h"
#include "project/game_data.h"
#include "core/script_name_validator.h"
#include "app.h"

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

#include <string>
#include <regex>
#include <fstream>
#include <algorithm>

namespace AGSEditor
{

// Alias for convenience
using TypeEntry = GlobalVariablesEditor::TypeEntry;

// Extract managed struct types from agsdefns.sh for the type dropdown
static std::vector<TypeEntry> ParseManagedTypesFromHeader()
{
    std::vector<TypeEntry> managed_types;

    // Try to find agsdefns.sh
    const char* search_paths[] = {
#ifdef SOURCE_DIR
        SOURCE_DIR "/../Editor/AGS.Editor/Resources/agsdefns.sh",
#endif
        "../Editor/AGS.Editor/Resources/agsdefns.sh",
        "Editor/AGS.Editor/Resources/agsdefns.sh",
        "/usr/share/ags/agsdefns.sh"
    };

    std::string header_content;
    for (const char* path : search_paths)
    {
        std::ifstream file(path);
        if (file.is_open())
        {
            header_content.assign(
                (std::istreambuf_iterator<char>(file)),
                std::istreambuf_iterator<char>());
            break;
        }
    }

    if (header_content.empty())
        return managed_types;

    // Parse managed struct definitions:
    // Pattern: (internalstring|autoptr|builtin|managed)+ struct Name
    std::regex struct_re(
        R"(\b(?:(internalstring|autoptr|builtin|managed)\s+)+struct\s+(\w+))",
        std::regex::ECMAScript);

    auto begin = std::sregex_iterator(
        header_content.begin(), header_content.end(), struct_re);
    auto end = std::sregex_iterator();

    for (auto it = begin; it != end; ++it)
    {
        std::string prefix = (*it)[0].str();
        std::string name = (*it)[2].str();

        bool is_managed = (prefix.find("managed") != std::string::npos);
        bool is_autoptr = (prefix.find("autoptr") != std::string::npos);
        bool is_string = (prefix.find("internalstring") != std::string::npos);

        if (!is_managed || is_string)
            continue;

        TypeEntry entry;
        if (is_autoptr)
            entry.name = name;        // e.g. "Character"
        else
            entry.name = name + "*";  // e.g. "DynamicSprite*"
        entry.can_have_default = false; // managed types cannot have default values
        managed_types.push_back(entry);
    }

    // Sort alphabetically
    std::sort(managed_types.begin(), managed_types.end(),
        [](const TypeEntry& a, const TypeEntry& b) { return a.name < b.name; });

    return managed_types;
}

GlobalVariablesEditor::GlobalVariablesEditor(EditorUI& editor)
    : editor_(editor)
{
    PopulateTypeList();
}

void GlobalVariablesEditor::PopulateTypeList()
{
    type_list_.clear();

    // Built-in types first
    type_list_.push_back({"int", true});
    type_list_.push_back({"String", true});
    type_list_.push_back({"float", true});
    type_list_.push_back({"bool", true});

    // Add managed types from agsdefns.sh
    auto managed = ParseManagedTypesFromHeader();
    for (auto& mt : managed)
        type_list_.push_back(std::move(mt));
}

bool GlobalVariablesEditor::IsBuiltinType(const std::string& type_name) const
{
    return type_name == "int" || type_name == "String" ||
           type_name == "float" || type_name == "bool";
}

bool GlobalVariablesEditor::CanHaveDefaultValue(const std::string& type_name, int array_type) const
{
    // Only builtin non-array types can have default values
    if (array_type != 0)
        return false;
    return IsBuiltinType(type_name);
}

bool GlobalVariablesEditor::ValidateDefaultValue(const std::string& type_name,
                                                  const std::string& value) const
{
    if (value.empty())
        return true;

    if (type_name == "int")
    {
        std::regex re(R"(^\-?[0-9]+$)");
        return std::regex_match(value, re);
    }
    else if (type_name == "float")
    {
        std::regex re(R"(^\-?[0-9]+(\.[0-9]+)?$)");
        return std::regex_match(value, re);
    }
    else if (type_name == "bool")
    {
        return value == "true" || value == "false";
    }
    else if (type_name == "String")
    {
        return true; // any string is valid
    }
    return false;
}

void GlobalVariablesEditor::Draw()
{
    auto* project = editor_.GetApp().GetProject();
    auto* gd = (project && project->IsLoaded()) ? project->GetGameData() : nullptr;
    if (!gd)
    {
        ImGui::TextDisabled("No project loaded.");
        return;
    }

    auto& variables = gd->global_variables;

    // Toolbar
    if (ImGui::Button("Add Variable"))
    {
        edit_dialog_is_new_ = true;
        edit_dialog_index_ = -1;
        edit_dialog_var_ = GameData::GlobalVariableInfo();
        edit_dialog_var_.name = "NewVariable" + std::to_string(variables.size());
        edit_dialog_var_.type_name = "int";
        edit_dialog_var_.default_value = "0";
        edit_dialog_type_index_ = 0; // "int"
        edit_dialog_error_.clear();
        show_edit_dialog_ = true;
        ImGui::OpenPopup("Edit Global Variable");
    }

    ImGui::SameLine();

    bool has_selection = (selected_var_ >= 0 && selected_var_ < (int)variables.size());
    if (!has_selection) ImGui::BeginDisabled();

    if (ImGui::Button("Edit"))
    {
        edit_dialog_is_new_ = false;
        edit_dialog_index_ = selected_var_;
        edit_dialog_var_ = variables[selected_var_];
        // Find the matching type index
        edit_dialog_type_index_ = 0;
        for (int i = 0; i < (int)type_list_.size(); i++)
        {
            if (type_list_[i].name == edit_dialog_var_.type_name)
            {
                edit_dialog_type_index_ = i;
                break;
            }
        }
        edit_dialog_error_.clear();
        show_edit_dialog_ = true;
        ImGui::OpenPopup("Edit Global Variable");
    }

    ImGui::SameLine();

    if (ImGui::Button("Remove"))
    {
        ImGui::OpenPopup("Confirm Remove Variable");
    }

    if (!has_selection) ImGui::EndDisabled();

    // Remove confirmation dialog
    if (ImGui::BeginPopupModal("Confirm Remove Variable", nullptr,
        ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Are you sure you want to remove variable '%s'?",
                     variables[selected_var_].name.c_str());
        ImGui::TextColored(ImVec4(1,0.5f,0,1),
            "Warning: Your game may no longer compile after removing this variable.");
        ImGui::Spacing();

        if (ImGui::Button("Remove", ImVec2(Dpi(120), 0)))
        {
            variables.erase(variables.begin() + selected_var_);
            if (selected_var_ >= (int)variables.size())
                selected_var_ = (int)variables.size() - 1;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(Dpi(120), 0)))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // Edit dialog
    ShowEditDialog(edit_dialog_index_, edit_dialog_is_new_);

    ImGui::Separator();

    // Variables table
    if (ImGui::BeginTable("GlobalVarsTable", 5,
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
        ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY))
    {
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, Dpi(140));
        ImGui::TableSetupColumn("Array", ImGuiTableColumnFlags_WidthFixed, Dpi(100));
        ImGui::TableSetupColumn("Default Value", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, Dpi(80));
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();

        for (int i = 0; i < (int)variables.size(); i++)
        {
            auto& var = variables[i];
            ImGui::TableNextRow();
            ImGui::PushID(i);

            bool selected = (i == selected_var_);

            // Highlight selected row
            if (selected)
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1,
                    ImGui::GetColorU32(ImGuiCol_Header));

            // Name (click to select, double-click to edit)
            ImGui::TableSetColumnIndex(0);
            std::string label = var.name + "##row";
            if (ImGui::Selectable(label.c_str(), selected,
                ImGuiSelectableFlags_SpanAllColumns |
                ImGuiSelectableFlags_AllowDoubleClick))
            {
                selected_var_ = i;
                if (ImGui::IsMouseDoubleClicked(0))
                {
                    edit_dialog_is_new_ = false;
                    edit_dialog_index_ = i;
                    edit_dialog_var_ = var;
                    edit_dialog_type_index_ = 0;
                    for (int t = 0; t < (int)type_list_.size(); t++)
                    {
                        if (type_list_[t].name == var.type_name)
                        {
                            edit_dialog_type_index_ = t;
                            break;
                        }
                    }
                    edit_dialog_error_.clear();
                    show_edit_dialog_ = true;
                    ImGui::OpenPopup("Edit Global Variable");
                }
            }

            // Type
            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(var.type_name.c_str());

            // Array type
            ImGui::TableSetColumnIndex(2);
            if (var.array_type == 1)
            {
                ImGui::Text("Array[%d]", var.array_size);
            }
            else if (var.array_type == 2)
            {
                ImGui::TextUnformatted("Dynamic[]");
            }
            else
            {
                ImGui::TextUnformatted("None");
            }

            // Default value
            ImGui::TableSetColumnIndex(3);
            if (CanHaveDefaultValue(var.type_name, var.array_type))
            {
                ImGui::TextUnformatted(var.default_value.c_str());
            }
            else
            {
                ImGui::TextDisabled("(N/A)");
            }

            // Actions
            ImGui::TableSetColumnIndex(4);
            if (ImGui::SmallButton("Up") && i > 0)
            {
                std::swap(variables[i], variables[i - 1]);
                if (selected_var_ == i) selected_var_ = i - 1;
                else if (selected_var_ == i - 1) selected_var_ = i;
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Dn") && i < (int)variables.size() - 1)
            {
                std::swap(variables[i], variables[i + 1]);
                if (selected_var_ == i) selected_var_ = i + 1;
                else if (selected_var_ == i + 1) selected_var_ = i;
            }

            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    // Help text
    ImGui::Spacing();
    ImGui::TextWrapped("Global variables are accessible from all scripts. "
        "They are declared in the global script header and can be used "
        "for game state like score, quest flags, inventory counts, etc. "
        "Double-click a variable to edit it.");
}

bool GlobalVariablesEditor::ShowEditDialog(int /*var_index*/, bool is_new)
{
    if (!show_edit_dialog_)
        return false;

    ImGui::SetNextWindowSize(ImVec2(Dpi(450), Dpi(350)), ImGuiCond_Appearing);
    if (!ImGui::BeginPopupModal("Edit Global Variable", &show_edit_dialog_,
        ImGuiWindowFlags_AlwaysAutoResize))
        return false;

    bool result = false;

    // Name
    ImGui::Text("Variable Name:");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##varname", &edit_dialog_var_.name);

    ImGui::Spacing();

    // Type dropdown with all types (builtin + managed)
    ImGui::Text("Type:");
    ImGui::SetNextItemWidth(-1);
    if (ImGui::BeginCombo("##vartype",
        edit_dialog_type_index_ < (int)type_list_.size()
            ? type_list_[edit_dialog_type_index_].name.c_str() : "int"))
    {
        for (int i = 0; i < (int)type_list_.size(); i++)
        {
            bool is_selected = (i == edit_dialog_type_index_);
            if (ImGui::Selectable(type_list_[i].name.c_str(), is_selected))
            {
                edit_dialog_type_index_ = i;
                edit_dialog_var_.type_name = type_list_[i].name;
                // Clear default value when switching to a type that cannot have one
                if (!type_list_[i].can_have_default)
                    edit_dialog_var_.default_value.clear();
            }
            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::Spacing();

    // Array type
    ImGui::Text("Array Type:");
    ImGui::SetNextItemWidth(-1);
    if (ImGui::Combo("##arrtype", &edit_dialog_var_.array_type, "None\0Array\0Dynamic Array\0"))
    {
        // Clear default value when array type set (arrays cannot have defaults)
        if (edit_dialog_var_.array_type != 0)
            edit_dialog_var_.default_value.clear();
    }

    if (edit_dialog_var_.array_type == 1)
    {
        ImGui::Text("Array Size:");
        ImGui::SetNextItemWidth(-1);
        ImGui::InputInt("##arrsize", &edit_dialog_var_.array_size, 1, 10);
        if (edit_dialog_var_.array_size < 1)
            edit_dialog_var_.array_size = 1;
    }

    ImGui::Spacing();

    // Default value (only for builtin non-array types)
    bool can_default = CanHaveDefaultValue(edit_dialog_var_.type_name,
                                            edit_dialog_var_.array_type);
    ImGui::Text("Default Value:");
    if (!can_default) ImGui::BeginDisabled();
    ImGui::SetNextItemWidth(-1);

    if (edit_dialog_var_.type_name == "bool" && can_default)
    {
        bool bval = (edit_dialog_var_.default_value == "true");
        if (ImGui::Checkbox("##boolval", &bval))
            edit_dialog_var_.default_value = bval ? "true" : "false";
    }
    else
    {
        ImGui::InputText("##defval", &edit_dialog_var_.default_value);
    }
    if (!can_default) ImGui::EndDisabled();

    // Show validation error
    if (!edit_dialog_error_.empty())
    {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1, 0.2f, 0.2f, 1), "%s", edit_dialog_error_.c_str());
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // OK / Cancel
    if (ImGui::Button("OK", ImVec2(Dpi(120), 0)))
    {
        // Validate name format
        std::string name_str = edit_dialog_var_.name;
        std::string err = ValidateScriptNameFormat(name_str);
        if (!err.empty())
        {
            edit_dialog_error_ = err;
        }
        else
        {
            // Validate name uniqueness
            auto* project2 = editor_.GetApp().GetProject();
            auto* gd2 = (project2 && project2->IsLoaded()) ? project2->GetGameData() : nullptr;
            if (gd2)
            {
                // Check uniqueness against all game entities
                err = ValidateScriptNameUnique(*gd2, name_str, "GlobalVariable");

                // Also check against other global variables (excluding self when editing)
                if (err.empty())
                {
                    for (int i = 0; i < (int)gd2->global_variables.size(); i++)
                    {
                        if (!is_new && i == edit_dialog_index_)
                            continue;
                        if (name_str == gd2->global_variables[i].name)
                        {
                            err = "A global variable with name '" + name_str + "' already exists.";
                            break;
                        }
                    }
                }
            }

            if (!err.empty())
            {
                edit_dialog_error_ = err;
            }
            else
            {
                // Validate default value
                if (can_default && !edit_dialog_var_.default_value.empty() &&
                    !ValidateDefaultValue(edit_dialog_var_.type_name,
                                          edit_dialog_var_.default_value))
                {
                    edit_dialog_error_ = "Invalid default value for type '" +
                                         edit_dialog_var_.type_name + "'.";
                }
                else
                {
                    auto* project3 = editor_.GetApp().GetProject();
                    auto* gd3 = (project3 && project3->IsLoaded()) ? project3->GetGameData() : nullptr;
                    if (gd3)
                    {
                        auto& vars = gd3->global_variables;
                        // All validation passed - apply changes
                        if (is_new)
                        {
                            vars.push_back(edit_dialog_var_);
                            selected_var_ = (int)vars.size() - 1;
                        }
                        else
                        {
                            std::string old_name = vars[edit_dialog_index_].name;
                            vars[edit_dialog_index_] = edit_dialog_var_;

                            // Track rename in log
                            if (old_name != name_str && !old_name.empty())
                            {
                                editor_.GetLogPanel().AddLog(
                                    "[Info] Global variable renamed: '%s' -> '%s'",
                                    old_name.c_str(), name_str.c_str());
                            }
                        }
                    }
                    show_edit_dialog_ = false;
                    ImGui::CloseCurrentPopup();
                    result = true;
                }
            }
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("Cancel", ImVec2(Dpi(120), 0)))
    {
        show_edit_dialog_ = false;
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
    return result;
}

} // namespace AGSEditor
