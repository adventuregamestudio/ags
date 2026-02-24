// AGS Editor ImGui - Welcome Pane
#pragma once

#include "ui/editor_ui.h"

namespace AGSEditor
{

class WelcomePane : public EditorPane
{
public:
    explicit WelcomePane(EditorUI& editor);

    void Draw() override;
    const char* GetTitle() const override { return "Welcome"; }

private:
    void DrawActionButtons();
    void DrawRecentGames();
    void DrawFeatureList();

    void ActionNewGame();
    void ActionOpenGame();
    void ActionOpenRecentGame(const std::string& path);

    EditorUI& editor_;
};

} // namespace AGSEditor
