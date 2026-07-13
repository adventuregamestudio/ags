// AGS - Project management (read-only AGF loader for agsbuild)
#pragma once

#include <memory>
#include <string>

namespace AGSBuild
{

class GameData;
class SpriteLoader;

// Loads an AGS game project (.agf) for headless compilation.
class Project
{
public:
    Project();
    ~Project();

    bool IsLoaded() const { return loaded_; }
    const std::string& GetProjectPath() const { return project_path_; }
    const std::string& GetProjectDir() const { return project_dir_; }
    const std::string& GetGameTitle() const { return game_title_; }
    void SetProjectDir(const std::string& dir);

    bool OpenProject(const std::string& path);
    void CloseProject();

    GameData* GetGameData() { return game_data_.get(); }
    SpriteLoader* GetSpriteLoader() { return sprite_loader_.get(); }

    // Reload acsprset.spr metrics after regenerating the sprite file on disk.
    void ReloadSpriteCache();

private:
    bool LoadFromAGF(const std::string& agf_path);
    void UpdateProjectDir();
    void InitSubsystems(const std::string& game_dir);

    bool loaded_ = false;
    std::string project_path_;
    std::string project_dir_;
    std::string game_title_;

    std::unique_ptr<GameData> game_data_;
    std::unique_ptr<SpriteLoader> sprite_loader_;
};

} // namespace AGSBuild
