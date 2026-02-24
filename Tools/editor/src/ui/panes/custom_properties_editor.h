// AGS Editor ImGui - Custom Properties Editor
// Defines custom property schemas that can be attached to game entities
// Operates directly on GameData (no copy-and-sync pattern).
#pragma once

#include "ui/editor_ui.h"
#include <string>

namespace AGSEditor
{

class CustomPropertiesEditor : public EditorPane
{
public:
    explicit CustomPropertiesEditor(EditorUI& editor);

    void Draw() override;
    const char* GetTitle() const override { return "Custom Properties"; }

private:
    void ImportSchema(const std::string& path);
    void ExportSchema(const std::string& path);
    EditorUI& editor_;
    int selected_ = -1;
};

} // namespace AGSEditor
