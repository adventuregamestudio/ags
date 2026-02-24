// AGS Editor ImGui - Script Editor implementation
// Each opened script file is its own EditorPane tab in the main tab bar.
// Features: Syntax highlighting, line numbers, auto-indentation,
// bracket matching, find/replace, go-to-line,
// compilation with error markers, auto-complete
#include "script_editor.h"
#include "find_results_pane.h"
#include "ui/editor_ui.h"
#include "ui/log_panel.h"
#include "project/project.h"
#include "project/game_data.h"
#include "core/dpi_helper.h"
#include "app.h"
#include "project/script_api_data.h"
#include "compiler/compiler_bridge.h"
#include "imgui.h"
#include "TextEditor.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <regex>

namespace fs = std::filesystem;

namespace AGSEditor
{

// -- AGS Script Language Definition --
static TextEditor::LanguageDefinition CreateAGSLanguageDefinition()
{
    TextEditor::LanguageDefinition langDef;
    langDef.mName = "AGS Script";

    // AGS Script keywords
    static const char* const keywords[] = {
        "if", "else", "while", "for", "do", "return", "break", "continue",
        "switch", "case", "default", "new", "delete", "null", "true", "false",
        "function", "int", "short", "char", "float", "bool", "void", "string",
        "import", "export", "readonly", "writeprotected", "protected",
        "static", "managed", "struct", "extends", "enum",
        "attribute", "autoptr", "builtin", "internalstring", "noloopcheck",
        "this", "const"
    };
    for (auto& k : keywords)
        langDef.mKeywords.insert(k);

    // Built-in identifiers (types/classes)
    static const char* const identifiers[] = {
        "Character", "Object", "Hotspot", "Region", "Room", "Game",
        "GUI", "GUIControl", "Button", "Label", "Slider", "ListBox",
        "TextBox", "InvWindow", "InventoryItem", "Dialog",
        "DrawingSurface", "DynamicSprite", "File", "Maths", "Mouse",
        "Overlay", "Parser", "System", "String", "ViewFrame",
        "AudioChannel", "AudioClip", "DateTime", "Dictionary",
        "Set", "Camera", "Viewport", "Screen", "Speech", "Point"
    };
    for (auto& id : identifiers)
    {
        TextEditor::Identifier ident;
        ident.mDeclaration = std::string("builtin type ") + id;
        langDef.mIdentifiers.insert(std::make_pair(std::string(id), ident));
    }

    // Built-in enums
    static const char* const enum_values[] = {
        // eKeyCode
        "eKeyNone", "eKeyCtrlA", "eKeyCtrlB", "eKeyCtrlC", "eKeyCtrlD",
        "eKeyCtrlE", "eKeyCtrlF", "eKeyCtrlG", "eKeyBackspace", "eKeyTab",
        "eKeyCtrlK", "eKeyCtrlL", "eKeyReturn", "eKeyCtrlN", "eKeyCtrlO",
        "eKeyCtrlP", "eKeyCtrlQ", "eKeyCtrlR", "eKeyCtrlS", "eKeyCtrlT",
        "eKeyCtrlU", "eKeyCtrlV", "eKeyCtrlW", "eKeyCtrlX", "eKeyCtrlY",
        "eKeyCtrlZ", "eKeyEscape", "eKeySpace",
        "eKeyF1", "eKeyF2", "eKeyF3", "eKeyF4", "eKeyF5", "eKeyF6",
        "eKeyF7", "eKeyF8", "eKeyF9", "eKeyF10", "eKeyF11", "eKeyF12",
        "eKeyHome", "eKeyEnd", "eKeyLeftArrow", "eKeyRightArrow",
        "eKeyUpArrow", "eKeyDownArrow", "eKeyInsert", "eKeyDelete",
        "eKey0", "eKey1", "eKey2", "eKey3", "eKey4",
        "eKey5", "eKey6", "eKey7", "eKey8", "eKey9",
        "eKeyA", "eKeyB", "eKeyC", "eKeyD", "eKeyE", "eKeyF",
        "eKeyG", "eKeyH", "eKeyI", "eKeyJ", "eKeyK", "eKeyL",
        "eKeyM", "eKeyN", "eKeyO", "eKeyP", "eKeyQ", "eKeyR",
        "eKeyS", "eKeyT", "eKeyU", "eKeyV", "eKeyW", "eKeyX",
        "eKeyY", "eKeyZ",
        // MouseButton
        "eMouseLeft", "eMouseRight", "eMouseMiddle",
        "eMouseLeftInv", "eMouseRightInv", "eMouseMiddleInv",
        "eMouseWheelNorth", "eMouseWheelSouth",
        // Alignment
        "eAlignLeft", "eAlignCentre", "eAlignCenter", "eAlignRight",
        "eAlignHasLeft", "eAlignHasRight", "eAlignHasCentre",
        "eAlignHasTop", "eAlignHasBottom", "eAlignHasMiddle",
        // TransitionStyle
        "eTransitionFade", "eTransitionInstant", "eTransitionDissolve",
        "eTransitionBoxout", "eTransitionCrossfade",
        // CursorMode
        "eModeWalkto", "eModeLookat", "eModeInteract", "eModeTalkto",
        "eModeUseinv", "eModePickup", "eModePointer", "eModeWait",
        "eModeUsermode1", "eModeUsermode2",
        // CharacterDirection
        "eDirectionDown", "eDirectionLeft", "eDirectionRight", "eDirectionUp",
        "eDirectionDownLeft", "eDirectionDownRight",
        "eDirectionUpLeft", "eDirectionUpRight", "eDirectionNone",
        // DialogOptionState
        "eOptionOff", "eOptionOn", "eOptionOffForever",
        // Blocking
        "eBlock", "eNoBlock",
        // RepeatStyle
        "eOnce", "eRepeat",
        // VolChangeType
        "eVolChangeExisting", "eVolSetFutureDefault", "eVolExistingAndFuture",
        // AudioPriority
        "eAudioPriorityVeryLow", "eAudioPriorityLow", "eAudioPriorityNormal",
        "eAudioPriorityHigh", "eAudioPriorityVeryHigh",
    };
    for (auto& ev : enum_values)
    {
        TextEditor::Identifier ident;
        ident.mDeclaration = std::string("enum ") + ev;
        langDef.mIdentifiers.insert(std::make_pair(std::string(ev), ident));
    }

    // Built-in functions
    static const char* const functions[] = {
        "Display", "DisplayAt", "DisplayMessage", "QuitGame", "RestartGame",
        "SaveGameSlot", "RestoreGameSlot", "Wait", "WaitKey", "WaitMouseKey",
        "IsGamePaused", "PauseGame", "UnPauseGame", "SetTimer", "IsTimerExpired",
        "Random", "FloatToInt", "IntToFloat", "AbortGame",
        "ProcessClick", "IsInteractionAvailable",
        "SetBackgroundFrame", "GetBackgroundFrame",
        "FadeIn", "FadeOut", "ShakeScreen",
        "SetScreenTransition", "Debug", "ClaimEvent",
        "GiveScore", "InputBox", "StartCutscene", "EndCutscene"
    };
    for (auto& fn : functions)
    {
        TextEditor::Identifier ident;
        ident.mDeclaration = std::string("function ") + fn + "(...)";
        langDef.mIdentifiers.insert(std::make_pair(std::string(fn), ident));
    }

    // C-like comments
    langDef.mCommentStart = "/*";
    langDef.mCommentEnd = "*/";
    langDef.mSingleLineComment = "//";

    langDef.mAutoIndentation = true;
    langDef.mCaseSensitive = true;

    // Token regex patterns for AGS script
    langDef.mTokenRegexStrings.push_back(
        std::make_pair<std::string, TextEditor::PaletteIndex>(
            "[ \\t]*#[ \\t]*[a-zA-Z_]+", TextEditor::PaletteIndex::Preprocessor));
    langDef.mTokenRegexStrings.push_back(
        std::make_pair<std::string, TextEditor::PaletteIndex>(
            "L?\\\"(\\\\.|[^\\\"])*\\\"", TextEditor::PaletteIndex::String));
    langDef.mTokenRegexStrings.push_back(
        std::make_pair<std::string, TextEditor::PaletteIndex>(
            "\\'\\\\?[^\\']\\'", TextEditor::PaletteIndex::CharLiteral));
    langDef.mTokenRegexStrings.push_back(
        std::make_pair<std::string, TextEditor::PaletteIndex>(
            "[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?[fF]?", TextEditor::PaletteIndex::Number));
    langDef.mTokenRegexStrings.push_back(
        std::make_pair<std::string, TextEditor::PaletteIndex>(
            "0[xX][0-9a-fA-F]+[uU]?[lL]?[lL]?", TextEditor::PaletteIndex::Number));
    langDef.mTokenRegexStrings.push_back(
        std::make_pair<std::string, TextEditor::PaletteIndex>(
            "[a-zA-Z_][a-zA-Z0-9_]*", TextEditor::PaletteIndex::Identifier));
    langDef.mTokenRegexStrings.push_back(
        std::make_pair<std::string, TextEditor::PaletteIndex>(
            "[\\[\\]\\{\\}\\!\\%\\^\\&\\*\\(\\)\\-\\+\\=\\~\\|\\<\\>\\?\\/\\;\\,\\.]",
            TextEditor::PaletteIndex::Punctuation));

    return langDef;
}

// Extracts user-defined struct and enum type names from script text
static void ExtractUserTypeNames(const std::string& text,
    std::vector<std::string>& out_names)
{
    // Match "struct <Name>" and "enum <Name>" at word boundaries
    // Simple line-by-line scan avoids full regex overhead
    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line))
    {
        // Skip comment lines
        size_t first = line.find_first_not_of(" \t");
        if (first == std::string::npos) continue;
        if (line[first] == '/' && first + 1 < line.size() && line[first + 1] == '/') continue;

        // Look for "struct" or "enum" keyword
        for (const char* kw : {"struct", "enum"})
        {
            size_t pos = 0;
            size_t kwlen = strlen(kw);
            while ((pos = line.find(kw, pos)) != std::string::npos)
            {
                // Check word boundary before
                if (pos > 0 && (isalnum(line[pos - 1]) || line[pos - 1] == '_'))
                {
                    pos += kwlen;
                    continue;
                }
                // Check word boundary after
                size_t after = pos + kwlen;
                if (after >= line.size() || line[after] != ' ')
                {
                    pos += kwlen;
                    continue;
                }
                // Skip whitespace to find the type name
                size_t name_start = line.find_first_not_of(" \t", after);
                if (name_start == std::string::npos) break;
                size_t name_end = name_start;
                while (name_end < line.size() &&
                    (isalnum(line[name_end]) || line[name_end] == '_'))
                    name_end++;
                if (name_end > name_start)
                {
                    out_names.push_back(line.substr(name_start, name_end - name_start));
                }
                pos = name_end;
            }
        }
    }
}

void ScriptEditor::PopulateGameIdentifiers()
{
    auto* project = editor_.GetProject();
    auto* gd = (project && project->IsLoaded()) ? project->GetGameData() : nullptr;
    if (!gd || !text_editor_) return;

    // Add character script names (e.g. cEgo)
    for (const auto& ch : gd->characters)
    {
        if (!ch.script_name.empty())
            text_editor_->AddIdentifier(ch.script_name,
                "Character " + ch.real_name);
    }

    // Add GUI names (e.g. gStatusline)
    for (const auto& gui : gd->guis)
    {
        if (!gui.name.empty())
            text_editor_->AddIdentifier(gui.name,
                "GUI " + gui.name);
        // GUI control names
        for (const auto& ctrl : gui.controls)
        {
            if (!ctrl.name.empty())
                text_editor_->AddIdentifier(ctrl.name,
                    ctrl.type_tag + " " + ctrl.name);
        }
    }

    // Add inventory item script names (e.g. iKey)
    for (const auto& inv : gd->inventory_items)
    {
        if (!inv.script_name.empty())
            text_editor_->AddIdentifier(inv.script_name,
                "InventoryItem " + inv.description);
    }

    // Add audio clip script names (e.g. aMusic1)
    for (const auto& clip : gd->audio_clips)
    {
        if (!clip.name.empty())
            text_editor_->AddIdentifier(clip.name,
                "AudioClip " + clip.name);
    }

    // Add dialog names (e.g. dDialog0)
    for (const auto& dlg : gd->dialogs)
    {
        if (!dlg.name.empty())
            text_editor_->AddIdentifier(dlg.name,
                "Dialog " + dlg.name);
    }

    // Parse struct and enum names from script module headers
    for (const auto& mod : gd->script_modules)
    {
        if (mod.header_file.empty()) continue;
        std::string header_path =
            (fs::path(project->GetProjectDir()) / mod.header_file).string();
        std::ifstream f(header_path);
        if (!f.is_open()) continue;
        std::stringstream ss;
        ss << f.rdbuf();
        std::string header_text = ss.str();

        std::vector<std::string> type_names;
        ExtractUserTypeNames(header_text, type_names);
        for (const auto& name : type_names)
            text_editor_->AddIdentifier(name, "user type " + name);
    }
}

// -- ScriptEditor Implementation --

ScriptEditor::ScriptEditor(EditorUI& editor, const std::string& filename)
    : editor_(editor), filename_(filename)
{
    // Extract just the filename for the tab title
    fs::path p(filename);
    title_ = p.filename().string();

    // For room scripts, build a richer title with room description
    std::string stem = p.stem().string();
    if (stem.size() > 4 && stem.substr(0, 4) == "room")
    {
        int room_num = -1;
        try { room_num = std::stoi(stem.substr(4)); } catch (...) {}
        if (room_num >= 0)
        {
            auto* project = editor_.GetProject();
            auto* gd = (project && project->IsLoaded()) ? project->GetGameData() : nullptr;
            bool is_header = (p.extension() == ".ash");
            std::string suffix = is_header ? " Header" : " Script";
            title_ = "Room " + std::to_string(room_num) + suffix;
            if (gd)
            {
                for (const auto& ri : gd->rooms)
                {
                    if (ri.number == room_num && !ri.description.empty())
                    {
                        title_ = "Room " + std::to_string(room_num) + suffix + ": " + ri.description;
                        break;
                    }
                }
            }
        }
    }

    // Create TextEditor instance
    text_editor_ = std::make_unique<TextEditor>();

    // Set language definition
    static auto lang = CreateAGSLanguageDefinition();
    text_editor_->SetLanguageDefinition(lang);
    text_editor_->SetTabSize(2);
    text_editor_->SetShowWhitespaces(false);

    // Add game-specific identifiers for syntax highlighting
    PopulateGameIdentifiers();

    // Try to load the file content
    std::string content;
    auto* project = editor_.GetProject();
    if (project && project->IsLoaded())
    {
        std::string filepath = (fs::path(project->GetProjectDir()) / filename).string();
        std::ifstream file(filepath);
        if (file.is_open())
        {
            std::stringstream ss;
            ss << file.rdbuf();
            content = ss.str();
            editor_.GetLogPanel().AddLog("[Info] Loaded script: %s", filename.c_str());
        }
        else
        {
            editor_.GetLogPanel().AddLog("[Info] Script file not found, using template: %s", filepath.c_str());
        }
    }

    // If no file content, use a default template
    if (content.empty())
    {
        bool is_header = fs::path(filename).extension() == ".ash";
        if (is_header)
        {
            content =
                "// " + filename + "\n"
                "//\n"
                "// AGS Script Header\n"
                "//\n"
                "\n"
                "// Place your script header imports and declarations here.\n"
                "// For example:\n"
                "//   import function MyFunction(int param);\n"
                "\n";
        }
        else
        {
            content =
                "// " + filename + "\n"
                "//\n"
                "// AGS Script file\n"
                "//\n"
                "\n"
                "function game_start()\n"
                "{\n"
                "  // Called when the game starts, before the first room is loaded.\n"
                "  // Put initialization code here.\n"
                "}\n"
                "\n"
                "function repeatedly_execute()\n"
                "{\n"
                "  // Called every game cycle (usually 40 times per second).\n"
                "}\n"
                "\n"
                "function on_key_press(eKeyCode keycode, int mod)\n"
                "{\n"
                "  // Called when a key is pressed.\n"
                "  if (keycode == eKeyEscape)\n"
                "  {\n"
                "    QuitGame(0);\n"
                "  }\n"
                "}\n"
                "\n"
                "function on_mouse_click(MouseButton button)\n"
                "{\n"
                "  // Called when a mouse button is clicked.\n"
                "}\n";
        }
    }

    text_editor_->SetText(content);
    modified_ = false;

    // Record file modification time for external change detection
    if (project && project->IsLoaded())
    {
        std::string filepath = (fs::path(project->GetProjectDir()) / filename).string();
        std::error_code ec;
        last_file_time_ = fs::last_write_time(filepath, ec);
    }
}

ScriptEditor::~ScriptEditor() = default;

std::string ScriptEditor::GetCurrentText() const
{
    return text_editor_->GetText();
}

void ScriptEditor::GoToLine(int line)
{
    // TextEditor uses 0-based line numbers
    text_editor_->SetCursorPosition(TextEditor::Coordinates(line - 1, 0));
}

std::unordered_set<int> ScriptEditor::GetBreakpoints() const
{
    return text_editor_->GetBreakpoints();
}

void ScriptEditor::SetExecutionLine(int line)
{
    text_editor_->SetExecutionLine(line);
}

void ScriptEditor::SetDebugError(int line, const std::string& message)
{
    TextEditor::ErrorMarkers markers;
    markers[line] = message;
    text_editor_->SetErrorMarkers(markers);
}

void ScriptEditor::ClearDebugError()
{
    text_editor_->SetErrorMarkers({});
}

std::string ScriptEditor::GetHelpKeyword() const
{
    return GetWordUnderCursor();
}

std::string ScriptEditor::GetWordUnderCursor() const
{
    // Try selected text first
    std::string sel = text_editor_->GetSelectedText();
    if (!sel.empty())
        return sel;

    // Extract word under cursor from current line
    auto pos = text_editor_->GetCursorPosition();
    std::string line = text_editor_->GetCurrentLineText();
    if (line.empty() || pos.mColumn >= (int)line.size())
        return {};

    // Find word boundaries
    int start = pos.mColumn;
    int end = pos.mColumn;
    auto is_word = [](char c) { return std::isalnum((unsigned char)c) || c == '_'; };
    while (start > 0 && is_word(line[start - 1]))
        --start;
    while (end < (int)line.size() && is_word(line[end]))
        ++end;
    if (start >= end)
        return {};
    return line.substr(start, end - start);
}

void ScriptEditor::Draw()
{
    // Unique ID scope per filename to avoid collisions between script panes
    ImGui::PushID(filename_.c_str());

    // External file change detection (check every 2 seconds)
    auto& io = ImGui::GetIO();
    file_check_timer_ += io.DeltaTime;
    if (file_check_timer_ >= 2.0f && !show_reload_prompt_)
    {
        file_check_timer_ = 0.0f;
        auto* project = editor_.GetProject();
        if (project && project->IsLoaded())
        {
            std::string filepath = (fs::path(project->GetProjectDir()) / filename_).string();
            std::error_code ec;
            auto current_time = fs::last_write_time(filepath, ec);
            if (!ec && current_time != last_file_time_ &&
                last_file_time_ != std::filesystem::file_time_type{})
            {
                show_reload_prompt_ = true;
            }
        }
    }

    // Reload prompt dialog
    if (show_reload_prompt_)
    {
        ImGui::OpenPopup("File Changed##reload");
        if (ImGui::BeginPopupModal("File Changed##reload", nullptr,
            ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("The file '%s' has been modified externally.",
                        filename_.c_str());
            ImGui::Text("Do you want to reload it?");
            ImGui::Spacing();
            if (ImGui::Button("Reload", ImVec2(Dpi(100), 0)))
            {
                auto* project = editor_.GetProject();
                if (project && project->IsLoaded())
                {
                    std::string filepath =
                        (fs::path(project->GetProjectDir()) / filename_).string();
                    std::ifstream file(filepath);
                    if (file.is_open())
                    {
                        std::stringstream ss;
                        ss << file.rdbuf();
                        text_editor_->SetText(ss.str());
                        modified_ = false;
                        function_list_dirty_ = true;
                        std::error_code ec;
                        last_file_time_ = fs::last_write_time(filepath, ec);
                    }
                }
                show_reload_prompt_ = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Ignore", ImVec2(Dpi(100), 0)))
            {
                // Update stored time so we don't keep prompting
                auto* project = editor_.GetProject();
                if (project && project->IsLoaded())
                {
                    std::string filepath =
                        (fs::path(project->GetProjectDir()) / filename_).string();
                    std::error_code ec;
                    last_file_time_ = fs::last_write_time(filepath, ec);
                }
                show_reload_prompt_ = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    DrawToolbar();
    ImGui::Separator();
    DrawEditor();

    if (show_find_replace_)
        DrawFindReplaceBar();
    if (show_goto_line_)
        DrawGoToLineDialog();
    if (show_errors_ && !compile_errors_.empty())
        DrawCompileErrors();

    // Close confirmation popup for unsaved changes
    if (pending_close_)
    {
        ImGui::OpenPopup("Unsaved Changes");
        pending_close_ = false;
    }
    if (ImGui::BeginPopupModal("Unsaved Changes", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("'%s' has unsaved changes.\nDo you want to save before closing?", filename_.c_str());
        ImGui::Separator();

        if (ImGui::Button("Save & Close", ImVec2(120, 0)))
        {
            // Save
            auto* project = editor_.GetProject();
            if (project && project->IsLoaded())
            {
                std::string filepath = (fs::path(project->GetProjectDir()) / filename_).string();
                std::ofstream file(filepath);
                if (file.is_open())
                {
                    file << text_editor_->GetText();
                    modified_ = false;
                    editor_.GetLogPanel().AddLog("[Info] Saved: %s", filename_.c_str());
                }
                else
                {
                    editor_.GetLogPanel().AddLog("[Error] Failed to save script: %s", filepath.c_str());
                }
            }
            open_ = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Discard", ImVec2(90, 0)))
        {
            open_ = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(90, 0)))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::PopID();
}

void ScriptEditor::Close()
{
    if (modified_)
    {
        pending_close_ = true;  // Will open the confirmation popup next frame
    }
    else
    {
        open_ = false;
    }
}

void ScriptEditor::DrawToolbar()
{
    // Save button
    if (ImGui::Button("Save"))
    {
        auto* project = editor_.GetProject();
        if (project && project->IsLoaded())
        {
            std::string filepath = (fs::path(project->GetProjectDir()) / filename_).string();
            std::ofstream file(filepath);
            if (file.is_open())
            {
                file << text_editor_->GetText();
                modified_ = false;
                editor_.GetLogPanel().AddLog("[Info] Saved: %s", filename_.c_str());
            }
            else
            {
                editor_.GetLogPanel().AddLog("[Error] Cannot save: %s", filepath.c_str());
            }
        }
        else
        {
            editor_.GetLogPanel().AddLog("[Warning] No project open, cannot save.");
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Compile"))
    {
        CompileCurrentScript();
    }

    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();

    // Find/Replace toggle (Ctrl+F)
    if (ImGui::Button("Find"))
    {
        show_find_replace_ = !show_find_replace_;
    }

    ImGui::SameLine();
    if (ImGui::Button("Go To"))
    {
        show_goto_line_ = !show_goto_line_;
    }

    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    // Function dropdown — list all functions in the current script
    if (function_list_dirty_)
        RebuildFunctionList();

    {
        // Show current function based on cursor position
        auto cursor = text_editor_->GetCursorPosition();
        int best = -1;
        for (int i = 0; i < (int)function_list_.size(); i++)
        {
            if (function_list_[i].line <= cursor.mLine)
                best = i;
        }
        selected_function_ = best;

        const char* preview = (selected_function_ >= 0)
            ? function_list_[selected_function_].name.c_str()
            : "(no function)";
        ImGui::SetNextItemWidth(Dpi(200));
        if (ImGui::BeginCombo("##FuncList", preview))
        {
            for (int i = 0; i < (int)function_list_.size(); i++)
            {
                bool is_sel = (selected_function_ == i);
                if (ImGui::Selectable(function_list_[i].name.c_str(), is_sel))
                {
                    text_editor_->SetCursorPosition(
                        TextEditor::Coordinates(function_list_[i].line, 0));
                }
                if (is_sel)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
    }

    // Keyboard shortcuts
    auto& kb_io = ImGui::GetIO();
    if (kb_io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_F))
        show_find_replace_ = !show_find_replace_;
    if (kb_io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_G))
        show_goto_line_ = !show_goto_line_;
    if (kb_io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S))
    {
        auto* project = editor_.GetProject();
        if (project && project->IsLoaded())
        {
            std::string filepath = (fs::path(project->GetProjectDir()) / filename_).string();
            std::ofstream file(filepath);
            if (file.is_open())
            {
                file << text_editor_->GetText();
                file.close();
                modified_ = false;
                // Update file time so we don't trigger reload prompt
                std::error_code ec;
                last_file_time_ = fs::last_write_time(filepath, ec);
                editor_.GetLogPanel().AddLog("[Info] Saved: %s", filename_.c_str());
            }
            else
            {
                editor_.GetLogPanel().AddLog("[Error] Failed to save script: %s", filepath.c_str());
            }
        }
    }

    // Ctrl+M — Switch between matching .asc/.ash files
    if (kb_io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_M))
    {
        fs::path current(filename_);
        std::string ext = current.extension().string();
        std::string partner;
        if (ext == ".asc")
            partner = current.stem().string() + ".ash";
        else if (ext == ".ash")
            partner = current.stem().string() + ".asc";
        if (!partner.empty())
            editor_.OpenScriptFile(partner);
    }

    // Ctrl+Shift+G — Open GlobalScript.asc
    if (kb_io.KeyCtrl && kb_io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_G))
    {
        editor_.OpenScriptFile("GlobalScript.asc");
    }

    // Ctrl+Shift+H — Open GlobalScript.ash (global header)
    if (kb_io.KeyCtrl && kb_io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_H))
    {
        editor_.OpenScriptFile("GlobalScript.ash");
    }

    // Ctrl+Shift+Q — Toggle line comment
    if (kb_io.KeyCtrl && kb_io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_Q))
    {
        ToggleLineComment();
    }

    // Shift+F12 — Find All Usages
    if (kb_io.KeyShift && !kb_io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_F12))
    {
        FindAllUsages();
    }

    // F9 — Toggle breakpoint on current line
    if (!kb_io.KeyCtrl && !kb_io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_F9))
    {
        auto cursor = text_editor_->GetCursorPosition();
        text_editor_->ToggleBreakpoint(cursor.mLine + 1); // 1-based
    }

    // Status bar info
    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();

    auto pos = text_editor_->GetCursorPosition();
    ImGui::TextDisabled("Ln %d, Col %d | %d lines | %s",
                       pos.mLine + 1, pos.mColumn + 1,
                       text_editor_->GetTotalLines(),
                       text_editor_->IsOverwrite() ? "OVR" : "INS");
    if (modified_)
    {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), " [Modified]");
    }
}

void ScriptEditor::DrawEditor()
{
    // Block editing while test game is running
    bool game_running = editor_.GetApp().IsGameRunning();
    if (game_running)
    {
        if (!text_editor_->IsReadOnly())
            text_editor_->SetReadOnly(true);
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f),
            "Script is read-only while a test game is running.");
    }
    else
    {
        if (text_editor_->IsReadOnly())
            text_editor_->SetReadOnly(false);
    }

    // Render the TextEditor
    ImVec2 avail = ImGui::GetContentRegionAvail();
    // Reserve space for find/replace bar and errors panel
    float reserve = 0;
    if (show_find_replace_) reserve += Dpi(60);
    if (show_errors_ && !compile_errors_.empty()) reserve += Dpi(120);
    avail.y -= reserve;
    if (avail.y < Dpi(100)) avail.y = Dpi(100);

    text_editor_->Render(filename_.c_str(), avail, true);

    // Right-click context menu on the editor area
    if (ImGui::BeginPopupContextItem("##ScriptEditorCtx"))
    {
        if (ImGui::MenuItem("Cut", "Ctrl+X", false, text_editor_->HasSelection()))
        {
            text_editor_->Cut();
        }
        if (ImGui::MenuItem("Copy", "Ctrl+C", false, text_editor_->HasSelection()))
        {
            text_editor_->Copy();
        }
        if (ImGui::MenuItem("Paste", "Ctrl+V"))
        {
            text_editor_->Paste();
        }
        if (ImGui::MenuItem("Select All", "Ctrl+A"))
        {
            text_editor_->SelectAll();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Toggle Line Comment", "Ctrl+Shift+Q"))
        {
            ToggleLineComment();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Find...", "Ctrl+F"))
        {
            show_find_replace_ = true;
        }
        if (ImGui::MenuItem("Find All Usages", "Shift+F12"))
        {
            FindAllUsages();
        }
        if (ImGui::MenuItem("Go to Line...", "Ctrl+G"))
        {
            show_goto_line_ = true;
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Compile Script", "F7"))
        {
            CompileCurrentScript();
        }
        ImGui::EndPopup();
    }

    if (text_editor_->IsTextChanged())
    {
        modified_ = true;
        function_list_dirty_ = true;

        // Trigger autocomplete on dot or after typing 3+ chars of a word
        auto pos = text_editor_->GetCursorPosition();
        auto lines = text_editor_->GetTextLines();
        if (pos.mLine < (int)lines.size())
        {
            const auto& line = lines[pos.mLine];
            if (pos.mColumn > 0 && pos.mColumn <= (int)line.size())
            {
                char last_char = line[pos.mColumn - 1];
                if (last_char == '.')
                {
                    // Get the word before the dot for context, then show autocomplete with empty prefix
                    autocomplete_prefix_ = "";
                    show_autocomplete_ = true;
                    autocomplete_selected_ = 0;
                }
                else if (isalnum(last_char) || last_char == '_')
                {
                    // Gather the current word being typed
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

    // Handle Escape to dismiss autocomplete
    if (show_autocomplete_ && ImGui::IsKeyPressed(ImGuiKey_Escape))
        show_autocomplete_ = false;

    // Draw autocomplete popup (if active)
    if (show_autocomplete_)
        DrawAutoCompletePopup();
}

void ScriptEditor::DrawFindReplaceBar()
{
    ImGui::Separator();
    ImGui::PushID("FindReplace");

    float width = ImGui::GetContentRegionAvail().x;

    // Find row
    ImGui::Text("Find:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(width * 0.4f);
    if (ImGui::InputText("##find", find_text_, sizeof(find_text_),
        ImGuiInputTextFlags_EnterReturnsTrue))
    {
        FindNext();
    }
    ImGui::SameLine();
    if (ImGui::Button("Next")) FindNext();
    ImGui::SameLine();
    if (ImGui::Button("Prev")) FindPrevious();
    ImGui::SameLine();
    ImGui::Checkbox("Case", &find_case_sensitive_);
    ImGui::SameLine();
    ImGui::Checkbox("Word", &find_whole_word_);

    // Replace row
    ImGui::Text("Repl:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(width * 0.4f);
    ImGui::InputText("##replace", replace_text_, sizeof(replace_text_));
    ImGui::SameLine();
    if (ImGui::Button("Replace")) ReplaceNext();
    ImGui::SameLine();
    if (ImGui::Button("All")) ReplaceAll();
    ImGui::SameLine();
    if (ImGui::Button("Close##findclose"))
        show_find_replace_ = false;

    ImGui::PopID();
}

void ScriptEditor::DrawGoToLineDialog()
{
    // Use a unique window title per script to avoid popup ID collisions
    std::string win_title = "Go To Line##" + filename_;
    if (ImGui::Begin(win_title.c_str(), &show_goto_line_,
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse))
    {
        int total = text_editor_->GetTotalLines();

        ImGui::Text("Line (1 - %d):", total);
        ImGui::SetNextItemWidth(Dpi(120));
        ImGui::InputInt("##gotoline", &goto_line_number_, 1, 10);
        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            if (goto_line_number_ < 1) goto_line_number_ = 1;
            if (goto_line_number_ > total) goto_line_number_ = total;
            text_editor_->SetCursorPosition(
                TextEditor::Coordinates(goto_line_number_ - 1, 0));
            show_goto_line_ = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Go"))
        {
            if (goto_line_number_ < 1) goto_line_number_ = 1;
            if (goto_line_number_ > total) goto_line_number_ = total;
            text_editor_->SetCursorPosition(
                TextEditor::Coordinates(goto_line_number_ - 1, 0));
            show_goto_line_ = false;
        }
    }
    ImGui::End();
}

void ScriptEditor::DrawAutoCompletePopup()
{
    auto* project = editor_.GetProject();
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
    std::string popup_id = "##autocomplete_" + filename_;
    if (ImGui::Begin(popup_id.c_str(), nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing))
    {
        for (int i = 0; i < (int)matches.size(); i++)
        {
            bool selected = (autocomplete_selected_ == i);
            if (ImGui::Selectable(matches[i]->name.c_str(), selected))
            {
                std::string remainder = matches[i]->name.substr(autocomplete_prefix_.size());
                text_editor_->InsertText(remainder);
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

void ScriptEditor::DrawCompileErrors()
{
    ImGui::Separator();

    ImGui::BeginChild("CompileErrors", ImVec2(0, Dpi(110)), ImGuiChildFlags_Borders);
    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Compile Errors (%d):",
                       (int)compile_errors_.size());
    ImGui::Separator();

    for (const auto& err : compile_errors_)
    {
        char buf[512];
        snprintf(buf, sizeof(buf), "Line %d: %s", err.line, err.message.c_str());

        if (ImGui::Selectable(buf))
        {
            text_editor_->SetCursorPosition(
                TextEditor::Coordinates(err.line - 1, 0));
        }
    }
    ImGui::EndChild();
}

void ScriptEditor::SetupAGSLanguage(TextEditor& te)
{
    static auto lang = CreateAGSLanguageDefinition();
    te.SetLanguageDefinition(lang);
}

void ScriptEditor::CompileCurrentScript()
{
    compile_errors_.clear();

    auto* project = editor_.GetProject();
    std::string script_text = text_editor_->GetText();

    // Save to disk if a project is loaded
    if (project && project->IsLoaded())
    {
        std::string script_path = (fs::path(project->GetProjectDir()) / filename_).string();
        std::ofstream file(script_path);
        if (file.is_open())
        {
            file << script_text;
            file.close();
        }
        else
        {
            editor_.GetLogPanel().AddLog("[Error] Failed to save script before compiling: %s",
                                        script_path.c_str());
        }
    }

    // For .ash header files, compile as a header (validate syntax)
    // For .asc script files, compile as a script
    bool is_header = fs::path(filename_).extension() == ".ash";

    CompilerBridge compiler;

    // Configure compiler with project settings (if a project is loaded)
    auto* gd = (project && project->IsLoaded()) ? project->GetGameData() : nullptr;
    if (gd)
    {
        ScriptCompileSettings scs;
        scs.script_api_version = gd->script_api_version;
        scs.script_compat_level = gd->script_compat_level;
        scs.enforce_object_scripting = gd->enforce_object_scripting;
        scs.enforce_new_strings = gd->enforce_new_strings;
        scs.left_to_right_precedence = gd->left_to_right_precedence;
        scs.enforce_new_audio = gd->enforce_new_audio;
        scs.use_old_custom_dialog_api = gd->use_old_custom_dialog_api;
        scs.use_old_keyboard_handling = gd->use_old_keyboard_handling;
        scs.debug_mode = gd->debug_mode;
        compiler.Configure(scs);
    }

    CompileResult result;
    if (is_header)
    {
        // Headers are compiled by treating them as a header for an empty script
        result = compiler.CompileScriptText("", filename_, script_text);
    }
    else
    {
        result = compiler.CompileScriptText(script_text, filename_, "");
    }

    if (result.success)
    {
        editor_.GetLogPanel().AddLog("[Build] Script '%s' compiled successfully.", filename_.c_str());
        show_errors_ = false;

        // Clear error markers
        TextEditor::ErrorMarkers markers;
        text_editor_->SetErrorMarkers(markers);
    }
    else
    {
        editor_.GetLogPanel().AddLog("[Build] Script '%s' compilation failed:", filename_.c_str());

        TextEditor::ErrorMarkers markers;
        for (const auto& err : result.errors)
        {
            compile_errors_.push_back({err.line, err.message});
            markers[err.line] = err.message;
            editor_.GetLogPanel().AddLog("[Build]   Line %d: %s", err.line, err.message.c_str());
        }
        text_editor_->SetErrorMarkers(markers);
        show_errors_ = true;
    }
}

void ScriptEditor::FindNext()
{
    if (strlen(find_text_) == 0)
        return;

    std::string text = text_editor_->GetText();
    std::string needle(find_text_);

    auto pos = text_editor_->GetCursorPosition();
    auto lines = text_editor_->GetTextLines();
    int offset = 0;
    for (int i = 0; i < pos.mLine && i < (int)lines.size(); i++)
        offset += (int)lines[i].size() + 1;
    offset += pos.mColumn;

    std::string search_text = text;
    std::string search_needle = needle;
    if (!find_case_sensitive_)
    {
        std::transform(search_text.begin(), search_text.end(), search_text.begin(), ::tolower);
        std::transform(search_needle.begin(), search_needle.end(), search_needle.begin(), ::tolower);
    }

    // Helper: check if match at position `p` is a whole-word match
    auto is_whole_word = [&](size_t p) -> bool {
        if (!find_whole_word_) return true;
        if (p > 0 && (std::isalnum(search_text[p - 1]) || search_text[p - 1] == '_'))
            return false;
        size_t end_pos = p + search_needle.size();
        if (end_pos < search_text.size() && (std::isalnum(search_text[end_pos]) || search_text[end_pos] == '_'))
            return false;
        return true;
    };

    size_t found = std::string::npos;
    size_t search_from = offset + 1;
    bool wrapped = false;
    while (true)
    {
        size_t pos_found = search_text.find(search_needle, search_from);
        if (pos_found == std::string::npos)
        {
            if (!wrapped) { wrapped = true; search_from = 0; continue; }
            break;
        }
        if (is_whole_word(pos_found)) { found = pos_found; break; }
        search_from = pos_found + 1;
    }

    if (found != std::string::npos)
    {
        int line = 0, col = 0;
        int cur = 0;
        for (int i = 0; i < (int)lines.size(); i++)
        {
            if (cur + (int)lines[i].size() + 1 > (int)found)
            {
                line = i;
                col = (int)found - cur;
                break;
            }
            cur += (int)lines[i].size() + 1;
        }

        text_editor_->SetCursorPosition(TextEditor::Coordinates(line, col));
        auto start = TextEditor::Coordinates(line, col);
        auto end = TextEditor::Coordinates(line, col + (int)needle.size());
        text_editor_->SetSelection(start, end);

        editor_.GetLogPanel().AddLog("[Find] Found '%s' at line %d, col %d",
                                     find_text_, line + 1, col + 1);
    }
    else
    {
        editor_.GetLogPanel().AddLog("[Find] '%s' not found.", find_text_);
    }
}

void ScriptEditor::FindPrevious()
{
    if (strlen(find_text_) == 0)
        return;

    std::string text = text_editor_->GetText();
    std::string needle(find_text_);

    auto pos = text_editor_->GetCursorPosition();
    auto lines = text_editor_->GetTextLines();
    int offset = 0;
    for (int i = 0; i < pos.mLine && i < (int)lines.size(); i++)
        offset += (int)lines[i].size() + 1;
    offset += pos.mColumn;

    std::string search_text = text;
    std::string search_needle = needle;
    if (!find_case_sensitive_)
    {
        std::transform(search_text.begin(), search_text.end(), search_text.begin(), ::tolower);
        std::transform(search_needle.begin(), search_needle.end(), search_needle.begin(), ::tolower);
    }

    // Helper: check if match at position `p` is a whole-word match
    auto is_whole_word = [&](size_t p) -> bool {
        if (!find_whole_word_) return true;
        if (p > 0 && (std::isalnum(search_text[p - 1]) || search_text[p - 1] == '_'))
            return false;
        size_t end_pos = p + search_needle.size();
        if (end_pos < search_text.size() && (std::isalnum(search_text[end_pos]) || search_text[end_pos] == '_'))
            return false;
        return true;
    };

    size_t found = std::string::npos;
    size_t search_from = (offset > 0) ? offset - 1 : std::string::npos;
    bool wrapped = false;
    while (true)
    {
        if (search_from == std::string::npos)
        {
            if (!wrapped) { wrapped = true; search_from = search_text.size(); continue; }
            break;
        }
        size_t pos_found = search_text.rfind(search_needle, search_from);
        if (pos_found == std::string::npos)
        {
            if (!wrapped) { wrapped = true; search_from = search_text.size(); continue; }
            break;
        }
        if (is_whole_word(pos_found)) { found = pos_found; break; }
        search_from = (pos_found > 0) ? pos_found - 1 : std::string::npos;
    }

    if (found != std::string::npos)
    {
        int line = 0, col = 0, cur = 0;
        for (int i = 0; i < (int)lines.size(); i++)
        {
            if (cur + (int)lines[i].size() + 1 > (int)found)
            {
                line = i;
                col = (int)found - cur;
                break;
            }
            cur += (int)lines[i].size() + 1;
        }
        text_editor_->SetCursorPosition(TextEditor::Coordinates(line, col));
        text_editor_->SetSelection(
            TextEditor::Coordinates(line, col),
            TextEditor::Coordinates(line, col + (int)needle.size()));
    }
    else
    {
        editor_.GetLogPanel().AddLog("[Find] '%s' not found.", find_text_);
    }
}

void ScriptEditor::ReplaceNext()
{
    if (strlen(find_text_) == 0)
        return;

    if (text_editor_->HasSelection())
    {
        std::string selected = text_editor_->GetSelectedText();
        std::string needle(find_text_);
        bool match = find_case_sensitive_
            ? (selected == needle)
            : (std::equal(selected.begin(), selected.end(), needle.begin(), needle.end(),
               [](char a, char b) { return tolower(a) == tolower(b); }));

        if (match)
        {
            text_editor_->InsertText(replace_text_);
            modified_ = true;
        }
    }

    FindNext();
}

void ScriptEditor::ReplaceAll()
{
    if (strlen(find_text_) == 0)
        return;

    std::string text = text_editor_->GetText();
    std::string needle(find_text_);
    std::string replacement(replace_text_);

    int count = 0;
    if (find_case_sensitive_)
    {
        size_t pos = 0;
        while ((pos = text.find(needle, pos)) != std::string::npos)
        {
            text.replace(pos, needle.length(), replacement);
            pos += replacement.length();
            count++;
        }
    }
    else
    {
        std::string lower_text = text;
        std::string lower_needle = needle;
        std::transform(lower_text.begin(), lower_text.end(), lower_text.begin(), ::tolower);
        std::transform(lower_needle.begin(), lower_needle.end(), lower_needle.begin(), ::tolower);

        size_t pos = 0;
        while ((pos = lower_text.find(lower_needle, pos)) != std::string::npos)
        {
            text.replace(pos, needle.length(), replacement);
            lower_text = text;
            std::transform(lower_text.begin(), lower_text.end(), lower_text.begin(), ::tolower);
            pos += replacement.length();
            count++;
        }
    }

    if (count > 0)
    {
        text_editor_->SetText(text);
        modified_ = true;
        editor_.GetLogPanel().AddLog("[Replace] Replaced %d occurrences.", count);
    }
    else
    {
        editor_.GetLogPanel().AddLog("[Replace] No occurrences found.");
    }
}

void ScriptEditor::RebuildFunctionList()
{
    function_list_.clear();
    function_list_dirty_ = false;

    auto lines = text_editor_->GetTextLines();
    // Match AGS script function definitions like:
    //   function game_start()
    //   int MyFunction(int param)
    //   static void SomeFunc()
    // But not declarations with "import" or lines inside comments
    std::regex func_re(
        R"(^\s*(?:static\s+)?(?:function|int|short|float|char|bool|void|String)\s+(\w+)\s*\()",
        std::regex::optimize);

    bool in_block_comment = false;
    for (int i = 0; i < (int)lines.size(); i++)
    {
        const auto& line = lines[i];

        // Track block comments
        if (in_block_comment)
        {
            if (line.find("*/") != std::string::npos)
                in_block_comment = false;
            continue;
        }
        if (line.find("/*") != std::string::npos)
        {
            if (line.find("*/") == std::string::npos)
                in_block_comment = true;
            continue;
        }

        // Skip single-line comments and import declarations
        size_t first = line.find_first_not_of(" \t");
        if (first == std::string::npos) continue;
        if (line.substr(first, 2) == "//") continue;
        if (line.find("import") != std::string::npos) continue;

        std::smatch m;
        if (std::regex_search(line, m, func_re))
        {
            FunctionEntry entry;
            entry.name = m[1].str() + "()";
            entry.line = i;
            function_list_.push_back(entry);
        }
    }
}

void ScriptEditor::ToggleLineComment()
{
    auto pos = text_editor_->GetCursorPosition();
    auto lines = text_editor_->GetTextLines();
    if (pos.mLine >= (int)lines.size())
        return;

    // Determine range: use selection if available, otherwise current line
    int start_line = pos.mLine;
    int end_line = pos.mLine;

    if (text_editor_->HasSelection())
    {
        // Determine selection range from selected text line count
        std::string sel_text = text_editor_->GetSelectedText();
        int sel_lines = 0;
        for (char c : sel_text) { if (c == '\n') sel_lines++; }
        // Selection extends sel_lines lines from cursor; use cursor as one end
        // Since cursor may be at start or end, expand range both ways
        start_line = std::max(0, pos.mLine - sel_lines);
        end_line = std::min((int)lines.size() - 1, pos.mLine + sel_lines);
        // Narrow: find the actual range that contains the selected text
        // Simple heuristic: if cursor is at end, selection starts above
        if (pos.mColumn == 0 && sel_lines > 0)
        {
            start_line = pos.mLine - sel_lines;
            end_line = pos.mLine > 0 ? pos.mLine - 1 : pos.mLine;
        }
        else
        {
            start_line = pos.mLine;
            end_line = pos.mLine + sel_lines;
        }
        start_line = std::max(0, start_line);
        end_line = std::min((int)lines.size() - 1, end_line);
    }

    // Check if all lines in range are already commented
    bool all_commented = true;
    for (int i = start_line; i <= end_line && i < (int)lines.size(); i++)
    {
        const std::string& line = lines[i];
        // Find first non-whitespace
        size_t first_char = line.find_first_not_of(" \t");
        if (first_char == std::string::npos)
            continue; // empty line, skip
        if (line.substr(first_char, 2) != "//")
        {
            all_commented = false;
            break;
        }
    }

    // Build new text for the whole document with toggled comments
    std::string new_text;
    for (int i = 0; i < (int)lines.size(); i++)
    {
        std::string line = lines[i];
        if (i >= start_line && i <= end_line)
        {
            size_t first_char = line.find_first_not_of(" \t");
            if (all_commented)
            {
                // Remove comment: find "//" and remove it (plus one trailing space if present)
                if (first_char != std::string::npos && line.substr(first_char, 2) == "//")
                {
                    int remove_len = 2;
                    if (first_char + 2 < line.size() && line[first_char + 2] == ' ')
                        remove_len = 3;
                    line.erase(first_char, remove_len);
                }
            }
            else
            {
                // Add comment at first non-whitespace position
                if (first_char == std::string::npos)
                    first_char = 0;
                line.insert(first_char, "// ");
            }
        }
        if (i > 0)
            new_text += '\n';
        new_text += line;
    }

    text_editor_->SetText(new_text);
    text_editor_->SetCursorPosition(pos);
    modified_ = true;
}

// -------------------------------------------------------------------------
// Find All Usages - search all project scripts for the word under cursor
// -------------------------------------------------------------------------
void ScriptEditor::FindAllUsages()
{
    std::string token = GetWordUnderCursor();
    if (token.empty())
        return;

    auto* project = editor_.GetProject();
    if (!project || !project->IsLoaded())
        return;

    GameData* gd = project->GetGameData();
    if (!gd)
        return;

    std::string project_dir = project->GetProjectDir();
    std::vector<FindResult> results;

    // Helper: search a single script text for whole-word matches of token
    auto search_text = [&](const std::string& filename,
                           const std::string& content)
    {
        std::istringstream stream(content);
        std::string line;
        int line_num = 0;
        while (std::getline(stream, line))
        {
            line_num++;
            size_t pos = 0;
            while ((pos = line.find(token, pos)) != std::string::npos)
            {
                // Whole-word check
                bool word_start = (pos == 0) ||
                    (!std::isalnum((unsigned char)line[pos - 1]) &&
                     line[pos - 1] != '_');
                size_t end_pos = pos + token.size();
                bool word_end = (end_pos >= line.size()) ||
                    (!std::isalnum((unsigned char)line[end_pos]) &&
                     line[end_pos] != '_');
                if (word_start && word_end)
                {
                    FindResult r;
                    r.filename = filename;
                    r.line = line_num;
                    r.line_text = line;
                    r.char_index = (int)pos;
                    results.push_back(r);
                }
                pos = end_pos;
            }
        }
    };

    // Helper: read a file and search it
    auto search_file = [&](const std::string& filename)
    {
        std::string filepath = (fs::path(project_dir) / filename).string();
        std::ifstream file(filepath);
        if (!file.is_open())
            return;
        std::ostringstream ss;
        ss << file.rdbuf();
        search_text(filename, ss.str());
    };

    // Search all script module headers and bodies
    for (const auto& mod : gd->script_modules)
    {
        if (!mod.header_file.empty())
            search_file(mod.header_file);
        if (!mod.script_file.empty())
            search_file(mod.script_file);
    }

    // Search room scripts (roomN.asc files)
    for (const auto& room : gd->rooms)
    {
        char room_script[64];
        snprintf(room_script, sizeof(room_script), "room%d.asc", room.number);
        std::string room_path = (fs::path(project_dir) / room_script).string();
        if (fs::exists(room_path))
            search_file(room_script);
    }

    // Search dialog scripts (in-memory)
    for (const auto& d : gd->dialogs)
    {
        if (!d.script.empty())
        {
            std::string dlg_name = "Dialog " + std::to_string(d.id);
            search_text(dlg_name, d.script);
        }
    }

    editor_.GetLogPanel().AddLog("[Find] '%s': %d result(s) across project scripts.",
        token.c_str(), (int)results.size());

    // Show results in FindResultsPane
    auto* pane = editor_.OpenOrFocusPane<FindResultsPane>(editor_);
    pane->SetResults(token, std::move(results));
    pane->SetNavigateCallback([this](const std::string& file, int line) {
        editor_.OpenScriptFile(file);
        // Navigate to line in the newly opened script
        // The script editor will be focused, we need to set cursor position
        for (auto& p : editor_.GetPanes())
        {
            auto* se = dynamic_cast<ScriptEditor*>(p.get());
            if (se && se->GetFilename() == file)
            {
                se->GoToLine(line);
                break;
            }
        }
    });
}

} // namespace AGSEditor
