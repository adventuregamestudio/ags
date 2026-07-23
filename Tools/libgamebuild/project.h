// AGS - Project management (read-only AGF loader for agdta)
#pragma once

#include <memory>
#include <string>

namespace AGSBuild
{

class GameData;

class Project
{
public:
    Project();
    ~Project();

    bool IsLoaded() const { return loaded_; }
    const std::string& GetProjectPath() const { return project_path_; }
    const std::string& GetProjectDir() const { return project_dir_; }
    void SetProjectDir(const std::string& dir);

    bool OpenProject(const std::string& path);
    void CloseProject();

    GameData* GetGameData() { return game_data_.get(); }

private:
    bool LoadFromAGF(const std::string& agf_path);
    void UpdateProjectDir();

    bool loaded_ = false;
    std::string project_path_;
    std::string project_dir_;

    std::unique_ptr<GameData> game_data_;
};

} // namespace AGSBuild
