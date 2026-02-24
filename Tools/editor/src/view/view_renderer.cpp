// AGS Editor ImGui - Native View Renderer implementation
// Uses AGS::Common PixelBuffer + SpriteFile for pixel-accurate sprite rendering
// and composites view loop frames into strip textures.
#include "view/view_renderer.h"
#include "project/sprite_loader.h"
#include "project/texture_cache.h"
#include "ac/spritefile.h"
#include "gfx/bitmapdata.h"

#include <SDL.h>
#include <cstring>
#include <cstdio>
#include <algorithm>
#include <cmath>

namespace AGSEditor
{

using namespace AGS::Common;

ViewRenderer::~ViewRenderer()
{
    ClearCache();
}

void ViewRenderer::SetPalette(const uint32_t* palette_argb32, int count)
{
    int n = (count > 256) ? 256 : count;
    for (int i = 0; i < n; i++)
        palette_[i] = palette_argb32[i];
    for (int i = n; i < 256; i++)
        palette_[i] = 0xFF000000;
    has_palette_ = (n > 0);
}

void ViewRenderer::ClearCache()
{
    for (auto& [key, cs] : strip_cache_)
    {
        if (cs.texture)
            SDL_DestroyTexture(cs.texture);
    }
    strip_cache_.clear();
    sprite_cache_.clear();
}

// Convert pixel buffer data to ARGB32, applying palette for 8-bit.
// Returns new buffer with ARGB32 pixels. Caller owns the buffer.
static std::unique_ptr<uint8_t[]> ConvertToARGB32(
    const uint8_t* src, int w, int h, int bpp, size_t stride,
    const uint32_t* palette, bool has_palette)
{
    auto dst = std::make_unique<uint8_t[]>(w * h * 4);

    if (bpp == 4)
    {
        // Already ARGB32, just copy
        for (int y = 0; y < h; y++)
            memcpy(dst.get() + y * w * 4, src + y * stride, w * 4);
    }
    else if (bpp == 3)
    {
        // RGB24 -> ARGB32
        for (int y = 0; y < h; y++)
        {
            const uint8_t* s = src + y * stride;
            uint8_t* d = dst.get() + y * w * 4;
            for (int x = 0; x < w; x++)
            {
                d[x * 4 + 0] = s[x * 3 + 0]; // B
                d[x * 4 + 1] = s[x * 3 + 1]; // G
                d[x * 4 + 2] = s[x * 3 + 2]; // R
                d[x * 4 + 3] = 255;
            }
        }
    }
    else if (bpp == 2)
    {
        // RGB565 -> ARGB32
        for (int y = 0; y < h; y++)
        {
            const uint16_t* s = reinterpret_cast<const uint16_t*>(src + y * stride);
            uint8_t* d = dst.get() + y * w * 4;
            for (int x = 0; x < w; x++)
            {
                uint16_t px = s[x];
                uint8_t r = ((px >> 11) & 0x1F) * 255 / 31;
                uint8_t g = ((px >>  5) & 0x3F) * 255 / 63;
                uint8_t b = ((px >>  0) & 0x1F) * 255 / 31;
                d[x * 4 + 0] = b;
                d[x * 4 + 1] = g;
                d[x * 4 + 2] = r;
                d[x * 4 + 3] = 255;
            }
        }
    }
    else if (bpp == 1)
    {
        // 8-bit indexed -> ARGB32
        for (int y = 0; y < h; y++)
        {
            const uint8_t* s = src + y * stride;
            uint8_t* d = dst.get() + y * w * 4;
            for (int x = 0; x < w; x++)
            {
                uint8_t idx = s[x];
                if (has_palette)
                {
                    uint32_t c = palette[idx];
                    d[x * 4 + 0] = (c >>  0) & 0xFF; // B
                    d[x * 4 + 1] = (c >>  8) & 0xFF; // G
                    d[x * 4 + 2] = (c >> 16) & 0xFF; // R
                    d[x * 4 + 3] = (c >> 24) & 0xFF; // A
                }
                else
                {
                    d[x * 4 + 0] = idx;
                    d[x * 4 + 1] = idx;
                    d[x * 4 + 2] = idx;
                    d[x * 4 + 3] = 255;
                }
            }
        }
    }
    else
    {
        // Unknown: fill magenta
        for (int y = 0; y < h; y++)
        {
            uint8_t* d = dst.get() + y * w * 4;
            for (int x = 0; x < w; x++)
            {
                d[x * 4 + 0] = 255;
                d[x * 4 + 1] = 0;
                d[x * 4 + 2] = 255;
                d[x * 4 + 3] = 255;
            }
        }
    }

    return dst;
}

// Blit scaled ARGB32 source into ARGB32 destination with nearest-neighbor sampling
static void BlitScaledARGB32(
    const uint8_t* src, int src_w, int src_h,
    uint8_t* dst, int dst_stride,
    int dst_x, int dst_y, int dst_w, int dst_h,
    int dst_total_w, int dst_total_h)
{
    if (dst_w <= 0 || dst_h <= 0 || src_w <= 0 || src_h <= 0)
        return;

    for (int y = 0; y < dst_h; y++)
    {
        int dy = dst_y + y;
        if (dy < 0 || dy >= dst_total_h) continue;

        int sy = y * src_h / dst_h;
        if (sy >= src_h) sy = src_h - 1;

        const uint32_t* srow = reinterpret_cast<const uint32_t*>(src + sy * src_w * 4);
        uint32_t* drow = reinterpret_cast<uint32_t*>(dst + dy * dst_stride);

        for (int x = 0; x < dst_w; x++)
        {
            int dx = dst_x + x;
            if (dx < 0 || dx >= dst_total_w) continue;

            int sx = x * src_w / dst_w;
            if (sx >= src_w) sx = src_w - 1;

            uint32_t pixel = srow[sx];
            uint8_t sa = (pixel >> 24) & 0xFF;
            if (sa == 0) continue; // Fully transparent

            if (sa == 255)
            {
                drow[dx] = pixel;
            }
            else
            {
                // Alpha blend
                uint32_t dpx = drow[dx];
                uint8_t dr = (dpx >> 16) & 0xFF;
                uint8_t dg = (dpx >>  8) & 0xFF;
                uint8_t db = (dpx >>  0) & 0xFF;
                uint8_t sr = (pixel >> 16) & 0xFF;
                uint8_t sg = (pixel >>  8) & 0xFF;
                uint8_t sb = (pixel >>  0) & 0xFF;
                uint8_t inv_a = 255 - sa;
                uint8_t or_ = (sr * sa + dr * inv_a) / 255;
                uint8_t og = (sg * sa + dg * inv_a) / 255;
                uint8_t ob = (sb * sa + db * inv_a) / 255;
                drow[dx] = 0xFF000000 | (or_ << 16) | (og << 8) | ob;
            }
        }
    }
}

// Flip ARGB32 buffer horizontally in place
static void FlipHorizontalARGB32(uint8_t* data, int w, int h)
{
    for (int y = 0; y < h; y++)
    {
        uint32_t* row = reinterpret_cast<uint32_t*>(data + y * w * 4);
        for (int x = 0; x < w / 2; x++)
        {
            uint32_t tmp = row[x];
            row[x] = row[w - 1 - x];
            row[w - 1 - x] = tmp;
        }
    }
}

ViewRenderer::LoopStripInfo ViewRenderer::RenderLoopStrip(
    const std::vector<FrameInput>& frames,
    int cell_size,
    SpriteLoader* loader)
{
    LoopStripInfo result;
    if (frames.empty() || !renderer_ || !loader || !loader->IsOpen())
        return result;

    auto* sprite_file = loader->GetSpriteFile();
    if (!sprite_file)
        return result;

    int frame_count = (int)frames.size();
    int pad = 2; // Pixel padding between frames
    int strip_w = (cell_size + pad) * frame_count + pad;
    int strip_h = cell_size + pad * 2;

    // Create strip buffer (ARGB32), fill with dark background
    std::vector<uint8_t> strip(strip_w * strip_h * 4, 0);
    // Fill with background color 0xFF232335
    {
        uint32_t* px = reinterpret_cast<uint32_t*>(strip.data());
        int total = strip_w * strip_h;
        for (int i = 0; i < total; i++)
            px[i] = 0xFF23232D;
    }

    result.width = strip_w;
    result.height = strip_h;

    // Composite each frame
    for (int f = 0; f < frame_count; f++)
    {
        int spr_id = frames[f].sprite_id;
        bool flipped = frames[f].flipped;
        int fx = pad + f * (cell_size + pad);
        int fy = pad;

        // Frame cell background
        {
            uint32_t* px = reinterpret_cast<uint32_t*>(strip.data());
            for (int y = fy; y < fy + cell_size && y < strip_h; y++)
                for (int x = fx; x < fx + cell_size && x < strip_w; x++)
                    px[y * strip_w + x] = 0xFF37374B;
        }

        FrameBounds fb;
        fb.x = fx;
        fb.y = fy;
        fb.w = cell_size;
        fb.h = cell_size;
        result.frame_bounds.push_back(fb);

        // Load sprite pixel data
        if (spr_id < 0) continue;
        PixelBuffer pixbuf;
        HError err = sprite_file->LoadSprite(spr_id, pixbuf);
        if (!err || !pixbuf.GetData() ||
            pixbuf.GetWidth() <= 0 || pixbuf.GetHeight() <= 0)
            continue;

        int sw = pixbuf.GetWidth();
        int sh = pixbuf.GetHeight();
        int bpp = pixbuf.GetBytesPerPixel();

        // Convert to ARGB32
        auto argb = ConvertToARGB32(
            pixbuf.GetData(), sw, sh, bpp, pixbuf.GetStride(),
            palette_, has_palette_);
        if (!argb) continue;

        // Flip if needed
        if (flipped)
            FlipHorizontalARGB32(argb.get(), sw, sh);

        // Calculate proportional fit within cell
        int margin = 2;
        int avail = cell_size - margin * 2;
        float scale = std::min((float)avail / sw, (float)avail / sh);
        int dw = (int)(sw * scale);
        int dh = (int)(sh * scale);
        if (dw < 1) dw = 1;
        if (dh < 1) dh = 1;
        int ox = fx + (cell_size - dw) / 2;
        int oy = fy + (cell_size - dh) / 2;

        // Blit scaled sprite into strip
        BlitScaledARGB32(argb.get(), sw, sh,
            strip.data(), strip_w * 4,
            ox, oy, dw, dh,
            strip_w, strip_h);
    }

    // Upload to SDL texture
    SDL_Texture* tex = SDL_CreateTexture(renderer_,
        SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC,
        strip_w, strip_h);
    if (tex)
    {
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
        SDL_UpdateTexture(tex, nullptr, strip.data(), strip_w * 4);
        result.texture = tex;
    }

    return result;
}

AGS::Common::Bitmap* ViewRenderer::RenderSpriteFitted(
    int sprite_id, int max_w, int max_h,
    bool flipped,
    SpriteLoader* loader)
{
    // Not implemented yet. The composited strip rendering is the primary use.
    (void)sprite_id; (void)max_w; (void)max_h; (void)flipped; (void)loader;
    return nullptr;
}

} // namespace AGSEditor
