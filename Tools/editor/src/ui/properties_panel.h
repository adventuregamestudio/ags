// AGS Editor ImGui - Properties Panel
#pragma once

#include <string>
#include <vector>
#include <variant>

namespace AGSEditor
{

class EditorUI;

// Simple property representation
struct PropertyItem
{
    std::string name;
    std::string category;
    std::variant<std::string, int, float, bool> value;
};

class PropertiesPanel
{
public:
    explicit PropertiesPanel(EditorUI& editor);

    void Draw();

    // Set properties to display
    void SetProperties(const std::string& title, const std::vector<PropertyItem>& props);
    void ClearProperties();

private:
    EditorUI& editor_;
    std::string title_;
    std::vector<PropertyItem> properties_;
};

} // namespace AGSEditor
