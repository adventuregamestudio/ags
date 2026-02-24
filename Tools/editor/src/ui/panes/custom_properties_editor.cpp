// AGS Editor ImGui - Custom Properties Editor implementation
// Operates directly on GameData::custom_property_schemas (no copy-and-sync).
#include "custom_properties_editor.h"
#include "ui/editor_ui.h"
#include "ui/log_panel.h"
#include "ui/file_dialog.h"
#include "core/dpi_helper.h"
#include "project/project.h"
#include "project/game_data.h"
#include "app.h"

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"
#include "tinyxml2.h"

namespace AGSEditor
{

CustomPropertiesEditor::CustomPropertiesEditor(EditorUI& editor)
    : editor_(editor)
{
    // Seed example properties if project loaded but has no schemas
    auto* project = editor_.GetApp().GetProject();
    auto* gd = project ? project->GetGameData() : nullptr;
    if (gd && gd->custom_property_schemas.empty())
    {
        GameData::CustomPropertySchemaInfo p1;
        p1.name = "Usable";
        p1.description = "Whether this item can be used by the player";
        p1.type = 0;
        p1.default_value = "true";
        gd->custom_property_schemas.push_back(p1);

        GameData::CustomPropertySchemaInfo p2;
        p2.name = "Weight";
        p2.description = "Weight of the item in inventory units";
        p2.type = 1;
        p2.default_value = "1";
        gd->custom_property_schemas.push_back(p2);
    }
}

void CustomPropertiesEditor::Draw()
{
    auto* project = editor_.GetApp().GetProject();
    auto* gd = (project && project->IsLoaded()) ? project->GetGameData() : nullptr;
    if (!gd)
    {
        ImGui::TextDisabled("No project loaded.");
        return;
    }

    auto& schemas = gd->custom_property_schemas;

    // Toolbar
    if (ImGui::Button("Add Property"))
    {
        GameData::CustomPropertySchemaInfo p;
        p.name = "NewProperty" + std::to_string(schemas.size());
        p.type = 0;
        p.default_value = "false";
        schemas.push_back(p);
        selected_ = (int)schemas.size() - 1;
    }

    ImGui::SameLine();

    if (ImGui::Button("Remove") && selected_ >= 0 && selected_ < (int)schemas.size())
    {
        schemas.erase(schemas.begin() + selected_);
        if (selected_ >= (int)schemas.size())
            selected_ = (int)schemas.size() - 1;
    }

    ImGui::SameLine();
    if (ImGui::Button("Import..."))
    {
        FileDialog::Open(FileDialogType::OpenFile, "Import Property Schema",
            ".cpschema", ".", [this](const std::string& path) { ImportSchema(path); });
    }

    ImGui::SameLine();
    if (ImGui::Button("Export..."))
    {
        if (!schemas.empty())
        {
            FileDialog::Open(FileDialogType::SaveFile, "Export Property Schema",
                ".cpschema", ".", [this](const std::string& path) { ExportSchema(path); });
        }
    }

    ImGui::Separator();

    ImVec2 avail = ImGui::GetContentRegionAvail();
    float list_width = Dpi(220);

    // Left: property list
    ImGui::BeginChild("PropList", ImVec2(list_width, avail.y), ImGuiChildFlags_Borders);
    {
        ImGui::Text("Property Schemas");
        ImGui::Separator();

        int context_idx = -1;
        for (int i = 0; i < (int)schemas.size(); i++)
        {
            bool selected = (i == selected_);
            if (ImGui::Selectable(schemas[i].name.c_str(), selected))
                selected_ = i;
            if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
            {
                selected_ = i;
                context_idx = i;
            }
        }

        // Right-click on empty area
        if (context_idx < 0 && ImGui::IsMouseClicked(ImGuiMouseButton_Right)
            && ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows))
        {
            ImGui::OpenPopup("PropListCtx");
        }
        if (context_idx >= 0)
            ImGui::OpenPopup("PropListCtx");

        if (ImGui::BeginPopup("PropListCtx"))
        {
            if (ImGui::MenuItem("Add Property"))
            {
                GameData::CustomPropertySchemaInfo p;
                p.name = "NewProperty" + std::to_string(schemas.size());
                p.type = 0;
                p.default_value = "false";
                schemas.push_back(p);
                selected_ = (int)schemas.size() - 1;
            }

            bool has_sel = selected_ >= 0 && selected_ < (int)schemas.size();

            if (ImGui::MenuItem("Duplicate", nullptr, false, has_sel))
            {
                auto dup = schemas[selected_];
                dup.name += "_Copy";
                schemas.insert(schemas.begin() + selected_ + 1, dup);
                selected_ = selected_ + 1;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Move Up", nullptr, false, has_sel && selected_ > 0))
            {
                std::swap(schemas[selected_], schemas[selected_ - 1]);
                selected_--;
            }
            if (ImGui::MenuItem("Move Down", nullptr, false,
                has_sel && selected_ < (int)schemas.size() - 1))
            {
                std::swap(schemas[selected_], schemas[selected_ + 1]);
                selected_++;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Delete", nullptr, false, has_sel))
            {
                schemas.erase(schemas.begin() + selected_);
                if (selected_ >= (int)schemas.size())
                    selected_ = (int)schemas.size() - 1;
            }
            ImGui::EndPopup();
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // Right: property details
    ImGui::BeginChild("PropDetails", ImVec2(0, avail.y), ImGuiChildFlags_Borders);
    {
        if (selected_ >= 0 && selected_ < (int)schemas.size())
        {
            auto& prop = schemas[selected_];

            ImGui::Text("Property Details");
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::InputText("Name", &prop.name);
            ImGui::InputTextMultiline("Description", &prop.description,
                ImVec2(-1, Dpi(60)));

            ImGui::Combo("Type", &prop.type, "Boolean\0Integer\0Text\0");
            ImGui::InputText("Default Value", &prop.default_value);

            ImGui::Spacing();
            ImGui::SeparatorText("Applies To");
            ImGui::Checkbox("Characters", &prop.applies_to_characters);
            ImGui::Checkbox("Hotspots", &prop.applies_to_hotspots);
            ImGui::Checkbox("Objects", &prop.applies_to_objects);
            ImGui::Checkbox("Rooms", &prop.applies_to_rooms);
            ImGui::Checkbox("Inventory Items", &prop.applies_to_inv_items);

            ImGui::Spacing();
            ImGui::SeparatorText("Translation");
            ImGui::Checkbox("Translated", &prop.translated);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("When enabled, text values for this property\nwill be extracted for translation.");
        }
        else
        {
            ImGui::TextDisabled("Select a property to edit its details.");
        }
    }
    ImGui::EndChild();
}

static void XmlSetChildText(tinyxml2::XMLDocument& doc,
    tinyxml2::XMLElement* parent, const char* name, const char* value)
{
    tinyxml2::XMLElement* el = doc.NewElement(name);
    el->SetText(value);
    parent->InsertEndChild(el);
}

static void XmlSetChildBool(tinyxml2::XMLDocument& doc,
    tinyxml2::XMLElement* parent, const char* name, bool value)
{
    XmlSetChildText(doc, parent, name, value ? "True" : "False");
}

static const char* XmlChildText(const tinyxml2::XMLElement* parent,
    const char* name, const char* def = "")
{
    const tinyxml2::XMLElement* child = parent->FirstChildElement(name);
    return (child && child->GetText()) ? child->GetText() : def;
}

static int XmlChildInt(const tinyxml2::XMLElement* parent,
    const char* name, int def = 0)
{
    const tinyxml2::XMLElement* child = parent->FirstChildElement(name);
    if (child && child->GetText()) return atoi(child->GetText());
    return def;
}

static bool XmlChildBool(const tinyxml2::XMLElement* parent,
    const char* name, bool def = false)
{
    const char* text = XmlChildText(parent, name, nullptr);
    if (!text) return def;
    return (strcmp(text, "True") == 0 || strcmp(text, "true") == 0
        || strcmp(text, "1") == 0);
}

void CustomPropertiesEditor::ExportSchema(const std::string& path)
{
    auto* project = editor_.GetApp().GetProject();
    auto* gd = (project && project->IsLoaded()) ? project->GetGameData() : nullptr;
    if (!gd) return;

    auto& schemas = gd->custom_property_schemas;

    tinyxml2::XMLDocument doc;
    doc.InsertFirstChild(doc.NewDeclaration("xml version=\"1.0\" encoding=\"utf-8\""));
    tinyxml2::XMLElement* root = doc.NewElement("CustomPropertySchemaFile");
    root->SetAttribute("VersionIndex", 3060308);
    doc.InsertEndChild(root);

    tinyxml2::XMLElement* list = doc.NewElement("PropertyDefinitions");
    root->InsertEndChild(list);

    // Map type int to C# enum name for compatibility
    static const char* type_names[] = { "Boolean", "Number", "Text" };

    for (const auto& s : schemas)
    {
        tinyxml2::XMLElement* item = doc.NewElement("PropertyDefinition");
        XmlSetChildText(doc, item, "Name", s.name.c_str());
        XmlSetChildText(doc, item, "Description", s.description.c_str());
        int ti = s.type;
        XmlSetChildText(doc, item, "Type",
            (ti >= 0 && ti < 3) ? type_names[ti] : "Boolean");
        XmlSetChildText(doc, item, "DefaultValue", s.default_value.c_str());
        XmlSetChildBool(doc, item, "AppliesToCharacters", s.applies_to_characters);
        XmlSetChildBool(doc, item, "AppliesToHotspots", s.applies_to_hotspots);
        XmlSetChildBool(doc, item, "AppliesToObjects", s.applies_to_objects);
        XmlSetChildBool(doc, item, "AppliesToRooms", s.applies_to_rooms);
        XmlSetChildBool(doc, item, "AppliesToInvItems", s.applies_to_inv_items);
        XmlSetChildBool(doc, item, "Translated", s.translated);
        list->InsertEndChild(item);
    }

    if (doc.SaveFile(path.c_str()) == tinyxml2::XML_SUCCESS)
        editor_.GetLogPanel().AddLog("[Properties] Exported %d schema(s) to %s",
            (int)schemas.size(), path.c_str());
    else
        editor_.GetLogPanel().AddLog("[Properties] Failed to export to %s", path.c_str());
}

void CustomPropertiesEditor::ImportSchema(const std::string& path)
{
    auto* project = editor_.GetApp().GetProject();
    auto* gd = (project && project->IsLoaded()) ? project->GetGameData() : nullptr;
    if (!gd) return;

    auto& schemas = gd->custom_property_schemas;

    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(path.c_str()) != tinyxml2::XML_SUCCESS)
    {
        editor_.GetLogPanel().AddLog("[Properties] Failed to load %s", path.c_str());
        return;
    }

    const tinyxml2::XMLElement* root = doc.FirstChildElement("CustomPropertySchemaFile");
    if (!root)
    {
        editor_.GetLogPanel().AddLog("[Properties] Invalid schema file: missing root element");
        return;
    }

    const tinyxml2::XMLElement* list = root->FirstChildElement("PropertyDefinitions");
    if (!list)
    {
        editor_.GetLogPanel().AddLog("[Properties] Invalid schema file: missing PropertyDefinitions");
        return;
    }

    int count = 0;
    for (const tinyxml2::XMLElement* item = list->FirstChildElement("PropertyDefinition");
         item; item = item->NextSiblingElement("PropertyDefinition"))
    {
        GameData::CustomPropertySchemaInfo p;
        p.name = XmlChildText(item, "Name");
        p.description = XmlChildText(item, "Description");
        p.default_value = XmlChildText(item, "DefaultValue");

        // Parse type from string or int
        const char* type_str = XmlChildText(item, "Type", "Boolean");
        if (strcmp(type_str, "Boolean") == 0) p.type = 0;
        else if (strcmp(type_str, "Number") == 0) p.type = 1;
        else if (strcmp(type_str, "Text") == 0) p.type = 2;
        else p.type = XmlChildInt(item, "Type", 0);

        p.applies_to_characters = XmlChildBool(item, "AppliesToCharacters", true);
        p.applies_to_hotspots = XmlChildBool(item, "AppliesToHotspots", true);
        p.applies_to_objects = XmlChildBool(item, "AppliesToObjects", true);
        p.applies_to_rooms = XmlChildBool(item, "AppliesToRooms", true);
        p.applies_to_inv_items = XmlChildBool(item, "AppliesToInvItems", true);
        p.translated = XmlChildBool(item, "Translated", false);

        schemas.push_back(p);
        count++;
    }

    if (count > 0)
        selected_ = (int)schemas.size() - count; // select first imported

    editor_.GetLogPanel().AddLog("[Properties] Imported %d schema(s) from %s",
        count, path.c_str());
}

} // namespace AGSEditor
