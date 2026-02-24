// AGS Editor ImGui - Sprite file loader implementation
#include "sprite_loader.h"
#include "ac/spritefile.h"
#include "util/file.h"
#include "util/stream.h"

#include <cstdio>
#include <filesystem>

namespace fs = std::filesystem;

namespace AGSEditor
{

SpriteLoader::SpriteLoader() = default;
SpriteLoader::~SpriteLoader() { Close(); }

bool SpriteLoader::Open(const std::string& game_dir)
{
    Close();

    using namespace AGS::Common;

    // Construct paths
    std::string sprite_path = (fs::path(game_dir) / SpriteFile::DefaultSpriteFileName.GetCStr()).string();
    std::string index_path  = (fs::path(game_dir) / SpriteFile::DefaultSpriteIndexName.GetCStr()).string();

    // Open sprite stream
    auto sprite_stream = File::OpenFileRead(String(sprite_path.c_str()));
    if (!sprite_stream)
    {
        fprintf(stderr, "[SpriteLoader] Cannot open sprite file: %s\n", sprite_path.c_str());
        return false;
    }

    // Optionally open index stream
    auto index_stream = File::OpenFileRead(String(index_path.c_str()));
    // Index file is optional, we can proceed without it

    // Open and read metrics using the extended SpriteDatHeader overload
    sprite_file_ = std::make_unique<SpriteFile>();
    std::vector<SpriteDatHeader> headers;
    HError err = sprite_file_->OpenFile(
        std::move(sprite_stream),
        std::move(index_stream),
        headers);

    if (!err)
    {
        fprintf(stderr, "[SpriteLoader] Failed to open sprite file: %s\n",
                err->FullMessage().GetCStr());
        sprite_file_.reset();
        return false;
    }

    topmost_sprite_ = (int)sprite_file_->GetTopmostSprite();
    int count = topmost_sprite_ + 1;
    metrics_.resize(count);

    for (int i = 0; i < count; i++)
    {
        metrics_[i].id = i;
        if (i < (int)headers.size() && (headers[i].Width > 0 || headers[i].Height > 0))
        {
            metrics_[i].width = headers[i].Width;
            metrics_[i].height = headers[i].Height;
            metrics_[i].color_depth = headers[i].BPP;
            metrics_[i].exists = true;
        }
        else
        {
            metrics_[i].exists = sprite_file_->DoesSpriteExist(i);
        }
    }

    is_open_ = true;
    fprintf(stderr, "[SpriteLoader] Opened sprite file with %d sprites (topmost: %d)\n",
            (int)sprite_file_->GetSpriteCount(), topmost_sprite_);
    return true;
}

void SpriteLoader::Close()
{
    if (sprite_file_)
        sprite_file_->Close();
    sprite_file_.reset();
    metrics_.clear();
    topmost_sprite_ = -1;
    is_open_ = false;
}

const SpriteMetrics* SpriteLoader::GetMetrics(int index) const
{
    if (index < 0 || index >= (int)metrics_.size())
        return nullptr;
    return &metrics_[index];
}

bool SpriteLoader::DoesSpriteExist(int index) const
{
    if (index < 0 || index >= (int)metrics_.size())
        return false;
    return metrics_[index].exists;
}

} // namespace AGSEditor
