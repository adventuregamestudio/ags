// AGS Editor ImGui - Script Editor (one file per pane)
// Uses ImGuiColorTextEdit for code editing.
// Each opened script file becomes its own EditorPane tab.
#pragma once

#include "ui/editor_ui.h"
#include <string>
#include <vector>
#include <memory>
#include <filesystem>
#include <unordered_set>

// Forward declare TextEditor
class TextEditor;

namespace AGSEditor
{

class ScriptAPIData;

class ScriptEditor : public EditorPane
{
public:
    explicit ScriptEditor(EditorUI& editor, const std::string& filename = "GlobalScript.asc");
    ~ScriptEditor();

    void Draw() override;
    const char* GetTitle() const override { return title_.c_str(); }
    bool IsModified() const override { return modified_; }
    std::string GetHelpKeyword() const override;
    void Close() override;

    // The script filename this editor manages
    const std::string& GetFilename() const { return filename_; }

    // Get the text content (for compilation etc.)
    std::string GetCurrentText() const;

    // Navigate to a specific line (1-based)
    void GoToLine(int line);

    // Get breakpoints from the text editor (1-based line numbers)
    std::unordered_set<int> GetBreakpoints() const;

    // Set the execution point marker (yellow arrow) at a line.
    // Pass 0 to clear.
    void SetExecutionLine(int line);

    // Set/clear a runtime debug error marker at a specific line.
    void SetDebugError(int line, const std::string& message);
    void ClearDebugError();

private:
    void DrawToolbar();
    void DrawEditor();
    void DrawFindReplaceBar();
    void DrawGoToLineDialog();
    void DrawAutoCompletePopup();
    void DrawCompileErrors();

    // Setup AGS script language definition for TextEditor
    void SetupAGSLanguage(TextEditor& te);

    // Compile current script
    void CompileCurrentScript();

    // Add game-specific identifiers (characters, GUIs, etc.) for highlighting
    void PopulateGameIdentifiers();

    // Find/Replace
    void FindNext();
    void FindPrevious();
    void ReplaceNext();
    void ReplaceAll();

    // Find All Usages across all project scripts
    void FindAllUsages();

    // Get the word under cursor (or selected text)
    std::string GetWordUnderCursor() const;

    // Code editing
    void ToggleLineComment();

    // Function list parsing
    struct FunctionEntry {
        std::string name;       // Display name (e.g. "function game_start()")
        int line;               // 0-based line number
    };
    void RebuildFunctionList();

    EditorUI& editor_;

    // Single file state
    std::string filename_;
    std::string title_;         // tab display title (short name, e.g. "GlobalScript.asc")
    std::unique_ptr<TextEditor> text_editor_;
    bool modified_ = false;

    // Function dropdown
    std::vector<FunctionEntry> function_list_;
    int selected_function_ = -1;
    bool function_list_dirty_ = true;

    // Find & Replace
    bool show_find_replace_ = false;
    char find_text_[256] = {};
    char replace_text_[256] = {};
    bool find_case_sensitive_ = false;
    bool find_whole_word_ = false;

    // Go to line dialog
    bool show_goto_line_ = false;
    int goto_line_number_ = 1;

    // Auto-complete
    bool show_autocomplete_ = false;
    std::string autocomplete_prefix_;
    int autocomplete_selected_ = 0;

    // Compilation errors
    struct CompileError {
        int line;
        std::string message;
    };
    std::vector<CompileError> compile_errors_;
    bool show_errors_ = false;

    // External file change detection
    std::filesystem::file_time_type last_file_time_{};
    float file_check_timer_ = 0.0f;
    bool show_reload_prompt_ = false;

    // Close confirmation
    bool pending_close_ = false;
};

} // namespace AGSEditor
