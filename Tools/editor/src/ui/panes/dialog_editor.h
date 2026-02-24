// AGS Editor ImGui - Dialog Editor pane
// Displays dialogs from loaded GameData.
#pragma once

#include "ui/editor_ui.h"
#include <string>
#include <memory>

class TextEditor;

namespace AGSEditor
{

class DialogEditor : public EditorPane
{
public:
    explicit DialogEditor(EditorUI& editor, int dialog_id = -1);
    ~DialogEditor() override;

    void Draw() override;
    const char* GetTitle() const override { return title_.c_str(); }

private:
    void DrawDialogList();
    void DrawDialogProperties();
    void DrawDialogOptions();
    void DrawAutoCompletePopup();

    EditorUI& editor_;
    std::string title_;

    int selected_dialog_ = 0;
    int selected_option_ = -1;
    bool confirm_delete_ = false;
    bool confirm_delete_option_ = false;
    int delete_option_index_ = -1;
    bool show_change_id_ = false;
    int change_id_target_ = 0;

    // Dialog script editor
    std::unique_ptr<TextEditor> script_editor_;
    int script_dialog_idx_ = -1; // which dialog is loaded in the editor

    // Autocomplete state
    bool show_autocomplete_ = false;
    std::string autocomplete_prefix_;
    int autocomplete_selected_ = 0;
    bool show_folder_tree_ = false;
    const void* selected_folder_ = nullptr;
};

} // namespace AGSEditor
