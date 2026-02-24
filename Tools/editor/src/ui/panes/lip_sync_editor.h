#pragma once
// AGS Editor ImGui - Lip Sync Editor pane
// Allows editing lip-sync frames for character speech
// Uses the same data model as the C# editor: CharactersPerFrame[20] array
// Operates directly on GameData (no copy-and-sync pattern).

#include "ui/editor_ui.h"

namespace AGSEditor
{

class LipSyncEditor : public EditorPane
{
public:
    explicit LipSyncEditor(EditorUI& editor);

    void Draw() override;
    const char* GetTitle() const override { return "Lip Sync"; }

private:
    void ResetToDefaults();

    EditorUI& editor_;
};

} // namespace AGSEditor
