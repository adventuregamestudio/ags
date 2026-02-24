// AGS Editor ImGui - Sprite Chooser Popup
// Reusable modal popup for selecting a sprite from the sprite set.
#pragma once

struct SDL_Texture;

namespace AGSEditor
{

class SpriteLoader;
class TextureCache;

// Draw a sprite chooser modal popup. Call from within your Draw() loop.
// Returns true if a sprite was selected (result written to *out_sprite).
// popup_id: unique string for ImGui popup ID
// loader: sprite loader for metrics/count
// tex_cache: texture cache for rendering sprite thumbnails
// out_sprite: receives the selected sprite ID when user clicks one
// current_sprite: currently selected sprite (highlighted in the grid)
bool DrawSpriteChooserPopup(const char* popup_id, SpriteLoader* loader,
                            TextureCache& tex_cache, int* out_sprite,
                            int current_sprite = -1);

// Helper: draw a small sprite preview thumbnail + "..." browse button.
// Returns true if the browse button was clicked (caller should open popup).
// sprite_id: current sprite to preview
// loader: sprite loader for texture lookup
// tex_cache: texture cache for rendering
bool DrawSpriteField(const char* label, int* sprite_id,
                     SpriteLoader* loader, TextureCache& tex_cache);

} // namespace AGSEditor
