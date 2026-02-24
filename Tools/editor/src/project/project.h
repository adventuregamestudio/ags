// AGS Editor ImGui - Project management
#pragma once

#include <string>
#include <vector>
#include <memory>

namespace AGSEditor
{

class GameData;
class SpriteLoader;
class RoomLoader;
class ScriptAPIData;
class TemplateManager;
struct ImportResult;

// Manages the list of recently opened projects
class RecentFiles
{
public:
    static const int kMaxRecentFiles = 10;

    void Load();
    void Save() const;
    void AddFile(const std::string& path);
    void Clear();

    const std::vector<std::string>& GetFiles() const { return files_; }

private:
    std::string GetConfigPath() const;
    std::vector<std::string> files_;
};

// Manages an AGS game project (.agf file)
class Project
{
public:
    Project();
    ~Project();

    bool IsLoaded() const { return loaded_; }
    bool IsDirty() const { return dirty_; }
    const std::string& GetProjectPath() const { return project_path_; }
    const std::string& GetProjectDir() const { return project_dir_; }
    const std::string& GetGameTitle() const { return game_title_; }
    void SetGameTitle(const std::string& title) { game_title_ = title; dirty_ = true; }

    // Project operations
    bool NewProject(const std::string& path, const std::string& title,
                    int width, int height, int color_depth);
    bool OpenProject(const std::string& path);
    bool ImportOldGame(const std::string& game_file, bool create_backup,
                       const std::string& backup_dir, bool import_editor_dat);
    bool SaveProject();
    bool SaveProjectAs(const std::string& path);
    void CloseProject();
    void MarkDirty() { dirty_ = true; }

    // Access game data
    GameData* GetGameData() { return game_data_.get(); }

    // Access sprite loader
    SpriteLoader* GetSpriteLoader() { return sprite_loader_.get(); }

    // Access room loader
    RoomLoader* GetRoomLoader() { return room_loader_.get(); }

    // Access script API data
    ScriptAPIData* GetScriptAPIData() { return script_api_data_.get(); }

    // Recent files
    RecentFiles& GetRecentFiles() { return recent_files_; }

private:
    bool LoadGameFromAGS(const std::string& filepath);
    bool LoadFromAGF(const std::string& agf_path);
    bool SaveToAGF(const std::string& agf_path);
    bool CreateDefaultGameData(const std::string& title, int width, int height, int color_depth);
    void UpdateProjectDir();
    void InitSubsystems(const std::string& game_dir);

    bool loaded_ = false;
    bool dirty_ = false;
    std::string project_path_;
    std::string project_dir_;
    std::string game_title_;

    std::unique_ptr<GameData> game_data_;
    std::unique_ptr<SpriteLoader> sprite_loader_;
    std::unique_ptr<RoomLoader> room_loader_;
    std::unique_ptr<ScriptAPIData> script_api_data_;
    RecentFiles recent_files_;
};

} // namespace AGSEditor
