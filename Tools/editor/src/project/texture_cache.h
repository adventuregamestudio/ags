// AGS Editor ImGui - SDL Texture Cache
// Converts AGS sprite pixel data and Bitmap objects to SDL textures
// for rendering in ImGui via ImGui::Image().
#pragma once

#include <map>
#include <string>
#include <memory>

struct SDL_Texture;
struct SDL_Renderer;

namespace AGS { namespace Common { class Bitmap; class PixelBuffer; } }

namespace AGSEditor
{

class SpriteLoader;

// Manages GPU-uploaded SDL textures for sprites and room backgrounds.
// Textures are created on demand and cached by key (sprite ID or custom key).
class TextureCache
{
public:
    TextureCache();
    ~TextureCache();

    // Set the SDL renderer (must be called before any texture creation)
    void SetRenderer(SDL_Renderer* renderer) { renderer_ = renderer; }

    // Set 256-color palette for 8-bit sprite rendering.
    // Each entry is ARGB32 (0xAARRGGBB).
    void SetPalette(const uint32_t* palette_argb32, int count);
    bool HasPalette() const { return has_palette_; }
    const uint32_t* GetPalette() const { return palette_; }

    // Get or create an SDL texture for a sprite.
    // Returns nullptr if the sprite cannot be loaded.
    // The texture is owned by the cache and valid until Evict/Clear.
    SDL_Texture* GetSpriteTexture(int sprite_id, SpriteLoader* loader);

    // Create a texture from an AGS Bitmap (e.g. room background).
    // key is an arbitrary unique identifier (e.g. "room5_bg0").
    // The bitmap data is copied into a GPU texture.
    SDL_Texture* GetOrCreateFromBitmap(const std::string& key,
                                        const AGS::Common::Bitmap* bmp);

    // Create a texture from a PixelBuffer.
    SDL_Texture* CreateFromPixelBuffer(const AGS::Common::PixelBuffer& pixbuf);

    // Evict a single entry
    void Evict(const std::string& key);
    void EvictSprite(int sprite_id);

    // Clear all cached textures
    void Clear();

    // Get texture dimensions (filled on creation)
    struct TexInfo {
        SDL_Texture* texture = nullptr;
        int width = 0;
        int height = 0;
    };

    // Look up without creating
    const TexInfo* Find(const std::string& key) const;
    const TexInfo* FindSprite(int sprite_id) const;

    // Stats
    int GetCachedCount() const { return (int)cache_.size(); }

private:
    SDL_Texture* UploadARGB32(const uint8_t* data, int w, int h, int stride);
    SDL_Texture* UploadRGB16(const uint8_t* data, int w, int h, int stride);

    // Convert non-32-bit pixel data to ARGB32 for upload
    std::unique_ptr<uint8_t[]> ConvertToARGB32(const uint8_t* src, int w, int h,
                                                int src_bpp, int src_stride);

    SDL_Renderer* renderer_ = nullptr;
    std::map<std::string, TexInfo> cache_;
    uint32_t palette_[256] = {};
    bool has_palette_ = false;
};

} // namespace AGSEditor
