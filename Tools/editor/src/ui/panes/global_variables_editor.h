// AGS Editor ImGui - Global Variables Editor
// Operates directly on GameData (no copy-and-sync pattern).
// The edit dialog uses a staging copy for transactional OK/Cancel.
#pragma once

#include "ui/editor_ui.h"
#include "project/game_data.h"
#include <vector>
#include <string>

namespace AGSEditor
{

class GlobalVariablesEditor : public EditorPane
{
public:
    explicit GlobalVariablesEditor(EditorUI& editor);

    void Draw() override;
    const char* GetTitle() const override { return "Global Variables"; }

    struct TypeEntry
    {
        std::string name;          // e.g. "int", "Character", "DynamicSprite*"
        bool can_have_default;     // builtin types (int/String/float/bool) can
    };

private:
    void PopulateTypeList();
    bool ShowEditDialog(int var_index, bool is_new);
    bool IsBuiltinType(const std::string& type_name) const;
    bool CanHaveDefaultValue(const std::string& type_name, int array_type) const;
    bool ValidateDefaultValue(const std::string& type_name, const std::string& value) const;

    EditorUI& editor_;

    std::vector<TypeEntry> type_list_;
    int selected_var_ = -1;

    // Edit dialog state (staging copy for transactional OK/Cancel)
    bool show_edit_dialog_ = false;
    int edit_dialog_index_ = -1;
    bool edit_dialog_is_new_ = false;
    GameData::GlobalVariableInfo edit_dialog_var_;
    int edit_dialog_type_index_ = 0;
    std::string edit_dialog_error_;
};

} // namespace AGSEditor
