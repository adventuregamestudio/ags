#pragma once
// Reusable ImGui widgets for editing custom property values on game entities
// (characters, hotspots, objects, inventory items, rooms)

#include "project/game_data.h"
#include "core/dpi_helper.h"
#include "imgui.h"
#include <map>
#include <string>
#include <cstring>

namespace AGSEditor
{

// Renders custom property value editors for the given entity.
// filter_field: which "applies_to_*" field to check (e.g. "characters", "hotspots", etc.)
// Returns true if any value was changed.
inline bool DrawCustomPropertyValues(
    const std::vector<GameData::CustomPropertySchemaInfo>& schemas,
    std::map<std::string, std::string>& properties,
    const char* filter_field)
{
    bool changed = false;

    for (const auto& schema : schemas)
    {
        // Filter by applies-to
        bool applies = false;
        if (strcmp(filter_field, "characters") == 0) applies = schema.applies_to_characters;
        else if (strcmp(filter_field, "hotspots") == 0) applies = schema.applies_to_hotspots;
        else if (strcmp(filter_field, "objects") == 0) applies = schema.applies_to_objects;
        else if (strcmp(filter_field, "rooms") == 0) applies = schema.applies_to_rooms;
        else if (strcmp(filter_field, "inv_items") == 0) applies = schema.applies_to_inv_items;
        if (!applies) continue;

        ImGui::PushID(schema.name.c_str());

        // Get current value (or default)
        auto it = properties.find(schema.name);
        std::string current_val = (it != properties.end())
            ? it->second : schema.default_value;

        bool val_changed = false;
        if (schema.type == 0) // Boolean
        {
            bool bval = (current_val == "1" || current_val == "true" || current_val == "True");
            if (ImGui::Checkbox(schema.name.c_str(), &bval))
            {
                current_val = bval ? "1" : "0";
                val_changed = true;
            }
        }
        else if (schema.type == 1) // Integer
        {
            int ival = 0;
            try { ival = std::stoi(current_val); } catch (...) {}
            ImGui::SetNextItemWidth(Dpi(100));
            if (ImGui::InputInt(schema.name.c_str(), &ival))
            {
                current_val = std::to_string(ival);
                val_changed = true;
            }
        }
        else // String (type == 2)
        {
            char buf[512];
            strncpy(buf, current_val.c_str(), sizeof(buf) - 1);
            buf[sizeof(buf) - 1] = '\0';
            ImGui::SetNextItemWidth(Dpi(200));
            if (ImGui::InputText(schema.name.c_str(), buf, sizeof(buf)))
            {
                current_val = buf;
                val_changed = true;
            }
        }

        if (val_changed)
        {
            properties[schema.name] = current_val;
            changed = true;
        }

        if (!schema.description.empty() && ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", schema.description.c_str());

        ImGui::PopID();
    }

    return changed;
}

} // namespace AGSEditor
