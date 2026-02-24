// AGS Editor ImGui - Properties Panel implementation
#include "properties_panel.h"
#include "editor_ui.h"
#include "imgui.h"

namespace AGSEditor
{

PropertiesPanel::PropertiesPanel(EditorUI& editor)
    : editor_(editor)
{
    // Default placeholder properties
    title_ = "Game Settings";
    properties_ = {
        {"Game Title", "General", std::string("My Adventure Game")},
        {"Resolution Width", "General", 320},
        {"Resolution Height", "General", 200},
        {"Color Depth", "General", 32},
        {"Debug Mode", "Compiler", true},
        {"Anti-Alias Fonts", "Visual", true},
        {"Target FPS", "Performance", 40},
    };
}

void PropertiesPanel::Draw()
{
    ImGui::Begin("Properties", nullptr,
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse);

    if (!title_.empty())
    {
        ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "%s", title_.c_str());
        ImGui::Separator();
    }

    if (properties_.empty())
    {
        ImGui::TextDisabled("No properties to display.");
        ImGui::TextDisabled("Select an item from the Project tree.");
    }
    else
    {
        // Group by category
        std::string last_category;
        for (auto& prop : properties_)
        {
            if (prop.category != last_category)
            {
                if (!last_category.empty())
                    ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.6f, 0.8f, 0.6f, 1.0f), "%s", prop.category.c_str());
                ImGui::Separator();
                last_category = prop.category;
            }

            ImGui::PushID(prop.name.c_str());

            if (std::holds_alternative<std::string>(prop.value))
            {
                auto& val = std::get<std::string>(prop.value);
                char buf[256];
                snprintf(buf, sizeof(buf), "%s", val.c_str());
                if (ImGui::InputText(prop.name.c_str(), buf, sizeof(buf)))
                    val = buf;
            }
            else if (std::holds_alternative<int>(prop.value))
            {
                auto& val = std::get<int>(prop.value);
                ImGui::InputInt(prop.name.c_str(), &val);
            }
            else if (std::holds_alternative<float>(prop.value))
            {
                auto& val = std::get<float>(prop.value);
                ImGui::InputFloat(prop.name.c_str(), &val);
            }
            else if (std::holds_alternative<bool>(prop.value))
            {
                auto& val = std::get<bool>(prop.value);
                ImGui::Checkbox(prop.name.c_str(), &val);
            }

            ImGui::PopID();
        }
    }

    ImGui::End();
}

void PropertiesPanel::SetProperties(const std::string& title, const std::vector<PropertyItem>& props)
{
    title_ = title;
    properties_ = props;
}

void PropertiesPanel::ClearProperties()
{
    title_.clear();
    properties_.clear();
}

} // namespace AGSEditor
