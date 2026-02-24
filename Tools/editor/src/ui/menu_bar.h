// AGS Editor ImGui - Menu Bar
#pragma once

#include <string>

namespace AGSEditor
{

class EditorUI;

class MenuBar
{
public:
    explicit MenuBar(EditorUI& editor);

    void Draw();

private:
    void DrawFileMenu();
    void DrawEditMenu();
    void DrawBuildMenu();
    void DrawToolsMenu();
    void DrawWindowMenu();
    void DrawHelpMenu();

    void RestoreSpritesFromSources();

    EditorUI& editor_;
    bool show_about_ = false;
    bool show_game_stats_ = false;
    bool show_go_to_room_ = false;
    int go_to_room_number_ = 0;
    bool show_auto_number_result_ = false;
    bool show_voice_script_result_ = false;
    std::string speech_result_message_;
    bool show_export_global_messages_ = false;
    bool show_remove_global_messages_ = false;
    bool show_restore_sprites_result_ = false;
    std::string restore_sprites_message_;
};

} // namespace AGSEditor
