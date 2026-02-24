// AGS Editor ImGui - Default Setup pane
// Allows editing the runtime configuration that gets written to the game's setup.
#pragma once

#include "ui/editor_ui.h"

namespace AGSEditor
{

class DefaultSetupPane : public EditorPane
{
public:
    explicit DefaultSetupPane(EditorUI& editor);

    void Draw() override;
    const char* GetTitle() const override { return "Default Setup"; }

private:
    void DrawGraphicsSection();
    void DrawAudioSection();
    void DrawGameplaySection();
    void DrawMouseSection();
    void DrawTouchSection();
    void DrawMiscSection();
    void DrawPerformanceSection();
    void DrawEnvironmentSection();

    EditorUI& editor_;
};

} // namespace AGSEditor
