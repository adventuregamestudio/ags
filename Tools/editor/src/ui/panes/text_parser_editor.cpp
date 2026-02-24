// AGS Editor ImGui - Text Parser Editor implementation
// Operates directly on GameData::text_parser_groups (no copy-and-sync).
#include "text_parser_editor.h"
#include "ui/editor_ui.h"
#include "ui/log_panel.h"
#include "core/dpi_helper.h"
#include "project/project.h"
#include "project/game_data.h"
#include "app.h"

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

#include <algorithm>
#include <cstring>

namespace AGSEditor
{

// Reserved word group IDs (from AGS engine wordsdictionary.h)
static const int WORD_GROUP_IGNORE = 0;
static const int WORD_GROUP_ANYWORD = 29999;
static const int WORD_GROUP_REST_OF_LINE = 30000;

static const char* GetWordTypeName(int group_id)
{
    if (group_id == WORD_GROUP_IGNORE)       return "Ignore";
    if (group_id == WORD_GROUP_ANYWORD)      return "Match Any Word";
    if (group_id == WORD_GROUP_REST_OF_LINE) return "Match Rest of Input";
    return "Normal";
}

static bool IsReservedGroup(int group_id)
{
    return group_id == WORD_GROUP_IGNORE ||
           group_id == WORD_GROUP_ANYWORD ||
           group_id == WORD_GROUP_REST_OF_LINE;
}

// Case-insensitive substring search
static bool ContainsCaseInsensitive(const std::string& haystack, const std::string& needle)
{
    if (needle.empty()) return true;
    auto it = std::search(haystack.begin(), haystack.end(),
        needle.begin(), needle.end(),
        [](char a, char b) { return std::tolower(a) == std::tolower(b); });
    return it != haystack.end();
}

TextParserEditor::TextParserEditor(EditorUI& editor)
    : editor_(editor)
{
    // Seed example word groups if project loaded but has no groups
    auto* project = editor_.GetApp().GetProject();
    auto* gd = project ? project->GetGameData() : nullptr;
    if (gd && gd->text_parser_groups.empty())
    {
        GameData::TextParserWordGroup g1;
        g1.id = 1;  g1.name = "look";
        g1.words.push_back({1, "look"});
        g1.words.push_back({1, "examine"});
        g1.words.push_back({1, "inspect"});
        gd->text_parser_groups.push_back(g1);

        GameData::TextParserWordGroup g2;
        g2.id = 2;  g2.name = "take";
        g2.words.push_back({2, "take"});
        g2.words.push_back({2, "get"});
        g2.words.push_back({2, "pick up"});
        g2.words.push_back({2, "grab"});
        gd->text_parser_groups.push_back(g2);

        GameData::TextParserWordGroup g3;
        g3.id = 3;  g3.name = "use";
        g3.words.push_back({3, "use"});
        g3.words.push_back({3, "apply"});
        gd->text_parser_groups.push_back(g3);
    }
}

void TextParserEditor::Draw()
{
    auto* project = editor_.GetApp().GetProject();
    auto* gd = (project && project->IsLoaded()) ? project->GetGameData() : nullptr;
    if (!gd)
    {
        ImGui::TextDisabled("No project loaded.");
        return;
    }

    auto& groups = gd->text_parser_groups;

    // Toolbar
    if (ImGui::Button("Add Word Group"))
    {
        GameData::TextParserWordGroup g;
        g.id = groups.empty() ? 1 : groups.back().id + 1;
        g.name = "group" + std::to_string(g.id);
        groups.push_back(g);
        selected_group_ = (int)groups.size() - 1;
    }

    ImGui::SameLine();

    if (ImGui::Button("Remove Group") && selected_group_ >= 0 &&
        selected_group_ < (int)groups.size())
    {
        int gid = groups[selected_group_].id;
        if (IsReservedGroup(gid))
        {
            editor_.GetLogPanel().AddLog(
                "[Warning] Cannot delete reserved word group '%s' (ID %d).",
                groups[selected_group_].name.c_str(), gid);
        }
        else
        {
            groups.erase(groups.begin() + selected_group_);
            if (selected_group_ >= (int)groups.size())
                selected_group_ = (int)groups.size() - 1;
        }
    }

    ImGui::SameLine();
    ImGui::Checkbox("Sort by ID", &sort_by_group_id_);

    ImGui::SameLine();
    ImGui::SetNextItemWidth(Dpi(200));
    ImGui::InputTextWithHint("##search", "Find word...", &search_filter_);

    ImGui::Separator();

    // Build sorted index for display
    std::vector<int> group_order;
    for (int i = 0; i < (int)groups.size(); i++)
        group_order.push_back(i);
    if (sort_by_group_id_)
    {
        std::sort(group_order.begin(), group_order.end(),
            [&groups](int a, int b) { return groups[a].id < groups[b].id; });
    }

    // If search filter is active, find matching groups
    bool has_filter = !search_filter_.empty();

    ImVec2 avail = ImGui::GetContentRegionAvail();
    float list_width = Dpi(200);

    // Left: word groups
    ImGui::BeginChild("WordGroups", ImVec2(list_width, avail.y), ImGuiChildFlags_Borders);
    {
        ImGui::Text("Word Groups");
        ImGui::Separator();

        for (int idx : group_order)
        {
            auto& grp = groups[idx];

            // If filter active, check if group name or any word matches
            if (has_filter)
            {
                bool match = ContainsCaseInsensitive(grp.name, search_filter_);
                if (!match)
                {
                    for (auto& w : grp.words)
                    {
                        if (ContainsCaseInsensitive(w.word, search_filter_))
                        {
                            match = true;
                            break;
                        }
                    }
                }
                if (!match) continue;
            }

            bool selected = (idx == selected_group_);
            char label[256];
            const char* type_str = GetWordTypeName(grp.id);
            snprintf(label, sizeof(label), "[%d] %s (%d words) [%s]",
                grp.id, grp.name.c_str(), (int)grp.words.size(), type_str);

            if (ImGui::Selectable(label, selected))
                selected_group_ = idx;
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // Right: words in group
    ImGui::BeginChild("GroupWords", ImVec2(0, avail.y), ImGuiChildFlags_Borders);
    {
        if (selected_group_ >= 0 && selected_group_ < (int)groups.size())
        {
            auto& group = groups[selected_group_];

            ImGui::Text("Group: %s (ID: %d)", group.name.c_str(), group.id);
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f),
                "[%s]", GetWordTypeName(group.id));
            ImGui::Separator();

            if (IsReservedGroup(group.id))
                ImGui::BeginDisabled();
            ImGui::InputText("Group Name", &group.name);
            if (IsReservedGroup(group.id))
                ImGui::EndDisabled();
            ImGui::Spacing();

            // Add word
            ImGui::SetNextItemWidth(Dpi(200));
            ImGui::InputText("##newword", &new_word_);
            ImGui::SameLine();
            if (ImGui::Button("Add Word") && !new_word_.empty())
            {
                GameData::TextParserWord w;
                w.word = new_word_;
                w.word_group = group.id;
                group.words.push_back(w);
                new_word_.clear();
            }

            ImGui::Spacing();

            // Word list
            if (ImGui::BeginTable("WordsTable", 3,
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                ImGuiTableFlags_ScrollY))
            {
                ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, Dpi(40));
                ImGui::TableSetupColumn("Word / Synonym", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, Dpi(60));
                ImGui::TableHeadersRow();

                int to_remove = -1;
                for (int i = 0; i < (int)group.words.size(); i++)
                {
                    ImGui::TableNextRow();
                    ImGui::PushID(i);

                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%d", i + 1);

                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(-1);
                    ImGui::InputText("##w", &group.words[i].word);

                    ImGui::TableSetColumnIndex(2);
                    if (ImGui::SmallButton("X"))
                        to_remove = i;

                    ImGui::PopID();
                }
                ImGui::EndTable();

                if (to_remove >= 0)
                    group.words.erase(group.words.begin() + to_remove);
            }

            ImGui::Spacing();
            ImGui::TextWrapped("Words in the same group are treated as synonyms. "
                "When the player types any word from a group, AGS considers it a match "
                "for that group's ID in Parser.Said() calls.");
        }
        else
        {
            ImGui::TextDisabled("Select a word group to edit.");
        }
    }
    ImGui::EndChild();
}

} // namespace AGSEditor
