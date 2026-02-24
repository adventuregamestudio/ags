// AGS Editor ImGui - Translation Editor
// Manages game translation files (.tra / .trs)
#pragma once

#include "ui/editor_ui.h"
#include <vector>
#include <string>

namespace AGSEditor
{

class TranslationEditor : public EditorPane
{
public:
    explicit TranslationEditor(EditorUI& editor);

    void Draw() override;
    const char* GetTitle() const override { return "Translation Editor"; }

private:
    // Translation data structs (must be before method declarations that use them)
    struct TranslationEntry
    {
        std::string original;
        std::string translated;
        bool modified = false;
    };

    struct TranslationLanguage
    {
        std::string name;
        std::string filename;  // .trs file
        std::vector<TranslationEntry> entries;
        bool modified = false;
        int normal_font = -1;   // -1 = use default
        int speech_font = -1;   // -1 = use default
        int right_to_left = -1; // -1 = use default, 0 = LTR, 1 = RTL
        std::string encoding;   // "UTF-8" or "ASCII" or ""
    };

    void DrawLanguageList();
    void DrawTranslationTable();
    void DrawToolbar();

    // .trs file I/O
    bool LoadTrsFile(const std::string& path, TranslationLanguage& lang);
    bool SaveTrsFile(const std::string& path, const TranslationLanguage& lang);
    void SaveAllTrsFiles();

    // Update source strings from game data
    void UpdateFromGame();
    void CollectGameStrings(std::vector<std::string>& out);

    // Make selected language the default (replace game text with translations)
    void MakeDefaultLanguage();

    // Compile .trs -> .tra for build
    bool CompileTra(const TranslationLanguage& lang, const std::string& output_path);

    EditorUI& editor_;

    std::vector<TranslationLanguage> languages_;
    int selected_language_ = -1;
    char filter_text_[256] = "";
    bool show_untranslated_only_ = false;
    char new_language_name_[128] = "";
    bool show_rename_popup_ = false;
    char rename_buf_[128] = "";
};

} // namespace AGSEditor
