// AGS Editor ImGui - Old Game Importer implementation
// Reads AGS 2.72 editor.dat metadata and combines with binary game loading
#include "old_game_importer.h"
#include "game_data.h"

#include "ac/gamesetupstruct.h"
#include "ac/view.h"
#include "ac/dialogtopic.h"
#include "game/main_game_file.h"
#include "util/file.h"
#include "util/string.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

namespace AGSEditor
{

// ---------------------------------------------------------------------------
// Utility: read null-terminated string from buffer
// ---------------------------------------------------------------------------
std::string OldGameImporter::ReadNullTermString(
    const uint8_t* data, size_t& offset, size_t max_size)
{
    std::string result;
    while (offset < max_size && data[offset] != 0)
    {
        result += static_cast<char>(data[offset]);
        offset++;
    }
    if (offset < max_size) offset++; // skip null terminator
    return result;
}

// ---------------------------------------------------------------------------
// Utility: read length-prefixed string (int32 length, then chars + null)
// ---------------------------------------------------------------------------
std::string OldGameImporter::ReadLenPrefixedString(
    const uint8_t* data, size_t& offset, size_t max_size)
{
    if (offset + 4 > max_size) return "";
    int32_t len = *reinterpret_cast<const int32_t*>(data + offset);
    offset += 4;
    if (len <= 0 || offset + static_cast<size_t>(len) > max_size) return "";

    std::string result(reinterpret_cast<const char*>(data + offset),
                       static_cast<size_t>(len));
    offset += static_cast<size_t>(len);

    // Remove trailing null if present
    while (!result.empty() && result.back() == '\0')
        result.pop_back();

    return result;
}

// ---------------------------------------------------------------------------
// Check if a file is an old (pre-3.x) AGS game binary
// ---------------------------------------------------------------------------
bool OldGameImporter::IsOldGameFile(const std::string& filepath)
{
    std::string ext = fs::path(filepath).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    // Old game files are typically .dta or .ags
    if (ext != ".dta" && ext != ".ags")
        return false;

    // Check if editor.dat exists alongside (strong indicator of old format)
    std::string dir = fs::path(filepath).parent_path().string();
    if (fs::exists(dir + "/editor.dat"))
        return true;

    // Also check for ac2game.dta which is the classic old filename
    std::string filename = fs::path(filepath).filename().string();
    std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);
    return (filename == "ac2game.dta" || filename == "ac2game.ags");
}

// ---------------------------------------------------------------------------
// Read editor.dat — AGS 2.72 editor metadata
// Format: "AGSEditorInfo\0" signature, int32 version (7), then sequential data
// ---------------------------------------------------------------------------
bool OldGameImporter::ReadEditorDat(const std::string& game_dir,
                                     EditorDatData& out)
{
    std::string path = game_dir + "/editor.dat";
    if (!fs::exists(path))
    {
        fprintf(stderr, "[ImportOld] editor.dat not found in '%s'\n", game_dir.c_str());
        return false;
    }

    // Read entire file into memory
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        fprintf(stderr, "[ImportOld] Cannot open editor.dat\n");
        return false;
    }

    size_t file_size = static_cast<size_t>(file.tellg());
    if (file_size < 18) // minimum: 14 sig + 4 version
    {
        fprintf(stderr, "[ImportOld] editor.dat too small (%zu bytes)\n", file_size);
        return false;
    }

    std::vector<uint8_t> buffer(file_size);
    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(file_size));
    file.close();

    const uint8_t* data = buffer.data();
    size_t offset = 0;

    // Check signature: "AGSEditorInfo\0" (14 bytes)
    const char* expected_sig = "AGSEditorInfo";
    if (memcmp(data, expected_sig, 14) != 0)
    {
        fprintf(stderr, "[ImportOld] editor.dat: invalid signature\n");
        return false;
    }
    offset = 14;

    // Version
    if (offset + 4 > file_size) return false;
    int32_t version = *reinterpret_cast<const int32_t*>(data + offset);
    offset += 4;

    if (version != EditorDatData::kEditorDatVersion)
    {
        fprintf(stderr, "[ImportOld] editor.dat version %d unsupported (expected %d / AGS 2.72)\n",
                version, EditorDatData::kEditorDatVersion);
        return false;
    }

    fprintf(stderr, "[ImportOld] Reading editor.dat v%d (%zu bytes)\n", version, file_size);

    // 1. Global script header (length-prefixed)
    out.global_script_header = ReadLenPrefixedString(data, offset, file_size);

    // 2. Global script (length-prefixed)
    out.global_script = ReadLenPrefixedString(data, offset, file_size);

    // 3. Sprite folders
    if (offset + 4 <= file_size)
    {
        int32_t folder_count = *reinterpret_cast<const int32_t*>(data + offset);
        offset += 4;

        if (folder_count > 0 && folder_count < 10000) // sanity check
        {
            out.sprite_folders.resize(static_cast<size_t>(folder_count));
            for (int i = 0; i < folder_count && offset < file_size; i++)
            {
                auto& folder = out.sprite_folders[static_cast<size_t>(i)];

                // 240 int16 sprite numbers
                size_t needed = 240 * sizeof(int16_t) + sizeof(int16_t) + 30;
                if (offset + needed > file_size) break;

                memcpy(folder.sprites, data + offset, 240 * sizeof(int16_t));
                offset += 240 * sizeof(int16_t);

                // Parent folder index
                folder.parent_folder = *reinterpret_cast<const int16_t*>(data + offset);
                offset += sizeof(int16_t);

                // 30-char folder name
                memcpy(folder.name, data + offset, 30);
                folder.name[30] = '\0';
                offset += 30;
            }
        }
    }

    // 4. Room descriptions (room count + descriptions)
    if (offset + 4 <= file_size)
    {
        int32_t room_count = *reinterpret_cast<const int32_t*>(data + offset);
        offset += 4;

        if (room_count > 0 && room_count < 1000) // sanity
        {
            for (int i = 0; i < room_count && offset < file_size; i++)
            {
                EditorDatData::RoomDescription rd;
                rd.number = i;
                rd.description = ReadNullTermString(data, offset, file_size);
                if (!rd.description.empty())
                    out.room_descriptions.push_back(rd);
            }
        }
    }

    // 5. Script modules (header int32 == 1, then module count)
    if (offset + 4 <= file_size)
    {
        int32_t module_header = *reinterpret_cast<const int32_t*>(data + offset);
        offset += 4;

        if (module_header == 1 && offset + 4 <= file_size)
        {
            int32_t module_count = *reinterpret_cast<const int32_t*>(data + offset);
            offset += 4;

            if (module_count > 0 && module_count < 1000) // sanity
            {
                for (int i = 0; i < module_count && offset < file_size; i++)
                {
                    EditorDatData::ScriptModuleEntry mod;
                    mod.author = ReadNullTermString(data, offset, file_size);
                    mod.description = ReadNullTermString(data, offset, file_size);
                    mod.name = ReadNullTermString(data, offset, file_size);
                    mod.version = ReadNullTermString(data, offset, file_size);

                    // Script text (length-prefixed)
                    mod.script_text = ReadLenPrefixedString(data, offset, file_size);

                    // Header text (length-prefixed)
                    mod.header_text = ReadLenPrefixedString(data, offset, file_size);

                    // Unique key
                    mod.unique_key = ReadNullTermString(data, offset, file_size);

                    // Permissions + owner (int32 each)
                    if (offset + 8 <= file_size)
                    {
                        mod.permissions = *reinterpret_cast<const int32_t*>(data + offset);
                        offset += 4;
                        mod.owner = *reinterpret_cast<const int32_t*>(data + offset);
                        offset += 4;
                    }

                    out.script_modules.push_back(std::move(mod));
                }
            }
        }
    }

    // Remaining data (VOX files list, COM plugin state) is skipped as it's
    // not needed for the import.

    fprintf(stderr, "[ImportOld] editor.dat parsed: %zu script modules, %zu sprite folders, %zu rooms\n",
            out.script_modules.size(), out.sprite_folders.size(), out.room_descriptions.size());

    return true;
}

// ---------------------------------------------------------------------------
// Backup game directory
// ---------------------------------------------------------------------------
bool OldGameImporter::BackupGameDir(const std::string& game_dir,
                                     const std::string& backup_dir)
{
    std::string target = backup_dir;
    if (target.empty())
    {
        // Auto: create "Backup" sibling directory
        target = fs::path(game_dir).parent_path().string() + "/Backup";
    }

    // If target exists, try Backup1, Backup2, etc.
    std::string base = target;
    int suffix = 1;
    while (fs::exists(target) && suffix < 100)
    {
        target = base + std::to_string(suffix);
        suffix++;
    }

    try
    {
        fs::create_directories(target);
        fs::copy(game_dir, target, fs::copy_options::recursive | fs::copy_options::overwrite_existing);
        fprintf(stderr, "[ImportOld] Backup created at '%s'\n", target.c_str());
        return true;
    }
    catch (const fs::filesystem_error& e)
    {
        fprintf(stderr, "[ImportOld] Backup failed: %s\n", e.what());
        return false;
    }
}

// ---------------------------------------------------------------------------
// Apply editor.dat data onto the existing GameData loaded from binary
// ---------------------------------------------------------------------------
void OldGameImporter::ApplyEditorDatData(const EditorDatData& edat, GameData& data)
{
    // Import script modules as ScriptModule entries
    for (const auto& mod : edat.script_modules)
    {
        GameData::ScriptModule sm;
        sm.name = mod.name;
        // The scripts are stored inline in editor.dat
        // Create placeholder filenames (would be saved as .asc/.ash)
        sm.header_file = mod.name + ".ash";
        sm.script_file = mod.name + ".asc";
        data.script_modules.push_back(std::move(sm));
    }

    // Apply room descriptions from editor.dat (more descriptive than binary)
    for (const auto& rd : edat.room_descriptions)
    {
        for (auto& r : data.rooms)
        {
            if (r.number == rd.number && !rd.description.empty())
            {
                r.description = rd.description;
                break;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Full import process
// ---------------------------------------------------------------------------
ImportResult OldGameImporter::Import(const ImportOptions& options,
                                      GameData& out_data,
                                      ProgressCallback progress)
{
    ImportResult result;

    if (options.game_file_path.empty())
    {
        result.error_message = "No game file specified.";
        return result;
    }

    if (!fs::exists(options.game_file_path))
    {
        result.error_message = "Game file not found: " + options.game_file_path;
        return result;
    }

    std::string game_dir = fs::path(options.game_file_path).parent_path().string();

    // Step 1: Backup
    if (progress) progress("Creating backup...", 0.0f);

    if (options.create_backup)
    {
        if (!BackupGameDir(game_dir, options.backup_dir))
        {
            result.warnings.push_back("Failed to create backup. Continuing anyway.");
        }
    }

    // Step 2: Load binary game data using AGS Common
    if (progress) progress("Reading game data...", 0.2f);

    using namespace AGS::Common;

    MainGameSource src;
    HGameFileError err = OpenMainGameFile(
        String(options.game_file_path.c_str()), src);
    if (!err)
    {
        result.error_message = std::string("Failed to open game file: ") +
                               err->FullMessage().GetCStr();
        return result;
    }

    GameSetupStruct game;
    LoadedGameEntities ents(game);
    err = ReadGameData(ents, std::move(src.InputStream), src.DataVersion);
    if (!err)
    {
        result.error_message = std::string("Failed to read game data: ") +
                               err->FullMessage().GetCStr();
        return result;
    }

    err = UpdateGameData(ents, src.DataVersion);
    if (!err)
    {
        result.warnings.push_back(
            std::string("Game data update had issues: ") + err->FullMessage().GetCStr());
    }

    if (progress) progress("Converting game data...", 0.5f);

    // Step 3: Convert to GameData
    out_data.game_title = game.gamename.GetCStr();
    out_data.resolution_width = game.GetGameRes().Width;
    out_data.resolution_height = game.GetGameRes().Height;
    out_data.color_depth = game.color_depth * 8;
    out_data.target_fps = 40;

    // Characters
    for (int i = 0; i < game.numcharacters; i++)
    {
        GameData::CharacterInfo ch;
        ch.id = i;
        ch.script_name = game.chars[i].scrname;
        ch.real_name = game.chars[i].name;
        ch.room = game.chars[i].room;
        ch.x = game.chars[i].x;
        ch.y = game.chars[i].y;
        out_data.characters.push_back(ch);
    }
    result.characters_imported = static_cast<int>(out_data.characters.size());

    // Views
    for (size_t i = 0; i < ents.Views.size(); i++)
    {
        GameData::ViewInfo v;
        v.id = static_cast<int>(i);
        if (i < game.viewNames.size())
            v.name = game.viewNames[i].GetCStr();
        else
            v.name = "View " + std::to_string(i);
        v.loop_count = static_cast<int>(ents.Views[i].loops.size());

        for (size_t l = 0; l < ents.Views[i].loops.size(); l++)
        {
            GameData::LoopData loop;
            loop.run_next_loop = (ents.Views[i].loops[l].flags & LOOPFLAG_RUNNEXTLOOP) != 0;
            for (size_t f = 0; f < ents.Views[i].loops[l].frames.size(); f++)
            {
                GameData::FrameData frame;
                frame.sprite_id = ents.Views[i].loops[l].frames[f].pic;
                frame.x_offset = ents.Views[i].loops[l].frames[f].xoffs;
                frame.y_offset = ents.Views[i].loops[l].frames[f].yoffs;
                frame.delay = ents.Views[i].loops[l].frames[f].speed;
                frame.flipped = (ents.Views[i].loops[l].frames[f].flags & VFLG_FLIPSPRITE) != 0;
                frame.sound = ents.Views[i].loops[l].frames[f].sound;
                loop.frames.push_back(frame);
            }
            v.loops.push_back(loop);
        }
        out_data.views.push_back(v);
    }
    result.views_imported = static_cast<int>(out_data.views.size());

    // Dialogs
    for (size_t i = 0; i < ents.Dialogs.size(); i++)
    {
        GameData::DialogInfo d;
        d.id = static_cast<int>(i);
        if (i < game.dialogScriptNames.size())
            d.name = game.dialogScriptNames[i].GetCStr();
        else
            d.name = "Dialog " + std::to_string(i);
        d.option_count = static_cast<int>(ents.Dialogs[i].Options.size());
        out_data.dialogs.push_back(d);
    }
    result.dialogs_imported = static_cast<int>(out_data.dialogs.size());

    // GUIs
    for (size_t i = 0; i < ents.Guis.size(); i++)
    {
        GameData::GUIInfo g;
        g.id = static_cast<int>(i);
        g.name = ents.Guis[i].GetName().GetCStr();
        g.width = ents.Guis[i].GetWidth();
        g.height = ents.Guis[i].GetHeight();
        g.visible = ents.Guis[i].IsVisible();
        out_data.guis.push_back(g);
    }
    result.guis_imported = static_cast<int>(out_data.guis.size());

    // Cursors
    for (size_t i = 0; i < game.mcurs.size(); i++)
    {
        GameData::CursorInfo c;
        c.id = static_cast<int>(i);
        c.name = game.mcurs[i].name.GetCStr();
        c.image = game.mcurs[i].pic;
        c.hotspot_x = game.mcurs[i].hotx;
        c.hotspot_y = game.mcurs[i].hoty;
        c.animate = (game.mcurs[i].view >= 0);
        c.view = game.mcurs[i].view;
        c.process_click = (game.mcurs[i].flags & MCF_STANDARD) != 0;
        out_data.cursors.push_back(c);
    }

    // Inventory items
    for (int i = 0; i < game.numinvitems; i++)
    {
        GameData::InventoryItemInfo inv;
        inv.id = i;
        inv.script_name = std::string(game.invScriptNames[i].GetCStr());
        inv.description = std::string(game.invinfo[i].name.GetCStr());
        inv.image = game.invinfo[i].pic;
        inv.cursor_image = game.invinfo[i].cursorPic;
        inv.start_with = (game.invinfo[i].flags & IFLG_STARTWITH) != 0;
        out_data.inventory_items.push_back(inv);
    }

    // Fonts
    for (size_t i = 0; i < game.fonts.size(); i++)
    {
        GameData::FontInfo f;
        f.id = static_cast<int>(i);
        f.name = "Font " + std::to_string(i);
        f.size = game.fonts[i].Size;
        f.outline_type = (game.fonts[i].Flags & 0x01) ? 1 : 0;
        f.outline_font = game.fonts[i].Outline;
        f.line_spacing = game.fonts[i].LineSpacing;
        f.size_multiplier = game.fonts[i].SizeMultiplier;
        out_data.fonts.push_back(f);
    }

    // Audio clips
    for (size_t i = 0; i < game.audioClips.size(); i++)
    {
        GameData::AudioClipInfo a;
        a.id = static_cast<int>(i);
        a.name = game.audioClips[i].scriptName.GetCStr();
        a.filename = game.audioClips[i].fileName.GetCStr();
        a.type = game.audioClips[i].type;
        out_data.audio_clips.push_back(a);
    }

    // Rooms
    for (int i = 0; i < game.roomCount; i++)
    {
        GameData::RoomInfo r;
        r.number = game.roomNumbers[i];
        if (i < static_cast<int>(game.roomNames.size()))
            r.description = game.roomNames[i].GetCStr();
        else
            r.description = "Room " + std::to_string(game.roomNumbers[i]);
        out_data.rooms.push_back(r);
    }
    result.rooms_imported = static_cast<int>(out_data.rooms.size());

    // Sort rooms by number
    std::sort(out_data.rooms.begin(), out_data.rooms.end(),
        [](const GameData::RoomInfo& a, const GameData::RoomInfo& b) {
            return a.number < b.number;
        });

    // Palette
    for (int i = 0; i < 256; i++)
    {
        GameData::PaletteEntry pe;
        pe.r = game.defpal[i].r * 4;
        pe.g = game.defpal[i].g * 4;
        pe.b = game.defpal[i].b * 4;
        pe.colour_type = game.paluses[i]; // 0=Gamewide, 1=Locked, 2=Background
        out_data.palette.push_back(pe);
    }

    // Sprites
    for (size_t i = 0; i < ents.SpriteCount && i < ents.SpriteFlags.size(); i++)
    {
        GameData::SpriteInfo s;
        s.id = static_cast<int>(i);
        s.width = 0;
        s.height = 0;
        s.color_depth = 0;
        out_data.sprites.push_back(s);
    }
    result.sprites_imported = static_cast<int>(out_data.sprites.size());

    // Step 4: Read editor.dat if present and requested
    if (progress) progress("Reading editor metadata...", 0.8f);

    if (options.import_editor_dat)
    {
        EditorDatData edat;
        if (ReadEditorDat(game_dir, edat))
        {
            ApplyEditorDatData(edat, out_data);
            result.had_editor_dat = true;
            result.script_modules_imported = static_cast<int>(edat.script_modules.size());

            // Save scripts to disk as .asc / .ash files
            for (const auto& mod : edat.script_modules)
            {
                // Save script file
                if (!mod.script_text.empty())
                {
                    std::string script_path = game_dir + "/" + mod.name + ".asc";
                    std::ofstream sf(script_path);
                    if (sf.is_open())
                    {
                        sf << mod.script_text;
                        sf.close();
                    }
                    else
                    {
                        result.warnings.push_back("Failed to save script: " + script_path);
                    }
                }

                // Save header file
                if (!mod.header_text.empty())
                {
                    std::string header_path = game_dir + "/" + mod.name + ".ash";
                    std::ofstream hf(header_path);
                    if (hf.is_open())
                    {
                        hf << mod.header_text;
                        hf.close();
                    }
                    else
                    {
                        result.warnings.push_back("Failed to save header: " + header_path);
                    }
                }
            }

            // Save global script and header
            if (!edat.global_script.empty())
            {
                std::string path = game_dir + "/GlobalScript.asc";
                std::ofstream f(path);
                if (f.is_open()) { f << edat.global_script; f.close(); }
            }
            if (!edat.global_script_header.empty())
            {
                std::string path = game_dir + "/GlobalScript.ash";
                std::ofstream f(path);
                if (f.is_open()) { f << edat.global_script_header; f.close(); }
            }
        }
        else
        {
            result.warnings.push_back("editor.dat not found or unreadable. "
                "Script modules and sprite folders were not imported.");
        }
    }

    if (progress) progress("Import complete.", 1.0f);

    result.success = true;
    return result;
}

} // namespace AGSEditor
