// AGS Editor ImGui - Old Game Importer
// Imports pre-3.x AGS games (AGS 2.72 format) by reading the main game
// binary data plus the editor.dat metadata file.
#pragma once

#include <string>
#include <vector>
#include <functional>
#include <cstdint>

namespace AGSEditor
{

struct GameData;

// Data read from editor.dat (AGS 2.72 editor metadata)
struct EditorDatData
{
    static const int kEditorDatVersion = 7; // AGS 2.72

    std::string global_script_header;
    std::string global_script;

    struct SpriteFolderEntry {
        int16_t sprites[240];
        int16_t parent_folder;
        char name[31]; // 30 chars + null
    };
    std::vector<SpriteFolderEntry> sprite_folders;

    struct ScriptModuleEntry {
        std::string author;
        std::string description;
        std::string name;
        std::string version;
        std::string script_text;
        std::string header_text;
        std::string unique_key;
        int permissions = 0;
        int owner = 0;
    };
    std::vector<ScriptModuleEntry> script_modules;

    struct RoomDescription {
        int number;
        std::string description;
    };
    std::vector<RoomDescription> room_descriptions;
};

// Result of an import operation
struct ImportResult
{
    bool success = false;
    std::string error_message;
    std::vector<std::string> warnings;

    int characters_imported = 0;
    int views_imported = 0;
    int dialogs_imported = 0;
    int guis_imported = 0;
    int rooms_imported = 0;
    int sprites_imported = 0;
    int script_modules_imported = 0;
    bool had_editor_dat = false;
};

// Import options for the wizard
struct ImportOptions
{
    std::string game_file_path;     // Path to ac2game.dta / game.ags
    bool create_backup = true;
    std::string backup_dir;         // Empty = auto (sibling "Backup/" dir)
    bool import_editor_dat = true;  // Try to read editor.dat if present
};

class OldGameImporter
{
public:
    using ProgressCallback = std::function<void(const std::string& status, float progress)>;

    // Perform the full import process
    static ImportResult Import(const ImportOptions& options,
                               GameData& out_data,
                               ProgressCallback progress = nullptr);

    // Check if a file looks like an old AGS game
    static bool IsOldGameFile(const std::string& filepath);

    // Read editor.dat from the same directory as the game file
    static bool ReadEditorDat(const std::string& game_dir, EditorDatData& out);

    // Create a backup of the game directory
    static bool BackupGameDir(const std::string& game_dir,
                              const std::string& backup_dir);

private:
    // Read a null-terminated string from a binary stream
    static std::string ReadNullTermString(const uint8_t* data, size_t& offset, size_t max_size);

    // Read a length-prefixed string (int32 length + chars + null)
    static std::string ReadLenPrefixedString(const uint8_t* data, size_t& offset, size_t max_size);

    // Apply editor.dat data onto GameData
    static void ApplyEditorDatData(const EditorDatData& edat, GameData& data);
};

} // namespace AGSEditor
