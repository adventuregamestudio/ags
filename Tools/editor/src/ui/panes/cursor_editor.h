// AGS Editor ImGui - Cursor Editor pane
// Displays cursors from loaded GameData with real sprite rendering.
#pragma once

#include "ui/editor_ui.h"

namespace AGSEditor
{

class CursorEditor : public EditorPane
{
public:
    explicit CursorEditor(EditorUI& editor, int cursor_id = -1);

    void Draw() override;
    const char* GetTitle() const override { return "Cursors"; }

private:
    void DrawCursorList();
    void DrawCursorProperties();
    void DrawCursorPreview();

    EditorUI& editor_;
    int selected_cursor_ = 0;
    bool confirm_delete_ = false;
    float preview_zoom_ = 2.0f;
};

} // namespace AGSEditor
