// AGS Editor ImGui - Events Widget
// Reusable widget for displaying and editing interaction event handlers
#pragma once

#include "project/game_data.h"
#include "imgui.h"
#include "core/dpi_helper.h"
#include <string>
#include <functional>

namespace AGSEditor
{

class EditorUI;

// Callback type for when an event handler needs to be created or navigated to
// Parameters: script_module, function_name
using EventActionCallback = std::function<void(const std::string& script_module,
                                                const std::string& function_name)>;

// Draw an events section with a table of event slots
// Returns true if any event was modified
// If force_focus is true, forces the header open and scrolls to it
inline bool DrawEventsSection(
    Interactions& interactions,
    const std::vector<InteractionEvent>& schema,
    const std::string& object_script_name,
    const std::string& default_script_module,
    EventActionCallback on_create_handler,
    EventActionCallback on_navigate_handler,
    bool force_focus = false)
{
    bool modified = false;

    if (force_focus)
    {
        ImGui::SetNextItemOpen(true);
        ImGui::SetScrollHereY(0.0f);
    }

    if (!ImGui::CollapsingHeader("Events"))
        return false;

    // Ensure handler array is sized to schema
    if ((int)interactions.handler_functions.size() < (int)schema.size())
        interactions.handler_functions.resize(schema.size());

    if (interactions.script_module.empty())
        interactions.script_module = default_script_module;

    if (ImGui::BeginTable("##EventsTable", 3,
        ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH))
    {
        ImGui::TableSetupColumn("Event", ImGuiTableColumnFlags_WidthFixed, Dpi(160));
        ImGui::TableSetupColumn("Handler", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("##Action", ImGuiTableColumnFlags_WidthFixed, Dpi(24));
        ImGui::TableHeadersRow();

        for (int i = 0; i < (int)schema.size(); ++i)
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            // Event name
            ImGui::TextUnformatted(schema[i].display_name);
            if (ImGui::IsItemHovered() && schema[i].params[0] != '\0')
            {
                ImGui::BeginTooltip();
                // Build default function name for tooltip
                std::string default_fn = object_script_name + "_" + schema[i].suffix;
                ImGui::Text("function %s(%s)", default_fn.c_str(), schema[i].params);
                ImGui::EndTooltip();
            }

            ImGui::TableNextColumn();

            // Handler function name (editable or placeholder)
            std::string& handler = interactions.handler_functions[i];
            if (handler.empty())
            {
                // Show placeholder with suggested name
                std::string suggested = object_script_name + "_" + schema[i].suffix;
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
                ImGui::TextUnformatted(suggested.c_str());
                ImGui::PopStyleColor();

                // Double-click to create
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
                {
                    handler = suggested;
                    modified = true;
                    if (on_create_handler)
                        on_create_handler(interactions.script_module, handler);
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::Text("Double-click to create handler");
                    ImGui::EndTooltip();
                }
            }
            else
            {
                // Show existing handler - clickable to navigate
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));
                ImGui::TextUnformatted(handler.c_str());
                ImGui::PopStyleColor();

                if (ImGui::IsItemClicked())
                {
                    if (on_navigate_handler)
                        on_navigate_handler(interactions.script_module, handler);
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::Text("Click to navigate to handler");
                    ImGui::EndTooltip();
                }
            }

            ImGui::TableNextColumn();

            // Action button: create or clear
            ImGui::PushID(i);
            if (handler.empty())
            {
                if (ImGui::SmallButton("+"))
                {
                    handler = object_script_name + "_" + schema[i].suffix;
                    modified = true;
                    if (on_create_handler)
                        on_create_handler(interactions.script_module, handler);
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::Text("Create event handler");
                    ImGui::EndTooltip();
                }
            }
            else
            {
                if (ImGui::SmallButton("x"))
                {
                    handler.clear();
                    modified = true;
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::Text("Remove handler binding");
                    ImGui::EndTooltip();
                }
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    return modified;
}

} // namespace AGSEditor
