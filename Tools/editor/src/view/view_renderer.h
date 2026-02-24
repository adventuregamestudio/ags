// AGS Editor ImGui - Native View Renderer
// Composites view loop frames into a single strip bitmap using
// AGS::Common::Bitmap operations, similar to the C# editor's NativeProxy.
#pragma once

#include <SDL.h>
#include <vector>
#include <unordered_map>
#include <string>
#include <cstdint>

namespace AGS { namespace Common { class Bitmap; } }

namespace AGSEditor
{

struct GameData;
class SpriteLoader;
class TextureCache;

// Renders view loop frames using AGS::Common::Bitmap for compositing,
// then uploads the result as an SDL texture for ImGui display.
class ViewRenderer
{
public:
    ViewRenderer() = default;
    ~ViewRenderer();

    void SetRenderer(SDL_Renderer* renderer) { renderer_ = renderer; }

    // Set 256-color palette for 8-bit sprite support (ARGB32 entries)
    void SetPalette(const uint32_t* palette_argb32, int count);
    bool HasPalette() const { return has_palette_; }

    // Per-frame positional info within the composited strip
    struct FrameBounds {
        int x = 0;
        int y = 0;
        int w = 0;
        int h = 0;
    };

    // Result of compositing a loop strip
    struct LoopStripInfo {
        SDL_Texture* texture = nullptr;
        int width = 0;
        int height = 0;
        std::vector<FrameBounds> frame_bounds;
    };

    // Frame data needed for rendering (matches GameData::FrameData fields)
    struct FrameInput {
        int sprite_id = 0;
        bool flipped = false;
    };

    // Composite all frames of a loop into a single strip texture.
    // cell_size: width/height of each frame cell in pixels.
    // Returns info about the composited texture and per-frame bounds.
    LoopStripInfo RenderLoopStrip(
        const std::vector<FrameInput>& frames,
        int cell_size,
        SpriteLoader* loader);

    // Render a single sprite to an AGS Bitmap at a given max size.
    // The bitmap is allocated internally. Returns nullptr on failure.
    // Caller does NOT own the bitmap (it is cached internally).
    AGS::Common::Bitmap* RenderSpriteFitted(
        int sprite_id, int max_w, int max_h,
        bool flipped,
        SpriteLoader* loader);

    // Clear all cached composited textures and bitmaps
    void ClearCache();

private:
    // Load a sprite into an AGS::Common::Bitmap with proper color conversion.
    // Handles 8-bit palette, 16-bit, 24-bit, and 32-bit sprites.
    // Returns a new Bitmap in 32-bit ARGB format.
    AGS::Common::Bitmap* LoadSpriteAsBitmap32(int sprite_id, SpriteLoader* loader);

    // Scale a bitmap proportionally to fit within max dimensions
    void BlitScaled(AGS::Common::Bitmap* src, AGS::Common::Bitmap* dst,
                    int dst_x, int dst_y, int dst_w, int dst_h);

    // Flip a bitmap horizontally in place
    void FlipHorizontal(AGS::Common::Bitmap* bmp);

    SDL_Renderer* renderer_ = nullptr;
    uint32_t palette_[256] = {};
    bool has_palette_ = false;

    // Cache for composited strip textures (keyed by content hash)
    struct CachedStrip {
        SDL_Texture* texture = nullptr;
        LoopStripInfo info;
    };
    std::unordered_map<std::string, CachedStrip> strip_cache_;

    // Temporary bitmap cache for sprite rendering
    std::unordered_map<int, AGS::Common::Bitmap*> sprite_cache_;
};

} // namespace AGSEditor
