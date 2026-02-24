// AGS Editor ImGui - Find All Usages Results Pane
// Shows search results in a table with clickable navigation to source.

#include "find_results_pane.h"
#include "core/dpi_helper.h"
#include "imgui.h"

#include <cstring>

namespace AGSEditor
{

FindResultsPane::FindResultsPane(EditorUI& editor)
    : editor_(editor)
    , title_("Find Results")
{
}

void FindResultsPane::SetResults(const std::string& token,
                                  std::vector<FindResult>&& results)
{
    search_token_ = token;
    results_ = std::move(results);
    title_ = "Find Results: " + token;
}

void FindResultsPane::Draw()
{
    // Summary line
    ImGui::Text("Search results for '%s': %d match(es)",
                search_token_.c_str(), (int)results_.size());
    ImGui::Separator();

    if (results_.empty())
    {
        ImGui::TextDisabled("No usages found.");
        return;
    }

    // Results table
    if (ImGui::BeginTable("FindResults", 3,
        ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg |
        ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupColumn("File", ImGuiTableColumnFlags_WidthFixed, Dpi(200));
        ImGui::TableSetupColumn("Line", ImGuiTableColumnFlags_WidthFixed, Dpi(50));
        ImGui::TableSetupColumn("Text", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        for (size_t i = 0; i < results_.size(); i++)
        {
            const auto& r = results_[i];
            ImGui::TableNextRow();

            // File column (clickable)
            ImGui::TableSetColumnIndex(0);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.7f, 1.0f, 1.0f));
            char label[512];
            snprintf(label, sizeof(label), "%s##%d", r.filename.c_str(), (int)i);
            if (ImGui::Selectable(label, false,
                ImGuiSelectableFlags_SpanAllColumns))
            {
                if (navigate_cb_)
                    navigate_cb_(r.filename, r.line);
            }
            ImGui::PopStyleColor();

            // Line column
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%d", r.line);

            // Text column - show trimmed line text
            ImGui::TableSetColumnIndex(2);
            // Trim leading whitespace for display
            const char* text = r.line_text.c_str();
            while (*text == ' ' || *text == '\t')
                ++text;
            ImGui::TextUnformatted(text);
        }

        ImGui::EndTable();
    }
}

} // namespace AGSEditor
