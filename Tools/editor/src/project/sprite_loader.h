// AGS Editor ImGui - Sprite file loader using AGS Common SpriteFile
#pragma once

#include <string>
#include <vector>
#include <memory>

namespace AGS { namespace Common { class SpriteFile; } }

namespace AGSEditor
{

struct SpriteMetrics
{
    int id = 0;
    int width = 0;
    int height = 0;
    int color_depth = 0;  // bytes per pixel
    bool exists = false;
};

// Loads sprite metrics (dimensions, color depth) from an AGS sprite file
// without loading actual pixel data.
class SpriteLoader
{
public:
    SpriteLoader();
    ~SpriteLoader();

    // Opens the sprite file (acsprset.spr) and optional index (spriteindex.dat)
    bool Open(const std::string& game_dir);
    void Close();

    bool IsOpen() const { return is_open_; }
    int  GetSpriteCount() const { return (int)metrics_.size(); }
    int  GetTopmostSprite() const { return topmost_sprite_; }

    // Get metrics for a specific sprite
    const SpriteMetrics* GetMetrics(int index) const;
    // Get all metrics
    const std::vector<SpriteMetrics>& GetAllMetrics() const { return metrics_; }

    // Check if a sprite slot has valid data
    bool DoesSpriteExist(int index) const;

    // Access the underlying SpriteFile (for pixel loading)
    AGS::Common::SpriteFile* GetSpriteFile() { return sprite_file_.get(); }

private:
    bool is_open_ = false;
    int topmost_sprite_ = -1;
    std::vector<SpriteMetrics> metrics_;
    std::unique_ptr<AGS::Common::SpriteFile> sprite_file_;
};

} // namespace AGSEditor
