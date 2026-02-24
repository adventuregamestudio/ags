// AGS Editor ImGui - Lip Sync Editor implementation
// Operates directly on GameData fields (no copy-and-sync).
// Matches the C# editor's CharactersPerFrame[20] data model:
//   Each of 20 frames has a slash-separated list of characters
//   that map to that frame during text-based lip sync.
#include "lip_sync_editor.h"
#include "ui/editor_ui.h"
#include "core/dpi_helper.h"
#include "project/project.h"
#include "project/game_data.h"
#include "app.h"

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

#include <cstring>

namespace AGSEditor
{

LipSyncEditor::LipSyncEditor(EditorUI& editor)
    : editor_(editor)
{
}

void LipSyncEditor::ResetToDefaults()
{
    auto* project = editor_.GetApp().GetProject();
    auto* gd = (project && project->IsLoaded()) ? project->GetGameData() : nullptr;
    if (!gd) return;

    // Default mapping matching the C# LipSync constructor
    static const char* kDefaults[GameData::kMaxLipSyncFrames] = {
        "A/I",                          // 0
        "E",                            // 1
        "O",                            // 2
        "U",                            // 3
        "M/B/P",                        // 4
        "C/D/G/K/N/R/S/Th/Y/Z",        // 5
        "L",                            // 6
        "F/V",                          // 7
        "W/Q",                          // 8
        "", "", "", "", "", "", "", "", "", "", ""  // 9-19
    };

    gd->default_lipsync_frame = 0;
    for (int i = 0; i < GameData::kMaxLipSyncFrames; i++)
        gd->lip_sync_chars_per_frame[i] = kDefaults[i];
}

void LipSyncEditor::Draw()
{
    auto* project = editor_.GetApp().GetProject();
    auto* gd = (project && project->IsLoaded()) ? project->GetGameData() : nullptr;
    if (!gd)
    {
        ImGui::TextDisabled("No project loaded.");
        return;
    }

    // -- Type selector --
    const char* type_items[] = { "Disabled", "Text (automatic)", "Voice (pamela sync files)" };
    ImGui::SetNextItemWidth(Dpi(250));
    ImGui::Combo("Lip Sync Type", &gd->lip_sync_type, type_items, 3);

    ImGui::Spacing();

    if (gd->lip_sync_type == 0)
    {
        ImGui::TextDisabled("Lip sync is disabled. Select a type above to enable it.");
        return;
    }

    // -- Default Frame --
    ImGui::SetNextItemWidth(Dpi(100));
    if (ImGui::InputInt("Default Frame", &gd->default_lipsync_frame, 1, 1))
    {
        if (gd->default_lipsync_frame < 0) gd->default_lipsync_frame = 0;
        if (gd->default_lipsync_frame >= GameData::kMaxLipSyncFrames)
            gd->default_lipsync_frame = GameData::kMaxLipSyncFrames - 1;
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // -- Instructions --
    ImGui::TextWrapped(
        "Type the letters that will cause each frame to be shown. "
        "Separate multiple letters with slashes, e.g. C/D/G/K");

    ImGui::Spacing();

    if (ImGui::Button("Reset to Defaults"))
        ResetToDefaults();

    ImGui::Spacing();

    // -- Frame text inputs: 2 columns of 10 rows (matching C# layout) --
    const int kRowsPerColumn = 10;
    float label_w = Dpi(60);
    float input_w = Dpi(200);

    if (ImGui::BeginTable("LipSyncFrames", 4, ImGuiTableFlags_None))
    {
        ImGui::TableSetupColumn("Label0", ImGuiTableColumnFlags_WidthFixed, label_w);
        ImGui::TableSetupColumn("Input0", ImGuiTableColumnFlags_WidthFixed, input_w);
        ImGui::TableSetupColumn("Label1", ImGuiTableColumnFlags_WidthFixed, label_w);
        ImGui::TableSetupColumn("Input1", ImGuiTableColumnFlags_WidthFixed, input_w);

        for (int row = 0; row < kRowsPerColumn; row++)
        {
            ImGui::TableNextRow();

            // Left column: frame [row]
            int idx_left = row;
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Frame %d:", idx_left);

            ImGui::TableSetColumnIndex(1);
            ImGui::PushID(idx_left);
            ImGui::SetNextItemWidth(-1);
            ImGui::InputText("##cpf", &gd->lip_sync_chars_per_frame[idx_left]);
            ImGui::PopID();

            // Right column: frame [row + 10]
            int idx_right = row + kRowsPerColumn;
            ImGui::TableSetColumnIndex(2);
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Frame %d:", idx_right);

            ImGui::TableSetColumnIndex(3);
            ImGui::PushID(idx_right);
            ImGui::SetNextItemWidth(-1);
            ImGui::InputText("##cpf", &gd->lip_sync_chars_per_frame[idx_right]);
            ImGui::PopID();
        }

        ImGui::EndTable();
    }
}

} // namespace AGSEditor
