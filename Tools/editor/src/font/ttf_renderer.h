// AGS Editor ImGui - Native TTF Font Renderer
// Uses FreeType directly to render TTF text to pixel buffers,
// matching the engine's font rendering more closely than ImGui's
// built-in font rasterizer. Rendered text is uploaded as SDL textures
// for display via ImGui::Image().
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// Forward declarations for FreeType
typedef struct FT_LibraryRec_* FT_Library;
typedef struct FT_FaceRec_* FT_Face;
struct SDL_Texture;
struct SDL_Renderer;

namespace AGSEditor
{

// Rendered text result — a pre-rasterized bitmap ready for GPU upload
struct RenderedText
{
    std::vector<uint8_t> pixels; // ARGB32 pixel data
    int width = 0;
    int height = 0;
};

// Caches a loaded FreeType face at a specific pixel size
struct TTFFont
{
    FT_Face face = nullptr;
    std::vector<uint8_t> file_data; // font file contents (FreeType needs them alive)
    int pixel_size = 0;
    int ascender = 0;  // pixels above baseline
    int descender = 0; // pixels below baseline (positive value)
    int line_height = 0;
};

// Native TTF renderer using FreeType.
// Renders text to ARGB bitmaps that can be uploaded as SDL textures.
class TTFRenderer
{
public:
    TTFRenderer();
    ~TTFRenderer();

    // Initialize FreeType library
    bool Init();

    // Load a TTF font file at a given pixel size.
    // Returns a font ID for use with RenderText, or -1 on failure.
    int LoadFont(const std::string& path, int pixel_size);

    // Unload a previously loaded font
    void UnloadFont(int font_id);

    // Unload all fonts
    void UnloadAll();

    // Check if a font is loaded
    bool IsFontLoaded(int font_id) const;

    // Get font metrics
    int GetLineHeight(int font_id) const;
    int GetAscender(int font_id) const;

    // Render text to an ARGB32 pixel buffer.
    // color is ARGB format (0xAARRGGBB).
    // max_width: if > 0, clip text to this width (no wrapping).
    RenderedText RenderText(int font_id, const char* text, uint32_t color,
                            int max_width = 0) const;

    // Render a single character glyph to an ARGB32 pixel buffer.
    RenderedText RenderGlyph(int font_id, int codepoint, uint32_t color) const;

    // Measure text width without rendering
    int MeasureTextWidth(int font_id, const char* text) const;

    // Render text with outline (draws text at 8 offsets in outline_color,
    // then main text on top). scale multiplies the outline offset distance.
    RenderedText RenderTextOutlined(int font_id, const char* text,
                                    uint32_t text_color, uint32_t outline_color,
                                    int outline_dist = 1, int max_width = 0) const;

    // Upload a RenderedText to an SDL texture. Caller must manage lifetime.
    // Returns nullptr on failure.
    SDL_Texture* UploadToTexture(SDL_Renderer* renderer,
                                  const RenderedText& rt) const;

private:
    FT_Library ft_library_ = nullptr;
    std::unordered_map<int, TTFFont> fonts_;
    int next_id_ = 1;

    // Decode one UTF-8 character, advance pointer. Returns codepoint.
    static int DecodeUTF8(const char*& p);
};

} // namespace AGSEditor
