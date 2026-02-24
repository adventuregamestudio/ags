// AGS Editor ImGui - Template Manager
// Discovers, validates, and extracts .agt game template files
// using the same CLIB pack format as the C# editor.
#pragma once

#include <string>
#include <vector>
#include <memory>

namespace AGSEditor
{

// Metadata about a game template (.agt file)
struct GameTemplate
{
    std::string file_path;      // full path to .agt file
    std::string file_name;      // filename only (e.g. "Empty Game.agt")
    std::string friendly_name;  // display name (e.g. "Empty Game")
    std::string description;    // from template.txt inside the .agt
    bool valid = false;         // true if template contains game.agf or editor.dat
};

// Manages discovery and extraction of AGS game templates (.agt files)
// Template format: CLIB pack files containing Game.agf + assets
class TemplateManager
{
public:
    TemplateManager();
    ~TemplateManager();

    // Scan directories for .agt template files and populate the template list.
    // Searches:
    //   1. <editor_dir>/Templates/
    //   2. ~/.local/share/ags/Templates/ (Linux) or %LOCALAPPDATA%/AGS/Templates/ (Win)
    //   3. Any additional paths added via AddSearchPath()
    void DiscoverTemplates(const std::string& editor_dir);

    // Add an additional search path for templates
    void AddSearchPath(const std::string& path);

    // Get the list of discovered templates
    const std::vector<GameTemplate>& GetTemplates() const { return templates_; }

    // Extract a template into a target directory, creating a new game project.
    // Returns true on success.
    // Extracts all files except "template.dta" (the lock file).
    // The extracted project will have Game.agf as its main project file.
    bool ExtractTemplate(const GameTemplate& tmpl, const std::string& target_dir);

    // Create a .agt template from the current game project.
    // Collects Game.agf, sprites, rooms, scripts, fonts, audio, translations etc.
    // and packs them into a CLIB format .agt file.
    // Returns true on success.
    bool MakeTemplate(const std::string& project_dir, const std::string& output_agt_path);

    // Create standard subdirectories that AGS expects:
    // Speech/, AudioCache/, Compiled/, Compiled/Data/
    static void EnsureStandardSubFolders(const std::string& game_dir);

private:
    // Load metadata (description, validation) for a single .agt file
    GameTemplate LoadTemplateMetadata(const std::string& agt_path);

    // Scan a single directory for .agt files
    void ScanDirectory(const std::string& dir);

    std::vector<GameTemplate> templates_;
    std::vector<std::string> search_paths_;
};

} // namespace AGSEditor
