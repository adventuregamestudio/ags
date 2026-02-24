// AGS Editor ImGui - New Game Wizard Dialog
#pragma once

#include <string>
#include <functional>
#include <vector>

namespace AGSEditor
{

struct GameTemplate;

struct NewGameSettings
{
    char game_title[128] = "My Adventure Game";
    int resolution_type = 2; // 0=320x200, 1=320x240, 2=640x480, 3=800x600, 4=1024x768, 5=1280x720, 6=1920x1080, 7=custom
    int custom_width = 640;
    int custom_height = 480;
    int color_depth = 2; // 0=8bit, 1=16bit, 2=32bit
    char project_dir[512] = "";
    int template_index = 0;

    // If a real template is selected, this points to it
    const GameTemplate* selected_template = nullptr;

    int GetWidth() const;
    int GetHeight() const;
    int GetColorDepthBits() const;
};

class NewGameDialog
{
public:
    using Callback = std::function<void(const NewGameSettings&)>;

    // Open the dialog, optionally with discovered templates
    static void Open(Callback on_create = nullptr,
                     const std::vector<GameTemplate>* templates = nullptr);

    // Draw the dialog (call every frame)
    static void Draw();

    // Check if dialog is open
    static bool IsOpen();

private:
    static void DrawPageBasic();
    static void DrawPageGraphics();
    static void DrawPageTemplate();
    static void DrawPageSummary();

    static bool open_;
    static int current_page_;
    static NewGameSettings settings_;
    static Callback callback_;
    static const std::vector<GameTemplate>* templates_;
};

} // namespace AGSEditor
