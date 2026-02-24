// AGS Editor ImGui - SDL Texture Cache implementation
#include "texture_cache.h"
#include "sprite_loader.h"
#include "ac/spritefile.h"
#include "gfx/allegrobitmap.h"
#include "gfx/bitmapdata.h"

#include <SDL.h>
#include <cstdio>
#include <cstring>

namespace AGSEditor
{

using namespace AGS::Common;

TextureCache::TextureCache() {
}

TextureCache::~TextureCache()
{
    Clear();
}

void TextureCache::SetPalette(const uint32_t* palette_argb32, int count)
{
    int n = (count > 256) ? 256 : count;
    for (int i = 0; i < n; i++)
        palette_[i] = palette_argb32[i];
    for (int i = n; i < 256; i++)
        palette_[i] = 0xFF000000;
    has_palette_ = (n > 0);
}

void TextureCache::Clear()
{
    for (auto& [key, info] : cache_)
    {
        if (info.texture)
            SDL_DestroyTexture(info.texture);
    }
    cache_.clear();
}

void TextureCache::Evict(const std::string& key)
{
    auto it = cache_.find(key);
    if (it != cache_.end())
    {
        if (it->second.texture)
            SDL_DestroyTexture(it->second.texture);
        cache_.erase(it);
    }
}

void TextureCache::EvictSprite(int sprite_id)
{
    char key[32];
    snprintf(key, sizeof(key), "spr_%d", sprite_id);
    Evict(key);
}

const TextureCache::TexInfo* TextureCache::Find(const std::string& key) const
{
    auto it = cache_.find(key);
    return (it != cache_.end()) ? &it->second : nullptr;
}

const TextureCache::TexInfo* TextureCache::FindSprite(int sprite_id) const
{
    char key[32];
    snprintf(key, sizeof(key), "spr_%d", sprite_id);
    return Find(key);
}

SDL_Texture* TextureCache::GetSpriteTexture(int sprite_id, SpriteLoader* loader)
{
    if (!renderer_ || !loader || sprite_id < 0)
        return nullptr;

    // Check cache first
    char key[32];
    snprintf(key, sizeof(key), "spr_%d", sprite_id);
    auto it = cache_.find(key);
    if (it != cache_.end())
        return it->second.texture;

    // Load the sprite pixel data via AGS Common SpriteFile
    if (!loader->IsOpen())
        return nullptr;

    // Access the underlying SpriteFile to load pixel data
    auto* sprite_file = loader->GetSpriteFile();
    if (!sprite_file)
        return nullptr;

    PixelBuffer pixbuf;
    HError err = sprite_file->LoadSprite(sprite_id, pixbuf);
    if (!err)
    {
        fprintf(stderr, "[TextureCache] Failed to load sprite %d\n", sprite_id);
        return nullptr;
    }

    if (!pixbuf.GetData() || pixbuf.GetWidth() <= 0 || pixbuf.GetHeight() <= 0)
        return nullptr;

    SDL_Texture* tex = CreateFromPixelBuffer(pixbuf);
    if (tex)
    {
        TexInfo info;
        info.texture = tex;
        info.width = pixbuf.GetWidth();
        info.height = pixbuf.GetHeight();
        cache_[key] = info;
    }
    return tex;
}

SDL_Texture* TextureCache::GetOrCreateFromBitmap(const std::string& key,
                                                  const Bitmap* bmp)
{
    if (!renderer_ || !bmp)
        return nullptr;

    // Check cache
    auto it = cache_.find(key);
    if (it != cache_.end())
        return it->second.texture;

    int w = bmp->GetWidth();
    int h = bmp->GetHeight();
    int bpp = bmp->GetBPP();
    const uint8_t* data = bmp->GetData();

    if (!data || w <= 0 || h <= 0)
        return nullptr;

    SDL_Texture* tex = nullptr;
    if (bpp == 4)
    {
        tex = UploadARGB32(data, w, h, bmp->GetLineLength());
    }
    else if (bpp == 2)
    {
        tex = UploadRGB16(data, w, h, bmp->GetLineLength());
    }
    else
    {
        // Convert to ARGB32
        auto converted = ConvertToARGB32(data, w, h, bpp, bmp->GetLineLength());
        if (converted)
            tex = UploadARGB32(converted.get(), w, h, w * 4);
    }

    if (tex)
    {
        TexInfo info;
        info.texture = tex;
        info.width = w;
        info.height = h;
        cache_[key] = info;
    }
    return tex;
}

SDL_Texture* TextureCache::CreateFromPixelBuffer(const PixelBuffer& pixbuf)
{
    if (!renderer_)
        return nullptr;

    int w = pixbuf.GetWidth();
    int h = pixbuf.GetHeight();
    int bpp = pixbuf.GetBytesPerPixel();
    const uint8_t* data = pixbuf.GetData();

    if (!data || w <= 0 || h <= 0)
        return nullptr;

    if (bpp == 4)
    {
        return UploadARGB32(data, w, h, (int)pixbuf.GetStride());
    }
    else if (bpp == 2)
    {
        return UploadRGB16(data, w, h, (int)pixbuf.GetStride());
    }
    else
    {
        auto converted = ConvertToARGB32(data, w, h, bpp, (int)pixbuf.GetStride());
        if (converted)
            return UploadARGB32(converted.get(), w, h, w * 4);
    }
    return nullptr;
}

SDL_Texture* TextureCache::UploadARGB32(const uint8_t* data, int w, int h, int stride)
{
    SDL_Texture* tex = SDL_CreateTexture(renderer_,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STATIC, w, h);
    if (!tex)
    {
        fprintf(stderr, "[TextureCache] SDL_CreateTexture failed: %s\n", SDL_GetError());
        return nullptr;
    }
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    SDL_UpdateTexture(tex, nullptr, data, stride);
    return tex;
}

SDL_Texture* TextureCache::UploadRGB16(const uint8_t* data, int w, int h, int stride)
{
    SDL_Texture* tex = SDL_CreateTexture(renderer_,
        SDL_PIXELFORMAT_RGB565,
        SDL_TEXTUREACCESS_STATIC, w, h);
    if (!tex)
    {
        fprintf(stderr, "[TextureCache] SDL_CreateTexture failed: %s\n", SDL_GetError());
        return nullptr;
    }
    SDL_UpdateTexture(tex, nullptr, data, stride);
    return tex;
}

std::unique_ptr<uint8_t[]> TextureCache::ConvertToARGB32(
    const uint8_t* src, int w, int h, int src_bpp, int src_stride)
{
    auto dst = std::make_unique<uint8_t[]>(w * h * 4);

    if (src_bpp == 3)
    {
        // RGB24 → ARGB32
        for (int y = 0; y < h; y++)
        {
            const uint8_t* s = src + y * src_stride;
            uint8_t* d = dst.get() + y * w * 4;
            for (int x = 0; x < w; x++)
            {
                // AGS R8G8B8: memory order is B, G, R (like Allegro)
                d[x * 4 + 0] = s[x * 3 + 0]; // B
                d[x * 4 + 1] = s[x * 3 + 1]; // G
                d[x * 4 + 2] = s[x * 3 + 2]; // R
                d[x * 4 + 3] = 255;            // A
            }
        }
    }
    else if (src_bpp == 1)
    {
        // 8-bit indexed -> ARGB32 using game palette if available
        for (int y = 0; y < h; y++)
        {
            const uint8_t* s = src + y * src_stride;
            uint8_t* d = dst.get() + y * w * 4;
            for (int x = 0; x < w; x++)
            {
                uint8_t idx = s[x];
                if (has_palette_)
                {
                    uint32_t c = palette_[idx];
                    d[x * 4 + 0] = (c >>  0) & 0xFF; // B
                    d[x * 4 + 1] = (c >>  8) & 0xFF; // G
                    d[x * 4 + 2] = (c >> 16) & 0xFF; // R
                    d[x * 4 + 3] = (c >> 24) & 0xFF; // A
                }
                else
                {
                    // Grayscale fallback when no palette is available
                    d[x * 4 + 0] = idx; // B
                    d[x * 4 + 1] = idx; // G
                    d[x * 4 + 2] = idx; // R
                    d[x * 4 + 3] = 255;
                }
            }
        }
    }
    else
    {
        // Unknown format — fill magenta for debugging
        for (int y = 0; y < h; y++)
        {
            uint8_t* d = dst.get() + y * w * 4;
            for (int x = 0; x < w; x++)
            {
                d[x * 4 + 0] = 255; // B
                d[x * 4 + 1] = 0;   // G
                d[x * 4 + 2] = 255; // R
                d[x * 4 + 3] = 255;
            }
        }
    }

    return dst;
}

} // namespace AGSEditor
