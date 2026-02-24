// AGS Editor ImGui - Native TTF Font Renderer implementation
// Uses FreeType 2.x to render TrueType fonts to pixel buffers.
#include "font/ttf_renderer.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include <SDL.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>

namespace AGSEditor
{

TTFRenderer::TTFRenderer() = default;

TTFRenderer::~TTFRenderer()
{
    UnloadAll();
    if (ft_library_)
    {
        FT_Done_FreeType(ft_library_);
        ft_library_ = nullptr;
    }
}

bool TTFRenderer::Init()
{
    if (ft_library_)
        return true;
    FT_Error err = FT_Init_FreeType(&ft_library_);
    return err == 0;
}

int TTFRenderer::LoadFont(const std::string& path, int pixel_size)
{
    if (!ft_library_ && !Init())
        return -1;

    // Read the entire font file into memory (FreeType needs data to remain valid)
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open())
        return -1;

    auto size = file.tellg();
    if (size <= 0)
        return -1;

    TTFFont font;
    font.file_data.resize((size_t)size);
    file.seekg(0);
    file.read(reinterpret_cast<char*>(font.file_data.data()), size);
    if (!file.good())
        return -1;

    FT_Error err = FT_New_Memory_Face(ft_library_,
        font.file_data.data(), (FT_Long)font.file_data.size(), 0, &font.face);
    if (err != 0)
        return -1;

    // Set pixel size
    err = FT_Set_Pixel_Sizes(font.face, 0, (FT_UInt)pixel_size);
    if (err != 0)
    {
        FT_Done_Face(font.face);
        return -1;
    }

    font.pixel_size = pixel_size;
    // Metrics are in 26.6 fixed point (1/64th of pixel)
    font.ascender = (int)(font.face->size->metrics.ascender >> 6);
    font.descender = (int)(-font.face->size->metrics.descender >> 6);
    font.line_height = (int)(font.face->size->metrics.height >> 6);

    int id = next_id_++;
    fonts_[id] = std::move(font);
    return id;
}

void TTFRenderer::UnloadFont(int font_id)
{
    auto it = fonts_.find(font_id);
    if (it != fonts_.end())
    {
        if (it->second.face)
            FT_Done_Face(it->second.face);
        fonts_.erase(it);
    }
}

void TTFRenderer::UnloadAll()
{
    for (auto& [id, font] : fonts_)
    {
        if (font.face)
            FT_Done_Face(font.face);
    }
    fonts_.clear();
}

bool TTFRenderer::IsFontLoaded(int font_id) const
{
    return fonts_.find(font_id) != fonts_.end();
}

int TTFRenderer::GetLineHeight(int font_id) const
{
    auto it = fonts_.find(font_id);
    return it != fonts_.end() ? it->second.line_height : 0;
}

int TTFRenderer::GetAscender(int font_id) const
{
    auto it = fonts_.find(font_id);
    return it != fonts_.end() ? it->second.ascender : 0;
}

int TTFRenderer::DecodeUTF8(const char*& p)
{
    unsigned char c = (unsigned char)*p;
    if (c == 0)
        return 0;

    int cp;
    int bytes;
    if (c < 0x80)
    {
        cp = c;
        bytes = 1;
    }
    else if (c < 0xC0)
    {
        // Invalid continuation byte — skip
        p++;
        return 0xFFFD;
    }
    else if (c < 0xE0)
    {
        cp = c & 0x1F;
        bytes = 2;
    }
    else if (c < 0xF0)
    {
        cp = c & 0x0F;
        bytes = 3;
    }
    else
    {
        cp = c & 0x07;
        bytes = 4;
    }

    p++;
    for (int i = 1; i < bytes; i++)
    {
        unsigned char cont = (unsigned char)*p;
        if ((cont & 0xC0) != 0x80)
            return 0xFFFD;
        cp = (cp << 6) | (cont & 0x3F);
        p++;
    }
    return cp;
}

int TTFRenderer::MeasureTextWidth(int font_id, const char* text) const
{
    auto it = fonts_.find(font_id);
    if (it == fonts_.end() || !text || !*text)
        return 0;

    FT_Face face = it->second.face;
    int pen_x = 0;

    const char* p = text;
    while (*p)
    {
        int cp = DecodeUTF8(p);
        if (cp == 0)
            break;

        FT_UInt glyph_index = FT_Get_Char_Index(face, (FT_ULong)cp);
        if (FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT) != 0)
            continue;

        pen_x += (int)(face->glyph->advance.x >> 6);
    }
    return pen_x;
}

RenderedText TTFRenderer::RenderText(int font_id, const char* text,
                                      uint32_t color, int max_width) const
{
    RenderedText result;
    auto it = fonts_.find(font_id);
    if (it == fonts_.end() || !text || !*text)
        return result;

    const TTFFont& font = it->second;
    FT_Face face = font.face;

    // First pass: measure total width and collect glyph info
    struct GlyphInfo
    {
        int codepoint;
        int bearing_x;
        int bearing_y;
        int bmp_width;
        int bmp_rows;
        int advance;
        std::vector<uint8_t> bitmap;
    };

    std::vector<GlyphInfo> glyphs;
    int total_advance = 0;

    const char* p = text;
    while (*p)
    {
        int cp = DecodeUTF8(p);
        if (cp == 0)
            break;

        FT_UInt glyph_index = FT_Get_Char_Index(face, (FT_ULong)cp);
        if (FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT) != 0)
            continue;
        if (FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL) != 0)
            continue;

        GlyphInfo gi;
        gi.codepoint = cp;
        gi.bearing_x = face->glyph->bitmap_left;
        gi.bearing_y = face->glyph->bitmap_top;
        gi.bmp_width = (int)face->glyph->bitmap.width;
        gi.bmp_rows = (int)face->glyph->bitmap.rows;
        gi.advance = (int)(face->glyph->advance.x >> 6);

        // Copy the glyph bitmap (8-bit alpha)
        int bmp_size = gi.bmp_width * gi.bmp_rows;
        if (bmp_size > 0)
        {
            gi.bitmap.resize(bmp_size);
            for (int row = 0; row < gi.bmp_rows; row++)
            {
                memcpy(&gi.bitmap[row * gi.bmp_width],
                       &face->glyph->bitmap.buffer[row * face->glyph->bitmap.pitch],
                       gi.bmp_width);
            }
        }

        // Check max_width
        if (max_width > 0 && total_advance + gi.advance > max_width && !glyphs.empty())
            break;

        total_advance += gi.advance;
        glyphs.push_back(std::move(gi));
    }

    if (glyphs.empty())
        return result;

    // Compute bitmap dimensions
    result.width = total_advance;
    result.height = font.line_height;
    if (result.width <= 0 || result.height <= 0)
        return result;

    result.pixels.resize(result.width * result.height * 4, 0);

    // Extract color channels
    uint8_t r = (uint8_t)((color >> 16) & 0xFF);
    uint8_t g = (uint8_t)((color >> 8) & 0xFF);
    uint8_t b = (uint8_t)(color & 0xFF);

    // Second pass: composite glyphs onto the bitmap
    int pen_x = 0;
    for (const auto& gi : glyphs)
    {
        int gx = pen_x + gi.bearing_x;
        int gy = font.ascender - gi.bearing_y;

        for (int row = 0; row < gi.bmp_rows; row++)
        {
            int dst_y = gy + row;
            if (dst_y < 0 || dst_y >= result.height)
                continue;

            for (int col = 0; col < gi.bmp_width; col++)
            {
                int dst_x = gx + col;
                if (dst_x < 0 || dst_x >= result.width)
                    continue;

                uint8_t alpha = gi.bitmap[row * gi.bmp_width + col];
                if (alpha == 0)
                    continue;

                int offset = (dst_y * result.width + dst_x) * 4;

                // Alpha blending (source over)
                uint8_t dst_a = result.pixels[offset + 3];
                if (dst_a == 0)
                {
                    result.pixels[offset + 0] = b;
                    result.pixels[offset + 1] = g;
                    result.pixels[offset + 2] = r;
                    result.pixels[offset + 3] = alpha;
                }
                else
                {
                    // Blend
                    int sa = alpha;
                    int da = dst_a;
                    int out_a = sa + da * (255 - sa) / 255;
                    if (out_a > 0)
                    {
                        result.pixels[offset + 0] = (uint8_t)((b * sa + result.pixels[offset + 0] * da * (255 - sa) / 255) / out_a);
                        result.pixels[offset + 1] = (uint8_t)((g * sa + result.pixels[offset + 1] * da * (255 - sa) / 255) / out_a);
                        result.pixels[offset + 2] = (uint8_t)((r * sa + result.pixels[offset + 2] * da * (255 - sa) / 255) / out_a);
                        result.pixels[offset + 3] = (uint8_t)out_a;
                    }
                }
            }
        }

        pen_x += gi.advance;
    }

    return result;
}

RenderedText TTFRenderer::RenderGlyph(int font_id, int codepoint, uint32_t color) const
{
    RenderedText result;
    auto it = fonts_.find(font_id);
    if (it == fonts_.end())
        return result;

    const TTFFont& font = it->second;
    FT_Face face = font.face;

    FT_UInt glyph_index = FT_Get_Char_Index(face, (FT_ULong)codepoint);
    if (FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT) != 0)
        return result;
    if (FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL) != 0)
        return result;

    int bmp_w = (int)face->glyph->bitmap.width;
    int bmp_h = (int)face->glyph->bitmap.rows;

    // Output height = line height, width = max of glyph width and advance
    result.width = std::max(bmp_w, (int)(face->glyph->advance.x >> 6));
    result.height = font.line_height;
    if (result.width <= 0)
        result.width = font.pixel_size; // fallback for space/missing
    if (result.width <= 0 || result.height <= 0)
        return result;

    result.pixels.resize(result.width * result.height * 4, 0);

    uint8_t r = (uint8_t)((color >> 16) & 0xFF);
    uint8_t g = (uint8_t)((color >> 8) & 0xFF);
    uint8_t b = (uint8_t)(color & 0xFF);

    int gx = face->glyph->bitmap_left;
    int gy = font.ascender - face->glyph->bitmap_top;

    for (int row = 0; row < bmp_h; row++)
    {
        int dst_y = gy + row;
        if (dst_y < 0 || dst_y >= result.height)
            continue;

        for (int col = 0; col < bmp_w; col++)
        {
            int dst_x = gx + col;
            if (dst_x < 0 || dst_x >= result.width)
                continue;

            uint8_t alpha = face->glyph->bitmap.buffer[row * face->glyph->bitmap.pitch + col];
            if (alpha == 0)
                continue;

            int offset = (dst_y * result.width + dst_x) * 4;
            result.pixels[offset + 0] = b;
            result.pixels[offset + 1] = g;
            result.pixels[offset + 2] = r;
            result.pixels[offset + 3] = alpha;
        }
    }

    return result;
}

RenderedText TTFRenderer::RenderTextOutlined(int font_id, const char* text,
                                              uint32_t text_color, uint32_t outline_color,
                                              int outline_dist, int max_width) const
{
    auto it = fonts_.find(font_id);
    if (it == fonts_.end() || !text || !*text)
        return {};

    const TTFFont& font = it->second;

    // Measure text to determine output bitmap size (with outline padding)
    int text_w = MeasureTextWidth(font_id, text);
    if (text_w <= 0)
        return {};
    if (max_width > 0 && text_w > max_width)
        text_w = max_width;

    int pad = outline_dist;
    int out_w = text_w + pad * 2;
    int out_h = font.line_height + pad * 2;

    RenderedText result;
    result.width = out_w;
    result.height = out_h;
    result.pixels.resize(out_w * out_h * 4, 0);

    // Render outline at 8 offsets
    static const int offsets[][2] = {{-1,0},{1,0},{0,-1},{0,1},{-1,-1},{1,-1},{-1,1},{1,1}};
    RenderedText outline_text = RenderText(font_id, text, outline_color, max_width);

    for (auto& off : offsets)
    {
        int ox = pad + off[0] * outline_dist;
        int oy = pad + off[1] * outline_dist;
        // Composite outline_text onto result at (ox, oy)
        for (int y = 0; y < outline_text.height; y++)
        {
            int dst_y = oy + y;
            if (dst_y < 0 || dst_y >= out_h)
                continue;
            for (int x = 0; x < outline_text.width; x++)
            {
                int dst_x = ox + x;
                if (dst_x < 0 || dst_x >= out_w)
                    continue;

                int src_off = (y * outline_text.width + x) * 4;
                uint8_t sa = outline_text.pixels[src_off + 3];
                if (sa == 0)
                    continue;

                int dst_off = (dst_y * out_w + dst_x) * 4;
                uint8_t da = result.pixels[dst_off + 3];
                if (da == 0)
                {
                    memcpy(&result.pixels[dst_off], &outline_text.pixels[src_off], 4);
                }
                else
                {
                    int out_a = sa + da * (255 - sa) / 255;
                    if (out_a > 0)
                    {
                        for (int c = 0; c < 3; c++)
                            result.pixels[dst_off + c] = (uint8_t)(
                                (outline_text.pixels[src_off + c] * sa +
                                 result.pixels[dst_off + c] * da * (255 - sa) / 255) / out_a);
                        result.pixels[dst_off + 3] = (uint8_t)out_a;
                    }
                }
            }
        }
    }

    // Render main text on top
    RenderedText main_text = RenderText(font_id, text, text_color, max_width);
    int mx = pad;
    int my = pad;
    for (int y = 0; y < main_text.height; y++)
    {
        int dst_y = my + y;
        if (dst_y < 0 || dst_y >= out_h)
            continue;
        for (int x = 0; x < main_text.width; x++)
        {
            int dst_x = mx + x;
            if (dst_x < 0 || dst_x >= out_w)
                continue;

            int src_off = (y * main_text.width + x) * 4;
            uint8_t sa = main_text.pixels[src_off + 3];
            if (sa == 0)
                continue;

            int dst_off = (dst_y * out_w + dst_x) * 4;
            uint8_t da = result.pixels[dst_off + 3];
            if (da == 0)
            {
                memcpy(&result.pixels[dst_off], &main_text.pixels[src_off], 4);
            }
            else
            {
                int out_a = sa + da * (255 - sa) / 255;
                if (out_a > 0)
                {
                    for (int c = 0; c < 3; c++)
                        result.pixels[dst_off + c] = (uint8_t)(
                            (main_text.pixels[src_off + c] * sa +
                             result.pixels[dst_off + c] * da * (255 - sa) / 255) / out_a);
                    result.pixels[dst_off + 3] = (uint8_t)out_a;
                }
            }
        }
    }

    return result;
}

SDL_Texture* TTFRenderer::UploadToTexture(SDL_Renderer* renderer,
                                            const RenderedText& rt) const
{
    if (!renderer || rt.width <= 0 || rt.height <= 0 || rt.pixels.empty())
        return nullptr;

    SDL_Texture* tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888,
                                          SDL_TEXTUREACCESS_STATIC,
                                          rt.width, rt.height);
    if (!tex)
        return nullptr;

    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    SDL_UpdateTexture(tex, nullptr, rt.pixels.data(), rt.width * 4);
    return tex;
}

} // namespace AGSEditor
