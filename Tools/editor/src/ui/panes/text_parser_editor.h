// AGS Editor ImGui - Text Parser Editor
// Manages the AGS text parser vocabulary and synonyms
// Operates directly on GameData (no copy-and-sync pattern).
#pragma once

#include "ui/editor_ui.h"
#include <string>

namespace AGSEditor
{

class TextParserEditor : public EditorPane
{
public:
    explicit TextParserEditor(EditorUI& editor);

    void Draw() override;
    const char* GetTitle() const override { return "Text Parser"; }

private:
    EditorUI& editor_;
    int selected_group_ = -1;
    std::string new_word_;
    std::string search_filter_;
    bool sort_by_group_id_ = true;
};

} // namespace AGSEditor
