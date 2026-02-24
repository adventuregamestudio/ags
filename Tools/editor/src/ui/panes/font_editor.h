// AGS Editor ImGui - Font Editor pane
// Displays fonts from loaded GameData. Loads TTF/WFN fonts for preview.
#pragma once

#include "imgui.h"
#include "ui/editor_ui.h"
#include "font/wfnfont.h"
#include "font/ttf_renderer.h"
#include <string>
#include <unordered_map>

struct ImFont;
struct ImDrawList;

namespace AGSEditor
{

class FontEditor : public EditorPane
{
public:
    explicit FontEditor(EditorUI& editor, int font_id = -1);

    void Draw() override;
    const char* GetTitle() const override { return title_.c_str(); }

private:
    void DrawFontList();
    void DrawFontProperties();
    void DrawFontPreview();
    void DrawCharacterGrid();
    void DrawContextMenu();
    void DeleteFont(int index);
    void ImportFont(const std::string& path);

    // WFN bitmap font rendering helpers
    WFNFont* GetWFNFont(int font_id);
    void DrawWFNChar(ImDrawList* dl, const WFNChar& ch, float x, float y,
                     float scale, ImU32 color) const;
    float DrawWFNText(ImDrawList* dl, const WFNFont& font, const char* text,
                      float x, float y, float scale, ImU32 color,
                      float max_width = 0.0f) const;

    // TTF native rendering helpers
    int GetTTFFont(int font_id, int pixel_size);
    void DrawTTFPreview(ImDrawList* dl, int ttf_id, const char* text,
                        float x, float y, float scale,
                        bool outline, float max_width);
    void DrawTTFGlyph(ImDrawList* dl, int ttf_id, int codepoint,
                      float x, float y, float scale);

    EditorUI& editor_;
    std::string title_;

    int selected_font_ = 0;
    std::unordered_map<int, WFNFont> wfn_cache_;
    TTFRenderer ttf_renderer_;
    // Maps (font_id * 1000 + pixel_size) -> ttf renderer font id
    std::unordered_map<int, int> ttf_cache_;
    bool confirm_delete_ = false;
    bool show_ttf_size_dialog_ = false;
    std::string import_ttf_path_;
    int import_ttf_size_ = 10;
    int selected_char_ = -1;
    int grid_cell_size_ = 32;
    bool show_char_codes_ = false;
    bool show_hex_codes_ = false;
    bool scroll_to_char_ = false;
    bool unicode_mode_ = false;
    int unicode_range_start_ = 0;
    int unicode_range_end_ = 0xFF;
    char char_code_input_[16] = "";
    char char_text_input_[4] = "";
    char preview_text_[256] = "The quick brown fox jumps over the lazy dog. 0123456789";
    int preview_scale_ = 1;
};

} // namespace AGSEditor
