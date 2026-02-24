// AGS Editor ImGui - Preferences Dialog Pane
#pragma once

#include "ui/editor_ui.h"
#include "core/preferences.h"

namespace AGSEditor
{

class PreferencesPane : public EditorPane
{
public:
    explicit PreferencesPane(EditorUI& editor);

    void Draw() override;
    const char* GetTitle() const override { return "Editor Preferences"; }

private:
    void DrawAppearanceSection();
    void DrawEditorSection();
    void DrawBehaviorSection();
    void DrawPathsSection();
    void DrawAndroidSection();
    void DrawShortcutsSection();
    void DrawAboutSection();
    void ApplyTheme();

    EditorUI& editor_;
    EditorPreferences prefs_;
    bool needs_font_reload_ = false;
    int active_section_ = 0;
};

} // namespace AGSEditor
